#include "game/fixed_count_resource_loader.hpp"

#include "core/rom_identity.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace oasis::game {
namespace {

constexpr std::string_view kManifestSha256 = "33b2faba97ae2b10a1822c77c268cd0531c35add2ac67f39ef9931cfe1bedfc3";
constexpr std::string_view kCensusSha256 = "99b4dc0fdb37b014e21775409e67cc72da593184d8279508b24b08ea0952cbc5";
constexpr std::string_view kHeader =
    "ROW_ID\tSTREAM_ID\tROM_START\tROM_END\tBYTE_LENGTH\tCOUNT_VALUE"
    "\tRECORD_COUNT\tRAW_SHA256\tALIASES\tCANONICAL_STREAM_ID"
    "\tSOURCE_CENSUS_HASH\tROM_SHA256";

struct Stream {
    std::string id;
    std::uint32_t start{};
    std::uint32_t end{};
    std::size_t length{};
    std::uint16_t count{};
    std::size_t records{};
    std::string sha;
    std::string aliases;
};

struct Catalog {
    std::map<std::uint32_t, Stream> rows;
    std::map<std::string, Stream, std::less<>> streams;
};

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open fixed-count manifest");
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) throw std::runtime_error("cannot read fixed-count manifest");
    return contents.str();
}

std::vector<std::string_view> split(std::string_view line, char separator) {
    std::vector<std::string_view> result;
    std::size_t start = 0;
    for (;;) {
        const auto end = line.find(separator, start);
        result.push_back(line.substr(start, end == std::string_view::npos
                                                 ? line.size() - start : end - start));
        if (end == std::string_view::npos) return result;
        start = end + 1U;
    }
}

std::uint32_t number(std::string_view text) {
    std::uint32_t value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::runtime_error("fixed-count manifest has invalid number");
    return value;
}

bool valid_sha(std::string_view value) {
    if (value.size() != 64U) return false;
    for (char ch : value) if (!((ch >= '0' && ch <= '9') ||
                                 (ch >= 'a' && ch <= 'f'))) return false;
    return true;
}

Catalog parse_catalog(const std::filesystem::path& path) {
    const auto text = read_text(path);
    const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    if (oasis::calculate_sha256(bytes) != kManifestSha256)
        throw std::runtime_error("fixed-count manifest SHA-256 mismatch");
    std::istringstream input(text);
    std::string line;
    auto expected = [&input, &line](std::string_view value) {
        if (!std::getline(input, line) || line != value)
            throw std::runtime_error("fixed-count manifest identity mismatch");
    };
    expected("OASIS_M68K_FIXED_COUNT_RECORDS_MANIFEST_V1");
    expected(std::string("ROM_SHA256\t") + kFixedCountRomSha256);
    expected(std::string("SOURCE_CENSUS_SHA256\t") + std::string(kCensusSha256));
    expected("ROW_COUNT\t96");
    expected("STREAM_COUNT\t78");
    expected(kHeader);
    Catalog catalog;
    while (std::getline(input, line)) {
        auto fields = split(line, '\t');
        if (fields.size() != 12U) throw std::runtime_error("fixed-count manifest row width mismatch");
        const auto row = number(fields[0]);
        Stream stream{std::string(fields[1]), number(fields[2]), number(fields[3]),
                      number(fields[4]), static_cast<std::uint16_t>(number(fields[5])),
                      number(fields[6]), std::string(fields[7]), std::string(fields[8])};
        if (row >= 99U || fields[9] != stream.id || fields[10] != kCensusSha256 ||
            fields[11] != kFixedCountRomSha256 || !valid_sha(stream.sha) ||
            stream.id != "STREAM_" + [&] { char out[7]{}; std::snprintf(out, sizeof(out), "%06X", stream.start); return std::string(out); }() ||
            stream.end <= stream.start || stream.end - stream.start != stream.length ||
            stream.length != 2U + 6U * (static_cast<std::size_t>(stream.count) + 1U) ||
            stream.records != static_cast<std::size_t>(stream.count) + 1U)
            throw std::runtime_error("fixed-count manifest row violates pinned extent");
        if (!catalog.rows.emplace(row, stream).second)
            throw std::runtime_error("duplicate fixed-count table row");
        const auto [it, inserted] = catalog.streams.emplace(stream.id, stream);
        if (!inserted && (it->second.start != stream.start || it->second.end != stream.end ||
            it->second.length != stream.length || it->second.count != stream.count ||
            it->second.records != stream.records || it->second.sha != stream.sha ||
            it->second.aliases != stream.aliases))
            throw std::runtime_error("aliased fixed-count stream metadata conflicts");
    }
    if (catalog.rows.size() != kFixedCountActiveRowCount ||
        catalog.streams.size() != kFixedCountUniqueStreamCount)
        throw std::runtime_error("fixed-count manifest family cardinality mismatch");
    for (const auto& [row, stream] : catalog.rows) {
        const auto aliases = split(stream.aliases, ',');
        std::set<std::string_view> unique_aliases(aliases.begin(), aliases.end());
        if (unique_aliases.size() != aliases.size())
            throw std::runtime_error("duplicate row in fixed-count alias group");
        if (std::find(aliases.begin(), aliases.end(), std::to_string(row)) == aliases.end())
            throw std::runtime_error("fixed-count alias topology omits row");
        for (auto alias : aliases) {
            const auto found = catalog.rows.find(number(alias));
            if (found == catalog.rows.end() || found->second.id != stream.id)
                throw std::runtime_error("fixed-count alias topology is not reciprocal");
        }
    }
    return catalog;
}

std::vector<std::uint8_t> read_file(const std::filesystem::path& path,
                                    std::size_t size) {
    std::error_code error;
    const auto actual = std::filesystem::file_size(path, error);
    if (error) throw std::runtime_error("MISSING_FILE: " + error.message());
    if (actual != size) throw std::runtime_error("WRONG_SIZE");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("CANNOT_OPEN_FILE");
    std::vector<std::uint8_t> data(size);
    if (size && !input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size)))
        throw std::runtime_error("TRUNCATED_FILE");
    return data;
}

std::vector<FixedCountRecord> parse_records(std::span<const std::uint8_t> data,
                                            std::size_t expected_count) {
    if (data.size() < 2U) throw std::runtime_error("TRUNCATED_COUNT_WORD");
    const auto count = static_cast<std::uint16_t>((data[0] << 8U) | data[1]);
    const auto records = static_cast<std::size_t>(count) + 1U;
    const auto extent = 2U + 6U * records;
    if (records != expected_count || extent != data.size())
        throw std::runtime_error("COUNT_EXTENT_MISMATCH");
    std::vector<FixedCountRecord> result;
    result.reserve(records);
    for (std::size_t offset = 2U; offset < extent; offset += 6U) {
        auto word = [&data, offset](std::size_t field) {
            const auto at = offset + field * 2U;
            return static_cast<std::uint16_t>((data[at] << 8U) | data[at + 1U]);
        };
        result.push_back({word(0U), word(1U), word(2U)});
    }
    return result;
}

} // namespace

LoadedFixedCountStream load_fixed_count_stream(
    std::span<const std::uint8_t> rom, std::uint32_t row_id,
    const std::filesystem::path& manifest,
    const std::filesystem::path& override_directory) {
    if (rom.size() != 0x300000U || oasis::calculate_sha256(rom) != kFixedCountRomSha256)
        throw std::runtime_error("fixed-count streams require canonical USA ROM");
    const auto catalog = parse_catalog(manifest);
    const auto row = catalog.rows.find(row_id);
    if (row == catalog.rows.end()) throw std::runtime_error("UNKNOWN_TABLE_ROW_ID");
    const auto& stream = catalog.streams.at(row->second.id);
    std::vector<std::uint8_t> payload;
    std::string rejection;
    std::filesystem::path external = override_directory / (stream.id + ".bin");
    bool active = false;
    try {
        payload = read_file(external, stream.length);
        (void)parse_records(payload, stream.records);
        if (oasis::calculate_sha256(payload) != stream.sha)
            throw std::runtime_error("RAW_SHA256_MISMATCH");
        active = true;
    } catch (const std::exception& error) {
        rejection = error.what();
        payload.assign(rom.begin() + stream.start, rom.begin() + stream.end);
    }
    const auto parsed = parse_records(payload, stream.records);
    const auto raw_sha = oasis::calculate_sha256(payload);
    if (raw_sha != stream.sha) throw std::runtime_error("ROM_STREAM_SHA256_MISMATCH");
    return {row_id, stream.id, stream.start, stream.end, stream.length,
            stream.count, stream.records, raw_sha, std::move(payload), parsed,
            active ? "EXTERNAL_FILE_ACTIVE_EXACT_COPY" : "ROM_BACKED",
            rejection, active ? external : std::filesystem::path{},
            active ? 0U : stream.length, active};
}

} // namespace oasis::game
