/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cinttypes>
#include <cstdio>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

#include "config.hpp"
#include "help-screen.hpp"
#include "help-view.hpp"
#include "stylist.hpp"
#include "status-view.hpp"
#include "utils.hpp"
#include "active-data-stream-file-changed-message.hpp"
#include "active-packet-changed-message.hpp"

namespace jacques {

StatusView::StatusView(const Rectangle& rect,
                       std::shared_ptr<const Stylist> stylist,
                       std::shared_ptr<State> state) :
    View {rect, "Status", DecorationStyle::BORDERLESS, stylist},
    _state {state},
    _stateObserverGuard {*state, *this}
{
}

void StatusView::_stateChanged(const Message& msg)
{
    if (dynamic_cast<const ActiveDataStreamFileChangedMessage *>(&msg) ||
            dynamic_cast<const ActivePacketChangedMessage *>(&msg)) {
        this->redraw();
    }
}

void StatusView::_redrawContent()
{
    // clear
    this->_stylist().statusViewStd(*this);
    this->_clearRect();

    // packet index
    std::array<char, 32> packetCount;
    std::array<char, 32> curPacket;

    if (_state->activeDataStreamFileState().hasActivePacket()) {
        const auto index = _state->activeDataStreamFileState().activePacket().indexEntry().natIndexInDataStream();

        std::snprintf(curPacket.data(), curPacket.size(), "%s",
                      utils::sepNumber(static_cast<long long>(index), ',').c_str());
    } else {
        std::strcpy(curPacket.data(), "");
    }

    const auto count = _state->activeDataStreamFileState().dataStreamFile().packetCount();

    std::snprintf(packetCount.data(), packetCount.size(), "/%s",
                  utils::sepNumber(static_cast<long long>(count), ',').c_str());

    auto pktInfoPos = Point {
        this->contentRect().w - std::strlen(packetCount.data()) - std::strlen(curPacket.data()) - 1,
        0
    };

    this->_putChar(pktInfoPos, '#');
    this->_stylist().statusViewStd(*this, true);
    ++pktInfoPos.x;
    this->_moveAndPrint(pktInfoPos, "%s", curPacket.data());
    this->_stylist().statusViewStd(*this);
    pktInfoPos.x += std::strlen(curPacket.data());
    this->_moveAndPrint(pktInfoPos, "%s", packetCount.data());

    // packet sequence number
    if (_state->activeDataStreamFileState().hasActivePacket()) {
        const auto seqNum = _state->activeDataStreamFileState().activePacket().indexEntry().seqNum();

        if (seqNum) {
            this->_moveAndPrint({this->contentRect().w - 31, 0}, "##");
            this->_stylist().statusViewStd(*this, true);
            this->_moveAndPrint({this->contentRect().w - 29, 0}, "%llu", *seqNum);
            this->_stylist().statusViewStd(*this);
        }
    }

    const auto& dsFileState = _state->activeDataStreamFileState();
    const auto& path = dsFileState.dataStreamFile().path();
    const auto pathMaxLen = this->contentRect().w - 35;
    std::string dirNameStr, filenameStr;

    std::tie(dirNameStr, filenameStr) = utils::formatPath(path, pathMaxLen);
    this->_moveCursor({0, 0});

    if (!dirNameStr.empty()) {
        this->_print("%s/", dirNameStr.c_str());
    }

    this->_stylist().statusViewFilename(*this);
    this->_print("%s", filenameStr.c_str());
}

} // namespace jacques
