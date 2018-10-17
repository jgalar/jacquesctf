/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_STATE_HPP
#define _JACQUES_PACKET_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "packet.hpp"
#include "aliases.hpp"

namespace jacques {

class State;

class PacketState
{
public:
    explicit PacketState(State& state, Packet& packet);
    void gotoPreviousEventRecord(Size count = 1);
    void gotoNextEventRecord(Size count = 1);
    void gotoPreviousDataRegion();
    void gotoNextDataRegion();
    void gotoPacketContext();
    void gotoLastDataRegion();
    void gotoDataRegionAtOffsetInPacketBits(Index offsetBits);

    void gotoDataRegionAtOffsetInPacketBits(const DataRegion& region)
    {
        this->gotoDataRegionAtOffsetInPacketBits(region.segment().offsetInPacketBits());
    }

    Packet& packet() noexcept
    {
        return *_packet;
    }

    const PacketIndexEntry& packetIndexEntry() const noexcept
    {
        return _packet->indexEntry();
    }

    Index curOffsetInPacketBits() const noexcept
    {
        return _curOffsetInPacketBits;
    }

    const EventRecord *currentEventRecord()
    {
        const auto& dataRegion = _packet->dataRegionAtOffsetInPacketBits(_curOffsetInPacketBits);
        const auto& scope = dataRegion.scope();

        if (!scope) {
            return nullptr;
        }

        // can return `nullptr`
        return scope->eventRecord();
    }

    const DataRegion& currentDataRegion()
    {
        return _packet->dataRegionAtOffsetInPacketBits(_curOffsetInPacketBits);
    }

    State& state() noexcept
    {
        return *_state;
    }

    const State& state() const noexcept
    {
        return *_state;
    }

private:
    State * const _state;
    Packet * const _packet;
    Index _curOffsetInPacketBits = 0;
};

} // namespace jacques

#endif // _JACQUES_PACKET_STATE_HPP
