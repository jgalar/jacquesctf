/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-segment.hpp"

namespace jacques {

DataSegment::DataSegment(const Index offsetInPacketBits, const DataSize& size) :
    _offsetInPacketBits {offsetInPacketBits},
    _size {size}
{
}

} // namespace jacques
