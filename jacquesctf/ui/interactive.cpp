/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <stdexcept>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <boost/optional.hpp>

#include "interactive.hpp"
#include "config.hpp"
#include "stylist.hpp"
#include "state.hpp"
#include "metadata-error.hpp"
#include "inspect-screen.hpp"
#include "help-screen.hpp"
#include "packets-screen.hpp"
#include "data-stream-files-screen.hpp"
#include "data-types-screen.hpp"
#include "trace-infos-screen.hpp"
#include "status-view.hpp"
#include "packet-index-build-progress-view.hpp"
#include "packet-checkpoints-build-progress-view.hpp"
#include "simple-message-view.hpp"
#include "utils.hpp"
#include "packet-checkpoints-build-listener.hpp"
#include "data-size.hpp"

namespace jacques {

static bool screenInited = false;

/*
 * Releases the terminal.
 */
static void finiScreen()
{
    if (screenInited) {
        curs_set(1);
        endwin();
        screenInited = false;
    }
}

/*
 * Returns true if the terminal's size has the minimum dimension.
 */
static bool termSizeOk()
{
    return COLS >= 80 && LINES >= 16;
}

/*
 * Initializes and takes control of the terminal.
 */
static bool initScreen()
{
    initscr();
    screenInited = true;

    if (!has_colors() || !termSizeOk()) {
        finiScreen();

        if (!has_colors()) {
            utils::error() << "Cannot continue: your terminal does not support colors.\n";
        } else if (!termSizeOk()) {
            utils::error() << "Cannot continue: terminal size must be at least 80x16.\n";
        }

        return false;
    }

    noecho();
    cbreak();
    nonl();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    use_default_colors();
    return true;
}

static void sigHandler(const int signo)
{
    if (signo == SIGINT) {
        finiScreen();
        std::cerr << '\n';
        utils::error() << "Interrupted by user.\n";
        std::exit(0);
    }
}

static void registerSignals()
{
    auto ret = signal(SIGINT, sigHandler);

    assert(ret != SIG_ERR);
    JACQUES_UNUSED(ret);
}

static bool init()
{
    registerSignals();
    return initScreen();
}

static void buildIndexes(State& state, std::shared_ptr<const Stylist> stylist)
{
    const auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                       static_cast<Size>(LINES)};
    const auto view = std::make_unique<PacketIndexBuildProgressView>(screenRect,
                                                                     stylist);
    auto func = [&view](const PacketIndexEntry& entry) {
        view->packetIndexEntry(entry);
        view->refresh();
        doupdate();
    };

    view->focus();
    view->isVisible(true);
    view->refresh(true);

    for (auto& dsfStateUp : state.dataStreamFileStates()) {
        auto& dsf = dsfStateUp->dataStreamFile();

        view->dataStreamFile(dsf);
        dsf.buildIndex(func, 443);
    }
}

static void showFullScreenMessage(const std::string& msg,
                                  std::shared_ptr<const Stylist> stylist)
{
    const auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                       static_cast<Size>(LINES)};
    const auto view = std::make_unique<SimpleMessageView>(screenRect, stylist);

    view->message(msg);
    view->focus();
    view->isVisible(true);
    view->refresh();
    doupdate();
}

class PacketCheckpointsBuildProgressUpdater :
    public PacketCheckpointsBuildListener
{
public:
    explicit PacketCheckpointsBuildProgressUpdater(std::shared_ptr<const Stylist> stylist,
                                                   Screen * const * const curScreen) :
        _stylist {stylist},
        _curScreen {curScreen}
    {
    }

private:
    void _startBuild(const PacketIndexEntry& packetIndexEntry) override
    {
        if (packetIndexEntry.packetSize() < 2_MiB) {
            // too fast anyway
            return;
        }

        const auto rect = Rectangle {{4, 4}, static_cast<Size>(COLS) - 8, 13};

        _view = std::make_unique<PacketCheckpointsBuildProgressView>(rect,
                                                                     _stylist);
        _view->focus();
        _view->isVisible(true);
        _view->packetIndexEntry(packetIndexEntry);
        _view->refresh(true);
        doupdate();
        _count = 0;
    }

    void _update(const EventRecord& eventRecord) override
    {
        if (!_view) {
            return;
        }

        if (_count++ % 13 != 0) {
            return;
        }

        _view->eventRecord(eventRecord);
        _view->refresh();
        doupdate();
    }

    void _endBuild() override
    {
        if (_view) {
            _view->isVisible(false);
            _view = nullptr;
        }

        // redraw current screen
        if (*_curScreen) {
            (*_curScreen)->redraw();
            doupdate();
        }
    }

private:
    Index _count = 0;
    std::unique_ptr<PacketCheckpointsBuildProgressView> _view;
    std::shared_ptr<const Stylist> _stylist;
    Screen * const * const _curScreen;
};

static bool tryStartInteractive(const Config& cfg)
{
    auto stylist = std::make_shared<const Stylist>();

    showFullScreenMessage("Opening data stream files...", stylist);

    Screen *curScreen = nullptr;
    auto packetCheckpointsBuildProgressUpdater = std::make_shared<PacketCheckpointsBuildProgressUpdater>(stylist,
                                                                                                         &curScreen);
    std::shared_ptr<State> state;

    try {
        state = std::make_shared<State>(cfg.filePaths(),
                                        packetCheckpointsBuildProgressUpdater);
    } catch (const MetadataError& error) {
        finiScreen();

        auto& metadata = error.metadata();

        utils::error() << "Metadata error: `" << metadata.path().string() << "`: ";

        if (metadata.invalidStreamError()) {
            std::cerr << "Invalid metadata stream: " <<
                         metadata.invalidStreamError()->what() << std::endl;
        } else if (metadata.invalidMetadataError()) {
            std::cerr << "Invalid metadata: " <<
                         metadata.invalidMetadataError()->what() << std::endl;
        } else if (metadata.parseError()) {
            std::cerr << "Cannot parse metadata text:\n" <<
                         metadata.parseError()->what();
        }

        return false;
    }

    if (state->dataStreamFileStates().empty()) {
        finiScreen();
        utils::error() << "All data stream files to inspect are empty.\n";
        return false;
    }

    auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                         static_cast<Size>(LINES) - 1};

    /*
     * At this point, the state is not ready. Data stream files have no
     * packet indexes, and there's no active packet built. This is
     * because we want to provide feedback to the user because it could
     * be a long process. Build indexes first.
     */
    buildIndexes(*state, stylist);

    /*
     * Show this message because some views created by the screens below
     * can perform some "heavy" caching operations initially.
     */
    showFullScreenMessage("Building caches...", stylist);

    // status
    auto statusView = std::make_unique<StatusView>(Rectangle {{0, screenRect.h},
                                                              screenRect.w, 1},
                                                   stylist, state);

    // create screens
    auto inspectScreen = std::make_unique<InspectScreen>(screenRect, cfg,
                                                         stylist, state);
    auto packetsScreen = std::make_unique<PacketsScreen>(screenRect, cfg,
                                                         stylist, state);
    auto dsfScreen = std::make_unique<DataStreamFilesScreen>(screenRect, cfg,
                                                             stylist, state);
    auto helpScreen = std::make_unique<HelpScreen>(screenRect, cfg, stylist,
                                                   state);
    auto dataTypesScreen = std::make_unique<DataTypesScreen>(screenRect, cfg,
                                                             stylist, state);
    auto traceInfosScreen = std::make_unique<TraceInfosScreen>(screenRect, cfg,
                                                               stylist, state);
    std::vector<Screen*> screens {
        inspectScreen.get(),
        packetsScreen.get(),
        dsfScreen.get(),
        helpScreen.get(),
        dataTypesScreen.get(),
        traceInfosScreen.get(),
    };

    // goto first packet if available: this creates it and shows the progress
    showFullScreenMessage("Selecting initial packet...", stylist);

    if (state->activeDataStreamFileState().dataStreamFile().packetCount() > 0) {
        state->gotoPacket(0);
    }

    // draw status
    statusView->isVisible(true);
    statusView->redraw();

    // initial screen depends on the situation
    if (state->dataStreamFileStateCount() == 1) {
        if (state->activeDataStreamFileState().dataStreamFile().packetCount() == 0) {
            curScreen = dsfScreen.get();
        } else if (state->activeDataStreamFileState().dataStreamFile().packetCount() == 1) {
            curScreen = inspectScreen.get();
        } else {
            curScreen = packetsScreen.get();
        }
    } else {
        curScreen = dsfScreen.get();
    }

    Screen *prevScreen = nullptr;
    bool done = false;

    curScreen->isVisible(true);
    doupdate();

    while (!done) {
        const auto ch = getch();
        bool renderStatus = true;

        switch (ch) {
        case KEY_RESIZE:
            // it looks like clearing and refreshing stdscr is required
            clear();
            refresh();

            if (!termSizeOk()) {
                stylist->error();
                mvprintw(0, 0, "Terminal size must be at least 80x16 (currently %dx%d).",
                         COLS, LINES);
                renderStatus = false;
                break;
            }

            screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                    static_cast<Size>(LINES) - 1};
            statusView->moveAndResize(Rectangle {{0, screenRect.h},
                                                 screenRect.w, 1});

            for (auto screen : screens) {
                screen->resize(screenRect.w, screenRect.h);
            }

            curScreen->redraw();
            statusView->redraw();
            break;

        case 'r':
            clear();
            refresh();
            statusView->redraw();
            curScreen->redraw();
            break;

        case 'p':
            if (curScreen == packetsScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = packetsScreen.get();
            curScreen->isVisible(true);
            break;

        case 'f':
            if (curScreen == dsfScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = dsfScreen.get();
            curScreen->isVisible(true);
            break;

        case 'd':
            if (curScreen == dataTypesScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = dataTypesScreen.get();
            curScreen->isVisible(true);
            break;

        case 'i':
            if (curScreen == traceInfosScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = traceInfosScreen.get();
            curScreen->isVisible(true);
            break;

        case 'h':
        case 'H':
            if (curScreen == helpScreen.get()) {
                break;
            }

            prevScreen = curScreen;
            curScreen->isVisible(false);
            curScreen = helpScreen.get();
            curScreen->isVisible(true);
            break;

        case 'q':
            if (curScreen == inspectScreen.get()) {
                break;
            } else if (curScreen == helpScreen.get()) {
                curScreen->isVisible(false);
                curScreen = prevScreen;
                curScreen->isVisible(true);
            } else {
                curScreen->isVisible(false);
                curScreen = inspectScreen.get();
                curScreen->isVisible(true);
            }

            break;

        case KEY_F(10):
        case 'Q':
            done = true;
            break;

        default:
            const auto reaction = curScreen->handleKey(ch);

            switch (reaction) {
            case KeyHandlingReaction::RETURN_TO_INSPECT:
                curScreen->isVisible(false);
                curScreen = inspectScreen.get();
                curScreen->isVisible(true);
                break;

            case KeyHandlingReaction::RETURN_TO_PACKETS:
                curScreen->isVisible(false);
                curScreen = packetsScreen.get();
                curScreen->isVisible(true);
                break;

            case KeyHandlingReaction::CONTINUE:
                break;
            }
        }

        if (renderStatus) {
            statusView->refresh();
        }

        doupdate();
    }

    return true;
}

bool startInteractive(const Config& cfg)
{
    if (!init()) {
        utils::error() << "Cannot initialize screen and signal handling.\n";
        return false;
    }

    /*
     * Initial refresh() because getch() implicitly calls refresh(),
     * which dumps stdscr the first time, effectively clearing the screen.
     */
    refresh();

    bool res;

    try {
        res = tryStartInteractive(cfg);
    } catch (const std::exception& ex) {
        finiScreen();
        utils::error() << "Unhandled exception: " << ex.what() << std::endl;
        return false;
    }

    finiScreen();
    return res;
}

} // namespace jacques
