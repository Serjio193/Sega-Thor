#include "game/screen_resource_loader.hpp"

#include "core/rom_identity.hpp"
#include "game/graphics_decompress.hpp"

#include <array>
#include <charconv>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace oasis::game {
namespace {

constexpr std::size_t kRomSize = 0x300000U;
constexpr std::uint32_t kRootTableStart = 0x00C92CU;
constexpr std::uint32_t kRootTableEnd = 0x00C980U;
constexpr std::string_view kManifestHeader =
    "PATH_ID\tROOT_ID\tROOT_ADDRESS\tROOT_SOURCE\tROOT_INDEX"
    "\tRELATIVE_WORD_ADDRESS\tRELATIVE_DISPLACEMENT\tDESCRIPTOR_ID"
    "\tDESCRIPTOR_ADDRESS\tSTREAM_ID\tROM_START\tROM_END"
    "\tCOMPRESSED_SIZE\tCOMPRESSED_SHA256\tDECODED_SIZE\tDECODED_SHA256"
    "\tALIAS_GROUP";

struct StreamEntry {
    std::string id;
    std::uint32_t start{};
    std::uint32_t end{};
    std::size_t compressed_size{};
    std::string compressed_sha;
    std::size_t decoded_size{};
    std::string decoded_sha;
    std::vector<ScreenResourcePath> paths;
};

struct Catalog {
    std::map<std::string, StreamEntry, std::less<>> streams;
};

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open screen resource manifest");
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) throw std::runtime_error("cannot read screen resource manifest");
    return contents.str();
}

std::vector<std::string_view> split_tabs(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    for (;;) {
        const auto end = line.find('\t', start);
        fields.push_back(line.substr(start, end == std::string_view::npos
                                               ? line.size() - start : end - start));
        if (end == std::string_view::npos) return fields;
        start = end + 1U;
    }
}

std::uint32_t parse_number(std::string_view text) {
    int base = 10;
    if (text.starts_with("0x") || text.starts_with("0X")) {
        text.remove_prefix(2U);
        base = 16;
    }
    std::uint32_t value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (text.empty() || result.ec != std::errc{} ||
        result.ptr != text.data() + text.size()) {
        throw std::runtime_error("screen manifest has invalid numeric field");
    }
    return value;
}

bool valid_hash(std::string_view value) {
    if (value.size() != 64U) return false;
    for (const auto ch : value) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return false;
    }
    return true;
}

std::vector<std::uint8_t> read_binary(const std::filesystem::path& path,
                                     std::size_t expected_size) {
    std::error_code error;
    if (std::filesystem::file_size(path, error) != expected_size || error) {
        throw std::runtime_error(error ? "cannot stat external stream"
                                       : "external compressed size mismatch");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open external stream");
    std::vector<std::uint8_t> bytes(expected_size);
    if (!bytes.empty() && !input.read(reinterpret_cast<char*>(bytes.data()),
                                      static_cast<std::streamsize>(bytes.size()))) {
        throw std::runtime_error("cannot read complete external stream");
    }
    return bytes;
}

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4U) {
        throw std::runtime_error("screen root pointer exceeds ROM");
    }
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void validate_rom(std::span<const std::uint8_t> rom) {
    if (rom.size() != kRomSize ||
        oasis::calculate_sha256(rom) != kGfxScreenRootsRomSha256) {
        throw std::runtime_error("screen streams require the canonical USA ROM");
    }
}

Catalog parse_catalog(const std::filesystem::path& path,
                      std::span<const std::uint8_t> rom) {
    const auto text = read_text(path);
    if (oasis::calculate_sha256({reinterpret_cast<const std::uint8_t*>(text.data()),
                                 text.size()}) != kGfxScreenRootsManifestSha256) {
        throw std::runtime_error("screen resource manifest SHA-256 mismatch");
    }
    std::istringstream input(text);
    std::string line;
    auto read_expected = [&input, &line](std::string_view expected) {
        if (!std::getline(input, line) || line != expected) {
            throw std::runtime_error("screen resource manifest identity mismatch");
        }
    };
    read_expected("OASIS_GFX3820_SCREEN_ROOTS_MANIFEST_V1");
    read_expected(std::string("ROM_SHA256\t") + kGfxScreenRootsRomSha256);
    read_expected(std::string("SOURCE_CENSUS_SHA256\t") +
                  kGfxScreenRootsSourceCensusSha256);
    read_expected("ENUMERATION_SHA256\tb97b3e1e13262777e7826b2123930a1a328a3f621d8d3421d13d12e8d5813866");
    read_expected("BOUNDARY_RULE_ID\tGFX3820_TERMINATOR_EXACT_EOS_V1");
    read_expected("ROOT_COUNT\t21");
    read_expected("PATH_USE_COUNT\t167");
    read_expected("DESCRIPTOR_COUNT\t163");
    read_expected("STREAM_COUNT\t159");
    read_expected("COMPRESSED_UNION_BYTES\t337514");
    read_expected("DECODED_TOTAL_BYTES\t1921774");
    read_expected(kManifestHeader);

    Catalog catalog;
    std::set<std::uint16_t> roots;
    std::set<std::uint32_t> descriptors;
    std::set<std::pair<std::uint16_t, std::uint16_t>> root_indexes;
    std::uint32_t expected_path = 1U;
    std::size_t compressed_total = 0U;
    std::size_t decoded_total = 0U;
    while (std::getline(input, line)) {
        const auto fields = split_tabs(line);
        if (fields.size() != 17U || expected_path > kGfxScreenPathUseCount) {
            throw std::runtime_error("screen resource path row count or width mismatch");
        }
        const auto path_id = parse_number(fields[0]);
        const auto root_id = parse_number(fields[1]);
        const auto root_address = parse_number(fields[2]);
        const auto root_source = parse_number(fields[3]);
        const auto root_index = parse_number(fields[4]);
        const auto relative_word = parse_number(fields[5]);
        const auto relative_displacement = parse_number(fields[6]);
        const auto descriptor_id = fields[7];
        const auto descriptor_address = parse_number(fields[8]);
        const auto stream_id = fields[9];
        const auto start = parse_number(fields[10]);
        const auto end = parse_number(fields[11]);
        const auto compressed_size = parse_number(fields[12]);
        const auto decoded_size = parse_number(fields[14]);
        if (path_id != expected_path || root_id >= kGfxScreenRootCount ||
            root_source != kRootTableStart + root_id * 4U ||
            read_be32(rom, root_source) != root_address ||
            relative_word != root_address + root_index * 2U ||
            relative_word + relative_displacement != descriptor_address ||
            descriptor_id != std::string("DESC_") + std::string(fields[8].substr(2U)) ||
            stream_id != std::string("STREAM_") + std::string(fields[10].substr(2U)) ||
            end <= start || end - start != compressed_size ||
            end > rom.size() || !valid_hash(fields[13]) || !valid_hash(fields[15]) ||
            decoded_size == 0U || fields[16].empty()) {
            throw std::runtime_error("screen resource path violates its pinned relation");
        }
        if (!root_indexes.emplace(static_cast<std::uint16_t>(root_id),
                                  static_cast<std::uint16_t>(root_index)).second) {
            throw std::runtime_error("duplicate root index path");
        }
        roots.insert(static_cast<std::uint16_t>(root_id));
        descriptors.insert(descriptor_address);
        auto [it, inserted] = catalog.streams.try_emplace(std::string(stream_id));
        auto& stream = it->second;
        if (inserted) {
            stream.id = stream_id;
            stream.start = start;
            stream.end = end;
            stream.compressed_size = compressed_size;
            stream.compressed_sha = fields[13];
            stream.decoded_size = decoded_size;
            stream.decoded_sha = fields[15];
            compressed_total += compressed_size;
            decoded_total += decoded_size;
        } else if (stream.start != start || stream.end != end ||
                   stream.compressed_size != compressed_size ||
                   stream.compressed_sha != fields[13] ||
                   stream.decoded_size != decoded_size || stream.decoded_sha != fields[15]) {
            throw std::runtime_error("aliased stream ID has conflicting metadata");
        }
        stream.paths.push_back({static_cast<std::uint16_t>(root_id), root_address,
                                root_source, static_cast<std::uint16_t>(root_index),
                                std::string(descriptor_id), descriptor_address});
        ++expected_path;
    }
    if (expected_path != kGfxScreenPathUseCount + 1U || roots.size() != kGfxScreenRootCount ||
        descriptors.size() != kGfxScreenDescriptorCount ||
        catalog.streams.size() != kGfxScreenStreamCount ||
        compressed_total != 337514U || decoded_total != 1921774U ||
        kRootTableEnd - kRootTableStart != kGfxScreenRootCount * 4U) {
        throw std::runtime_error("screen resource manifest does not close the accepted family");
    }
    return catalog;
}

LoadedScreenResource decode_rom_stream(std::span<const std::uint8_t> rom,
                                       const StreamEntry& stream,
                                       std::string rejection) {
    std::vector<std::uint8_t> output(stream.decoded_size);
    const auto compressed = rom.subspan(stream.start, stream.compressed_size);
    const auto decoded = decompress_graphics(compressed, output);
    output.resize(decoded.output_size);
    if (decoded.source_consumed != stream.compressed_size ||
        decoded.output_size != stream.decoded_size ||
        oasis::calculate_sha256(compressed) != stream.compressed_sha ||
        oasis::calculate_sha256(output) != stream.decoded_sha) {
        throw std::runtime_error("ROM stream differs from exact screen manifest contract");
    }
    return {stream.id, stream.start, stream.end, stream.compressed_size,
            stream.compressed_sha, stream.decoded_size, stream.decoded_sha,
            std::move(output), stream.paths, "ROM_BACKED", std::move(rejection), {},
            stream.compressed_size, false};
}

} // namespace

LoadedScreenResource load_screen_graphics_resource(
    std::span<const std::uint8_t> rom, std::string_view stream_id,
    const std::filesystem::path& manifest,
    const std::filesystem::path& override_directory) {
    validate_rom(rom);
    auto catalog = parse_catalog(manifest, rom);
    const auto found = catalog.streams.find(stream_id);
    if (found == catalog.streams.end()) {
        throw std::out_of_range("unknown GFX_SCREEN_ROOTS StreamID");
    }
    const auto& stream = found->second;
    const auto override_path = override_directory / (stream.id + ".bin");
    std::error_code error;
    const bool exists = std::filesystem::is_regular_file(override_path, error);
    std::string rejection;
    if (error) rejection = "cannot inspect external stream";
    if (exists) {
        try {
            const auto compressed = read_binary(override_path, stream.compressed_size);
            if (oasis::calculate_sha256(compressed) != stream.compressed_sha) {
                throw std::runtime_error("external compressed SHA-256 mismatch");
            }
            std::vector<std::uint8_t> output(stream.decoded_size);
            const auto decoded = decompress_graphics(compressed, output);
            output.resize(decoded.output_size);
            if (decoded.source_consumed != compressed.size()) {
                throw std::runtime_error("decoder did not consume the complete external file");
            }
            if (decoded.output_size != stream.decoded_size ||
                oasis::calculate_sha256(output) != stream.decoded_sha) {
                throw std::runtime_error("external decoded output differs from manifest");
            }
            return {stream.id, stream.start, stream.end, stream.compressed_size,
                    stream.compressed_sha, stream.decoded_size, stream.decoded_sha,
                    std::move(output), stream.paths, "EXTERNAL_FILE_ACTIVE", {},
                    override_path.string(), 0U, true};
        } catch (const std::exception& exception) {
            rejection = exception.what();
        }
    } else if (rejection.empty()) {
        rejection = "external stream absent";
    }
    return decode_rom_stream(rom, stream, std::move(rejection));
}

} // namespace oasis::game
