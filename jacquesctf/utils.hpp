/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_UTILS_HPP
#define _JACQUES_UTILS_HPP

#include <string>
#include <iostream>
#include <utility>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <yactfr/metadata/metadata-parse-error.hpp>
#include <yactfr/metadata/invalid-metadata.hpp>
#include <yactfr/metadata/invalid-metadata-stream.hpp>

#include "data/metadata-error.hpp"
#include "io-error.hpp"
#include "cmd-error.hpp"
#include "aliases.hpp"
#include "cfg.hpp"

namespace jacques {
namespace utils {

/*
 * Formats the path `path`, considering that the total length of the
 * output must not exceed `maxLen`.
 *
 * The first element of the returned pair is the directory name
 * (possibly with ellipses if, due to `maxLen`, it's incomplete), and
 * the second element is the file name.
 *
 * The lengths of both returned elements plus one (to add a path
 * separator between them) is always less than or equal to `maxLen`.
 */
std::pair<std::string, std::string> formatPath(const boost::filesystem::path& path, Size maxLen);

/*
 * Normalizes a globbing pattern, that is, removes consecutive `*`
 * characters.
 */
std::string normalizeGlobPattern(const std::string& pattern);

/*
 * Returns whether or not `candidate` matches the globbing pattern
 * `pattern`. Only the `*` special character works as of this version.
 */
bool globMatch(const std::string& pattern, const std::string& candidate);

/*
 * Does pretty much what the fold(1) command does.
 */
std::string wrapText(const std::string& text, Size lineLen);

/*
 * Escapes a string, replacing special characters with typical escape
 * sequences.
 */
std::string escapeStr(const std::string& str);

/*
 * Creates a string which has "thousands separators" from a value,
 * like so:
 *
 *     1827912   -> 1 827 912
 *     -21843812 -> -21,843,812
 */
std::string sepNumber(long long val, char sep = ' ');
std::string sepNumber(unsigned long long val, char sep = ' ');

/*
 * Used as such:
 *
 *     error() << "Cannot something something" << std::endl;
 *
 * I hate this. Let's change it some day.
 */
static inline std::ostream& error()
{
    std::cerr << "ERROR: ";
    return std::cerr;
}

enum class LenFmtMode
{
    FULL_FLOOR,
    FULL_FLOOR_WITH_EXTRA_BITS,
    BYTES_FLOOR,
    BYTES_FLOOR_WITH_EXTRA_BITS,
    BITS,
};

std::pair<std::string, std::string> formatLen(Size lenBits,
                                              LenFmtMode fmtMode = LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
                                              const boost::optional<char>& sep = boost::none);

std::pair<std::string, std::string> formatNs(long long ns,
                                             const boost::optional<char>& sep = boost::none);

void printMetadataParseError(std::ostream& os, const std::string& path,
                             const yactfr::MetadataParseError& error);

namespace internal {

static inline void maybeAppendPeriod(std::string& str)
{
    if (str.empty()) {
        return;
    }

    if (str.back() != '.') {
        str += '.';
    }
}

static inline std::string formatMetadataParseError(const std::string& path,
                                                   const yactfr::MetadataParseError& error)
{
    std::ostringstream ss;

    for (auto it = error.errorMessages().rbegin(); it != error.errorMessages().rend(); ++it) {
        auto& msg = *it;

        ss << path << ":" << msg.location().natLineNumber() <<
              ":" << msg.location().natColNumber() <<
              ": " << msg.message();

        if (it < error.errorMessages().rend() - 1) {
            ss << std::endl;
        }
    }

    return ss.str();
}

} // namespace internal

template <typename FuncT>
boost::optional<std::string> tryFunc(FuncT&& func)
{
    std::ostringstream ss;

    try {
        func();
    } catch (const MetadataError<yactfr::InvalidMetadataStream>& exc) {
        ss << "Metadata error: `" << exc.path().string() <<
              "`: invalid metadata stream: " << exc.what();
    } catch (const MetadataError<yactfr::InvalidMetadata>& exc) {
        ss << "Metadata error: `" << exc.path().string() <<
              "`: invalid metadata: " << exc.what();
    } catch (const MetadataError<yactfr::MetadataParseError>& exc) {
        ss << internal::formatMetadataParseError(exc.path().string(), exc.subError());
    } catch (const CliError& exc) {
        ss << "Command-line error: " << exc.what();
    } catch (const boost::filesystem::filesystem_error& exc) {
        ss << "File system error: " << exc.what();
    } catch (const IOError& exc) {
        ss << "I/O error: " << exc.what();
    } catch (const CmdError& exc) {
        ss << "Command error: " << exc.what();
    } catch (const std::exception& exc) {
        ss << exc.what();
    } catch (...) {
        ss << "Unknown exception";
    }

    auto str = ss.str();

    if (str.empty()) {
        return boost::none;
    }

    internal::maybeAppendPeriod(str);
    return str;
}

} // namespace utils
} // namespace jacques

#endif // _JACQUES_UTILS_HPP
