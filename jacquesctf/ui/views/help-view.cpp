/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <algorithm>
#include <boost/variant/get.hpp>

#include "help-view.hpp"
#include "stylist.hpp"

namespace jacques {

static constexpr Size keyWidth = 15;

HelpView::HelpView(const Rectangle& rect,
                   std::shared_ptr<const Stylist> stylist) :
    ScrollView {rect, "Help", DecorationStyle::BORDERS, stylist}
{
    this->_buildRows();
    this->_rowCount(_rows.size());
    this->_drawRows();
}

void HelpView::_buildRows()
{
    _rows = {
        _TextRow {"     _____"},
        _TextRow {"    /\\___ \\"},
        _TextRow {"    \\/__/\\ \\     __      ___     __   __  __     __    ____"},
        _TextRow {"       _\\ \\ \\  /'__`\\   /'___\\ /'__`\\/\\ \\/\\ \\  /'__`\\ /',__\\  C"},
        _TextRow {"      /\\ \\_\\ \\/\\ \\L\\.\\_/\\ \\__//\\ \\L\\ \\ \\ \\_\\ \\/\\  __//\\__, `\\  T"},
        _TextRow {"      \\ \\____/\\ \\__/.\\_\\ \\____\\ \\___, \\ \\____/\\ \\____\\/\\____/   F"},
        _TextRow {"       \\/___/  \\/__/\\/_/\\/____/\\/___/\\ \\/___/  \\/____/\\/___/"},
        _TextRow {"                                    \\ \\_\\ " JACQUES_VERSION " \a{eepp.ca}"},
        _TextRow {"                                     \\/_/"},
        _SectionRow {"General keys"},
        _KeyRow {"f", "Go to `Data stream files` screen"},
        _KeyRow {"p", "Go to `Packets` screen"},
        _KeyRow {"d", "Go to `Data types` screen"},
        _KeyRow {"i", "Go to `Trace info` screen"},
        _KeyRow {"h, H", "Go to `Help` screen"},
        _KeyRow {"r", "Hard refresh screen"},
        _KeyRow {"q", "Quit current screen"},
        _KeyRow {"F10, Q", "Quit program"},
        _EmptyRow {},
        _SectionRow {"`Packet inspection` screen: frame keys"},
        _KeyRow {"e", "Show/hide `Event records` frame"},
        _KeyRow {"i", "Show/hide `Info` frame"},
        _EmptyRow {},
        _SectionRow {"`Packet inspection` screen: navigation keys"},
        _KeyRow {"Left", "Go to previous datum"},
        _KeyRow {"Right", "Go to next datum"},
        _KeyRow {"Up", "Go to datum on previous row"},
        _KeyRow {"Down", "Go to datum on next row"},
        _KeyRow {"Pg up", "Go to previous content page"},
        _KeyRow {"Pg down", "Go to next content page"},
        _KeyRow {"/, g", "Search/go to (see syntax below)"},
        _KeyRow {"n", "Repeat previous search"},
        _KeyRow {"N", "Repeat previous search in reverse direction"},
        _KeyRow {"Home", "Go to first packet datum"},
        _KeyRow {"c", "Go to first packet context datum"},
        _KeyRow {"End", "Go to last packet datum"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _KeyRow {"F5", "Go to previous packet"},
        _KeyRow {"F6", "Go to next packet"},
        _KeyRow {"-", "Go to previous event record"},
        _KeyRow {"+, =, Space", "Go to next event record"},
        _KeyRow {"F7", "Skip a few event records backward"},
        _KeyRow {"F8", "Skip a few event records forward"},
        _EmptyRow {},
        _SectionRow {"`Packet inspection` screen: display keys"},
        _KeyRow {"x", "Toggle: show bytes in binary or hexadecimal"},
        _KeyRow {"t", "Toggle: show timestamp's full date, ns, or cycles"},
        _EmptyRow {},
        _SectionRow {"`Data stream files` screen keys"},
        _KeyRow {"Up", "Select previous row"},
        _KeyRow {"Down", "Select next row"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"c", "Center selected row"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _KeyRow {"t", "Cycle timestamp columns's format"},
        _KeyRow {"s", "Cycle size columns's format"},
        _KeyRow {"Enter", "Accept selection"},
        _KeyRow {"q", "Discard selection and return to `Packet inspection` screen"},
        _EmptyRow {},
        _SectionRow {"`Packets` screen keys"},
        _KeyRow {"Up", "Select previous row"},
        _KeyRow {"Down", "Select next row"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"c", "Center selected row"},
        _KeyRow {"F3", "Go to previous data stream file"},
        _KeyRow {"F4", "Go to next data stream file"},
        _KeyRow {"F5", "Go to previous packet"},
        _KeyRow {"F6", "Go to next packet"},
        _KeyRow {"/, g", "Search/go to (see syntax below)"},
        _KeyRow {"n", "Repeat previous search"},
        _KeyRow {"N", "Repeat previous search in reverse direction"},
        _KeyRow {"t", "Cycle timestamp columns's format"},
        _KeyRow {"s", "Cycle size columns's format"},
        _KeyRow {"a", "Analyze all packets"},
        _KeyRow {"Enter", "Accept selection and return to `Packet inspection` screen"},
        _KeyRow {"q", "Discard selection and return to `Packet inspection` screen"},
        _EmptyRow {},
        _SectionRow {"`Data types` screen keys"},
        _KeyRow {"Up", "Select previous row/go up one line"},
        _KeyRow {"Down", "Select next row/go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"Home", "Select first row"},
        _KeyRow {"End", "Select last row"},
        _KeyRow {"c", "Center selected table row"},
        _KeyRow {"/, g", "Search event record type (name or ID, see syntax below)"},
        _KeyRow {"n", "Repeat previous search"},
        _KeyRow {"Tab, Enter", "Focus next frame"},
        _KeyRow {"Left", "Focus left frame"},
        _KeyRow {"Right", "Focus right frame"},
        _KeyRow {"+", "Show/hide tables"},
        _KeyRow {"q", "Return to `Packet inspection` screen"},
        _EmptyRow {},
        _SectionRow {"`Trace info` screen keys"},
        _KeyRow {"Up", "Select previous row/go up one line"},
        _KeyRow {"Down", "Select next row/go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"q", "Return to `Packet inspection` screen"},
        _EmptyRow {},
        _SectionRow {"`Help` screen keys"},
        _KeyRow {"Up", "Go up one line"},
        _KeyRow {"Down", "Go down one line"},
        _KeyRow {"Pg up", "Jump to previous page"},
        _KeyRow {"Pg down", "Jump to next page"},
        _KeyRow {"q", "Return to previous screen"},
        _EmptyRow {},
        _SectionRow {"Search box keys"},
        _KeyRow {"Ctrl+w", "Clear input"},
        _KeyRow {"Enter", "Search if input is not empty, else cancel"},
        _KeyRow {"Ctrl+d", "Cancel"},
        _EmptyRow {},
        _SectionRow {"Search syntax"},
        _TextRow {"X is a constant integer which you can write in decimal, hexadecimal"},
        _TextRow {"(`0x` prefix), or octal (`0` prefix)."},
        _EmptyRow {},
        _SearchSyntaxRow {"Packet at index X within data stream file", "#X"},
        _SearchSyntaxRow {"Packet at relative index X within data stream file", "+#X  -#X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Packet with sequence number X", "##X"},
        _SearchSyntaxRow {"Packet with relative sequence number X", "+##X  -##X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Event record at index X within packet", "###X"},
        _SearchSyntaxRow {"Event record at relative index X within packet", "+###X  -###X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Offset X (bits) within data stream file", ":X"},
        _SearchSyntaxRow {"Relative offset X (bits) within data stream file", "+:X  -:X"},
        _SearchSyntaxRow {"Offset X (bytes) within data stream file", ":$X"},
        _SearchSyntaxRow {"Relative offset X (bytes) within data stream file", "+:$X  -:$X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Offset X (bits) within packet", "X"},
        _SearchSyntaxRow {"Relative offset X (bits) within packet", "+X  -X"},
        _SearchSyntaxRow {"Offset X (bytes) within packet", "$X"},
        _SearchSyntaxRow {"Relative offset X (bytes) within packet", "+$X  -$X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Timestamp X (ns)", "*X  *-X"},
        _SearchSyntaxRow {"Relative timestamp X (ns)", "+*X  -*X"},
        _SearchSyntaxRow {"Timestamp X (cycles)", "**X"},
        _SearchSyntaxRow {"Relative timestamp X (cycles)", "+**X  -**X"},
        _EmptyRow {},
        _SearchSyntaxRow {"Event record with event record type name NAME", "/NAME"},
        _SearchSyntaxRow {"Event record with event record type ID X", "%X"},
    };

    _ssRowFmtPos = 0;
    _longestRowWidth = 0;

    for (const auto& row : _rows) {
        if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            _ssRowFmtPos = std::max(_ssRowFmtPos,
                                    static_cast<Size>(ssRow->descr.size() + 2));
        }
    }

    for (const auto& row : _rows) {
        if (auto sectRow = boost::get<_SectionRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(sectRow->title.size()));
        } else if (auto keyRow = boost::get<_KeyRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(keyWidth + keyRow->descr.size() + 2));
        } else if (auto textRow = boost::get<_TextRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(textRow->line.size()));
        } else if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            _longestRowWidth = std::max(_longestRowWidth,
                                        static_cast<Size>(ssRow->format.size() + _ssRowFmtPos));
        }
    }
}

void HelpView::_drawRows()
{
    this->_stylist().std(*this);
    this->_clearContent();

    const Index startX = (this->contentRect().w - _longestRowWidth) / 2;

    for (Index rowNum = 0; rowNum < this->contentRect().h; ++rowNum) {
        const auto y = rowNum;
        const auto rowIdx = this->_index() + rowNum;

        if (rowIdx >= this->_rowCount()) {
            break;
        }

        const auto& row = _rows[rowIdx];

        if (auto sectRow = boost::get<_SectionRow>(&row)) {
            this->_stylist().helpViewSection(*this);
            this->_moveAndPrint({startX, y}, "%s", sectRow->title.c_str());
        } else if (auto keyRow = boost::get<_KeyRow>(&row)) {
            this->_moveCursor({startX + 2, y});

            for (const auto& ch : keyRow->key) {
                if (ch == ',') {
                    this->_stylist().std(*this);
                } else {
                    this->_stylist().helpViewKey(*this);
                }

                this->_appendChar(ch);
            }

            this->_stylist().std(*this);
            this->_moveAndPrint({startX + keyWidth, y}, "%s",
                                keyRow->descr.c_str());
        } else if (auto textRow = boost::get<_TextRow>(&row)) {
            this->_stylist().std(*this);
            this->_moveCursor({startX, y});
            this->_stylist().std(*this, textRow->bold);

            for (const auto& ch : textRow->line) {
                if (ch == '\a') {
                    this->_stylist().stdDim(*this);
                    continue;
                }

                this->_appendChar(ch);
            }
        } else if (auto ssRow = boost::get<_SearchSyntaxRow>(&row)) {
            this->_stylist().std(*this);
            this->_moveCursor({startX, y});

            for (const auto& ch : ssRow->descr) {
                if (ch == 'X') {
                    this->_stylist().helpViewKey(*this);
                } else {
                    this->_stylist().std(*this);
                }

                this->_appendChar(ch);
            }

            this->_moveCursor({startX + _ssRowFmtPos, y});
            this->_stylist().helpViewKey(*this);

            for (const auto& ch : ssRow->format) {
                if (ch == 'X') {
                    this->_stylist().helpViewKey(*this);
                } else {
                    this->_stylist().std(*this);
                }

                this->_appendChar(ch);
            }
        }
    }
}

} // namespace jacques
