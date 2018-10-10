/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILE_STATE_HPP
#define _JACQUES_DATA_STREAM_FILE_STATE_HPP

#include <vector>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <yactfr/memory-mapped-file-view-factory.hpp>

#include "aliases.hpp"
#include "timestamp.hpp"
#include "data-stream-file.hpp"
#include "search-parser.hpp"
#include "packet.hpp"
#include "metadata.hpp"
#include "lru-cache.hpp"
#include "packet-checkpoints-build-listener.hpp"

namespace jacques {

class State;

class DataStreamFileState
{
public:
    explicit DataStreamFileState(State& state,
                                 const boost::filesystem::path& path,
                                 std::shared_ptr<const Metadata> metadata,
                                 std::shared_ptr<PacketCheckpointsBuildListener> packetCheckpointsBuildListener);
    void gotoOffsetBits(Index offsetBits);
    void gotoOffsetBytes(Index offsetBytes);
    void gotoPacket(Index index);
    void gotoPreviousPacket();
    void gotoNextPacket();
    bool search(const SearchQuery& query);
    void analyzeAllPackets(PacketCheckpointsBuildListener& buildListener);

    DataStreamFile& dataStreamFile()
    {
        return _dataStreamFile;
    }

    const DataStreamFile& dataStreamFile() const
    {
        return _dataStreamFile;
    }

    bool hasActivePacket() const noexcept
    {
        return _activePacket != nullptr;
    }

    Packet& activePacket()
    {
        if (!_activePacket) {
            _activePacket = this->_packet(_activePacketIndex,
                                          *_packetCheckpointsBuildListener);
        }

        return *_activePacket;
    }

    Index activePacketIndex() const
    {
        return _activePacketIndex;
    }

    Index curOffsetInPacketBits() const
    {
        return _curOffsetInPacketBits;
    }

    boost::optional<Index> curEventRecordIndex()
    {
        return this->activePacket().eventRecordIndexInPacketAtOffsetInPacketBits(_curOffsetInPacketBits);
    }

    const Metadata& metadata() const
    {
        return *_metadata;
    }

private:
    Packet::SP _packet(Index index, PacketCheckpointsBuildListener& buildListener);
    void _gotoPacket(Index index);

private:
    State *_state;
    Packet::SP _activePacket;
    Index _activePacketIndex;
    Index _curOffsetInPacketBits = 0;
    std::shared_ptr<const Metadata> _metadata;
    std::shared_ptr<PacketCheckpointsBuildListener> _packetCheckpointsBuildListener;
    std::shared_ptr<yactfr::MemoryMappedFileViewFactory> _factory;
    yactfr::PacketSequence _seq;
    DataStreamFile _dataStreamFile;
    LruCache<Index, Packet::SP> _packetCache;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_STATE_HPP
