/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <algorithm>
#include <numeric>
#include <cinttypes>
#include <cstdio>
#include <curses.h>
#include <signal.h>
#include <unistd.h>

#include "../stylist.hpp"
#include "pkt-region-info-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"
#include "data/content-pkt-region.hpp"
#include "data/padding-pkt-region.hpp"
#include "data/error-pkt-region.hpp"

namespace jacques {

PktRegionInfoView::PktRegionInfoView(const Rect& rect, const Stylist& stylist,
                                     InspectCmdState& appState) :
    View {rect, "Packet region info", DecorationStyle::BORDERLESS, stylist},
    _appState {&appState},
    _appStateObserverGuard {appState, *this}
{
    this->_setMaxDtPathSizes();
}

void PktRegionInfoView::_appStateChanged(Message)
{
    this->redraw();
}

namespace {

const char *scopeStr(const yactfr::Scope scope) noexcept
{
    switch (scope) {
    case yactfr::Scope::PACKET_HEADER:
        return "PH";

    case yactfr::Scope::PACKET_CONTEXT:
        return "PC";

    case yactfr::Scope::EVENT_RECORD_HEADER:
        return "ERH";

    case yactfr::Scope::EVENT_RECORD_COMMON_CONTEXT:
        return "ERCC";

    case yactfr::Scope::EVENT_RECORD_SPECIFIC_CONTEXT:
        return "ERSC";

    case yactfr::Scope::EVENT_RECORD_PAYLOAD:
        return "ERP";

    default:
        std::abort();
    }
}

class DtPathItemStrVisitor final :
    public boost::static_visitor<std::string>
{
public:
    template <typename ItemT>
    std::string operator()(const ItemT& item) const
    {
        return this->_itemStr(item);
    }

private:
    std::string _itemStr(const DtPath::StructMemberItem& item) const
    {
        return utils::escapeStr(item.name);
    }

    std::string _itemStr(const DtPath::VarOptItem& item) const
    {
        if (item.name) {
            return std::string {'<'} + utils::escapeStr(*item.name) + '>';
        } else {
            return std::string {'<'} + std::to_string(item.index) + '>';
        }
    }

    std::string _itemStr(const DtPath::CurArrayElemItem&) const
    {
        return "<%>";
    }

    std::string _itemStr(const DtPath::CurOptDataItem&) const
    {
        return "<%>";
    }
};

} // namespace

std::string dtPathItemStr(const DtPath::Item& item)
{
    return boost::apply_visitor(DtPathItemStrVisitor {}, item);
}

void PktRegionInfoView::_safePrintScope(const yactfr::Scope scope)
{
    this->_safePrint(scopeStr(scope));
}

void PktRegionInfoView::_redrawContent()
{
    // clear
    this->_stylist().pktRegionInfoViewStd(*this);
    this->_clearRect();

    const auto pktRegion = _appState->curPktRegion();

    if (!pktRegion) {
        return;
    }

    const ContentPktRegion *cPktRegion = nullptr;
    bool isError = false;

    this->_moveCursor({0, 0});

    if ((cPktRegion = dynamic_cast<const ContentPktRegion *>(pktRegion))) {
        // path
        const auto& path = _appState->metadata().dtPath(cPktRegion->dt());

        if (path.items().empty()) {
            this->_stylist().pktRegionInfoViewStd(*this, true);
        }

        this->_safePrintScope(path.scope());

        if (path.items().empty()) {
            this->_stylist().pktRegionInfoViewStd(*this);
        }

        for (auto it = path.items().begin(); it != path.items().end(); ++it) {
            this->_safePrint("/");

            if (it == path.items().end() - 1) {
                this->_stylist().pktRegionInfoViewStd(*this, true);
            }

            this->_safePrint("%s", dtPathItemStr(*it).c_str());
        }
    } else if (const auto sPktRegion = dynamic_cast<const PaddingPktRegion *>(pktRegion)) {
        if (pktRegion->scope()) {
            this->_stylist().pktRegionInfoViewStd(*this);
            this->_safePrintScope(pktRegion->scope()->scope());
            this->_print(" ");
        }

        this->_stylist().pktRegionInfoViewStd(*this, true);
        this->_print("PADDING");
    } else if (const auto sPktRegion = dynamic_cast<const ErrorPktRegion *>(pktRegion)) {
        this->_stylist().pktRegionInfoViewStd(*this, true);
        this->_print("ERROR");
        isError = true;
    }

    // size
    this->_stylist().pktRegionInfoViewStd(*this, false);

    const auto pathWidth = _maxDtPathSizes[&_appState->trace()];
    const auto str = utils::sepNumber(pktRegion->segment().len()->bits(), ',');

    this->_safeMoveAndPrint({
        pathWidth + 4 + this->_curMaxOffsetSize() - 2 - str.size(), 0
    }, "%s b", str.c_str());

    // byte order
    if (pktRegion->segment().bo()) {
        this->_safePrint("    ");

        if (*pktRegion->segment().bo() == yactfr::ByteOrder::BIG) {
            this->_safePrint("BE");
        } else {
            this->_safePrint("LE");
        }
    } else {
        this->_safePrint("      ");
    }

    // value
    if (cPktRegion && cPktRegion->val()) {
        const auto& varVal = *cPktRegion->val();

        this->_safePrint("    ");
        this->_stylist().pktRegionInfoViewVal(*this);

        if (const auto val = boost::get<bool>(&varVal)) {
            this->_safePrint("%s", *val ? "true" : "false");
        } else if (const auto val = boost::get<long long>(&varVal)) {
            this->_safePrint("%s", utils::sepNumber(*val, ',').c_str());
        } else if (const auto val = boost::get<unsigned long long>(&varVal)) {
            const auto prefDispBase = [cPktRegion] {
                if (cPktRegion->dt().isFixedLengthIntegerType()) {
                    return cPktRegion->dt().asFixedLengthIntegerType().preferredDisplayBase();
                } else {
                    assert(cPktRegion->dt().isVariableLengthIntegerType());
                    return cPktRegion->dt().asVariableLengthIntegerType().preferredDisplayBase();
                }
            }();

            std::string intFmt;

            switch (prefDispBase) {
            case yactfr::DisplayBase::OCTAL:
                intFmt = "0%" PRIo64;
                break;

            case yactfr::DisplayBase::HEXADECIMAL:
                intFmt = "0x%" PRIx64;
                break;

            default:
                break;
            }

            if (intFmt.empty()) {
                this->_safePrint("%s", utils::sepNumber(*val, ',').c_str());
            } else {
                this->_safePrint(intFmt.c_str(), *val);
            }
        } else if (const auto val = boost::get<double>(&varVal)) {
            this->_safePrint("%f", *val);
        } else if (const auto val = boost::get<std::string>(&varVal)) {
            this->_safePrint("%s", utils::escapeStr(*val).c_str());
        }
    } else if (isError) {
        const auto& error = _appState->activePktState().pkt().error();

        assert(error);
        this->_safePrint("    ");
        this->_stylist().pktRegionInfoViewError(*this);
        this->_safePrint("%s", utils::escapeStr(error->decodingError().what()).c_str());
    }
}

void PktRegionInfoView::_setMaxDtPathSize(const Trace& trace)
{
    const auto accFunc = [](const auto total, auto& item) {
        return total + dtPathItemStr(item).size();
    };

    const auto totalDtPathItSizeFunc = [&accFunc](auto& dtDtPathPair) {
        /*
         * Adding `dtDtPathPair.second.items().size()` for the
         * separators and 4 for the scope name.
         */
        return std::accumulate(dtDtPathPair.second.items().begin(),
                               dtDtPathPair.second.items().end(), 0ULL, accFunc) +
               dtDtPathPair.second.items().size() + 4;
    };

    const auto maxDtPathIt = std::max_element(trace.metadata().dtPaths().begin(),
                                              trace.metadata().dtPaths().end(),
                                              [&totalDtPathItSizeFunc](const auto& pairA,
                                                                       const auto& pairB) {
        return totalDtPathItSizeFunc(pairA) < totalDtPathItSizeFunc(pairB);
    });

    _maxDtPathSizes.insert(std::make_pair(&trace,
                                          maxDtPathIt == trace.metadata().dtPaths().end() ? 0ULL :
                                          totalDtPathItSizeFunc(*maxDtPathIt)));
}

void PktRegionInfoView::_setMaxDtPathSizes()
{
    for (const auto& dsfState : _appState->dsFileStates()) {
        auto& trace = dsfState->trace();

        if (_maxDtPathSizes.find(&trace) == _maxDtPathSizes.end()) {
            this->_setMaxDtPathSize(trace);
        }
    }
}

Size PktRegionInfoView::_curMaxOffsetSize()
{
    assert(_appState->hasActivePktState());

    const auto& pkt = _appState->activePktState().pkt();
    const auto it = _maxOffsetSizes.find(&pkt);

    if (it == _maxOffsetSizes.end()) {
        const auto str = utils::sepNumber(pkt.indexEntry().effectiveTotalLen().bits());
        const auto size = str.size();

        _maxOffsetSizes[&pkt] = size;
        return size;
    } else {
        return it->second;
    }
}

} // namespace jacques
