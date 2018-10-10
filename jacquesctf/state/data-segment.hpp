/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_SEGMENT_HPP
#define _JACQUES_DATA_SEGMENT_HPP

#include <boost/operators.hpp>

#include "aliases.hpp"
#include "data-size.hpp"

namespace jacques {

struct DataSegment :
    public boost::totally_ordered<DataSegment>
{
    DataSegment() = default;
    explicit DataSegment(Index offsetInPacketBits, const DataSize& size);

    Index offsetInPacketBits() const noexcept
    {
        return _offsetInPacketBits;
    }

    void offsetInPacketBits(const Index offsetInPacketBits) noexcept
    {
        _offsetInPacketBits = offsetInPacketBits;
    }

    Index offsetInPacketExtraBits() const noexcept
    {
        return _offsetInPacketBits & 7;
    }

    const DataSize& size() const noexcept
    {
        return _size;
    }

    void size(const DataSize& size) noexcept
    {
        _size = size;
    }

    bool operator<(const DataSegment& other)
    {
        return _offsetInPacketBits < other._offsetInPacketBits;
    }

    bool operator==(const DataSegment& other)
    {
        return _offsetInPacketBits == other._offsetInPacketBits;
    }

    Index _offsetInPacketBits = 0;
    DataSize _size = 0;
};

} // namespace jacques

#endif // _JACQUES_DATA_SEGMENT_HPP
