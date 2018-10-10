/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_METADATA_HPP
#define _JACQUES_METADATA_HPP

#include <memory>
#include <unordered_map>
#include <yactfr/metadata/metadata-stream.hpp>
#include <yactfr/metadata/fwd.hpp>
#include <yactfr/metadata/trace-type.hpp>
#include <yactfr/metadata/invalid-metadata.hpp>
#include <yactfr/metadata/invalid-metadata-stream.hpp>
#include <yactfr/metadata/metadata-parse-error.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "aliases.hpp"

namespace jacques {

class Metadata
{
public:
    using DataTypeParentMap = std::unordered_map<const yactfr::DataType *,
                                                 const yactfr::DataType *>;
    using DataTypeScopeMap = std::unordered_map<const yactfr::DataType *,
                                                yactfr::Scope>;

public:
    explicit Metadata(const boost::filesystem::path& path);

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

    const boost::optional<Size>& streamPacketCount() const noexcept
    {
        return _stream.packetCount;
    }

    const boost::optional<unsigned int>& streamMajorVersion() const noexcept
    {
        return _stream.majorVersion;
    }

    const boost::optional<unsigned int>& streamMinorVersion() const noexcept
    {
        return _stream.minorVersion;
    }

    const boost::optional<yactfr::ByteOrder>& streamByteOrder() const noexcept
    {
        return _stream.byteOrder;
    }

    const boost::optional<boost::uuids::uuid>& streamUuid() const noexcept
    {
        return _stream.uuid;
    }

    const boost::optional<yactfr::InvalidMetadataStream>& invalidStreamError() const noexcept
    {
        return _invalidStreamError;
    }

    const boost::optional<yactfr::InvalidMetadata>& invalidMetadataError() const noexcept
    {
        return _invalidMetadataError;
    }

    const boost::optional<yactfr::MetadataParseError>& parseError() const noexcept
    {
        return _parseError;
    }

    yactfr::TraceType::SP traceType() const noexcept
    {
        return _traceType;
    }

    bool isCorrelatable() const noexcept
    {
        return _isCorrelatable;
    }

    const yactfr::DataType *dataTypeParent(const yactfr::DataType& dataType) const;
    yactfr::Scope dataTypeScope(const yactfr::DataType& dataType) const;
    bool dataTypeIsScopeRoot(const yactfr::DataType& dataType) const;

private:
    void _setDataTypeParents();
    void _setIsCorrelatable();

private:
    const boost::filesystem::path _path;

    struct {
        boost::optional<Size> packetCount;
        boost::optional<unsigned int> majorVersion;
        boost::optional<unsigned int> minorVersion;
        boost::optional<yactfr::ByteOrder> byteOrder;
        boost::optional<boost::uuids::uuid> uuid;
    } _stream;

    boost::optional<yactfr::InvalidMetadataStream> _invalidStreamError;
    boost::optional<yactfr::InvalidMetadata> _invalidMetadataError;
    boost::optional<yactfr::MetadataParseError> _parseError;
    yactfr::TraceType::SP _traceType;
    DataTypeParentMap _dataTypeParents;
    DataTypeScopeMap _dataTypeScopes;
    bool _isCorrelatable = false;
};

} // namespace jacques

#endif // _JACQUES_METADATA_HPP
