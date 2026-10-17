#include "game/resource_loader.hpp"

#include "core/rom_identity.hpp"
#include "game/graphics_decompress.hpp"

#include <array>
#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace oasis::game {
namespace {

constexpr std::size_t kRomSize = 0x300000U;
constexpr std::size_t kManifestRows = kGfx3820ResourceCount + 1U;
constexpr std::string_view kManifestName = "manifest.tsv";
constexpr std::string_view kReplacementName = "replacements.tsv";
constexpr std::string_view kManifestHeader =
    "RESOURCE_ID\tROM_START\tROM_END\tCOMPRESSED_SIZE\tCOMPRESSED_SHA256"
    "\tDECODED_SIZE\tDECODED_SHA256\tSOURCE_CENSUS_HASH\tROM_SHA256";
constexpr std::string_view kReplacementHeader =
    "RESOURCE_ID\tCOMPRESSED_SIZE\tCOMPRESSED_SHA256\tDECODED_SHA256";

struct ManifestEntry {
    std::uint16_t id{};
    std::uint32_t rom_start{};
    std::uint32_t rom_end{};
    std::size_t compressed_size{};
    std::string compressed_sha256;
    std::size_t decoded_size{};
    std::string decoded_sha256;
};

struct Manifest {
    std::array<ManifestEntry, kManifestRows> entries{};
};

struct ReplacementEntry {
    std::size_t compressed_size{};
    std::string compressed_sha256;
    std::string decoded_sha256;
};

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4U) {
        throw std::out_of_range("resource table read out of range");
    }
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open " + path.string());
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) throw std::runtime_error("cannot read " + path.string());
    return contents.str();
}

std::vector<std::uint8_t> read_binary(const std::filesystem::path& path,
                                     std::size_t expected_size) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) throw std::runtime_error("cannot stat override file");
    if (size != expected_size) throw std::runtime_error("compressed size mismatch");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open override file");
    std::vector<std::uint8_t> bytes(expected_size);
    if (expected_size != 0U &&
        !input.read(reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(expected_size))) {
        throw std::runtime_error("cannot read complete override file");
    }
    return bytes;
}

std::string normalize_newlines(std::string text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1U >= text.size() || text[i + 1U] != '\n') {
                throw std::runtime_error("manifest contains a bare carriage return");
            }
            ++i;
            normalized.push_back('\n');
        } else {
            normalized.push_back(text[i]);
        }
    }
    return normalized;
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
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::runtime_error("manifest contains an invalid numeric field");
    }
    return value;
}

bool valid_sha256(std::string_view value) {
    if (value.size() != 64U) return false;
    for (const char ch : value) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) return false;
    }
    return true;
}

std::span<const std::uint8_t> byte_span(std::string_view text) {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}

Manifest parse_manifest(const std::filesystem::path& path) {
    const auto normalized = normalize_newlines(read_text(path));
    if (calculate_sha256(byte_span(normalized)) != kGfx3820ManifestSha256) {
        throw std::runtime_error("manifest SHA-256 mismatch");
    }
    std::istringstream input(normalized);
    std::string line;
    if (!std::getline(input, line) || line != "OASIS_GFX3820_RESOURCE_MANIFEST_V1") {
        throw std::runtime_error("manifest schema mismatch");
    }
    if (!std::getline(input, line) || line != std::string("ROM_SHA256\t") + kCanonicalRomSha256) {
        throw std::runtime_error("manifest ROM identity mismatch");
    }
    if (!std::getline(input, line) || line != std::string("SOURCE_CENSUS_SHA256\t") + kGfx3820SourceCensusSha256) {
        throw std::runtime_error("manifest source census identity mismatch");
    }
    if (!std::getline(input, line) || line != kManifestHeader) {
        throw std::runtime_error("manifest columns mismatch");
    }

    Manifest manifest{};
    std::uint16_t expected_id = 1U;
    while (std::getline(input, line)) {
        const auto fields = split_tabs(line);
        if (fields.size() != 9U || expected_id > kGfx3820ResourceCount) {
            throw std::runtime_error("manifest row count or field count mismatch");
        }
        const auto id = parse_number(fields[0]);
        const auto start = parse_number(fields[1]);
        const auto end = parse_number(fields[2]);
        const auto compressed_size = parse_number(fields[3]);
        const auto decoded_size = parse_number(fields[5]);
        if (id != expected_id || end <= start || end - start != compressed_size ||
            decoded_size != kResourceOutputSize ||
            fields[7] != kGfx3820SourceCensusSha256 || fields[8] != kCanonicalRomSha256 ||
            !valid_sha256(fields[4]) || !valid_sha256(fields[6])) {
            throw std::runtime_error("manifest row violates the pinned family contract");
        }
        manifest.entries[id] = {static_cast<std::uint16_t>(id), start, end,
                                compressed_size, std::string(fields[4]), decoded_size,
                                std::string(fields[6])};
        ++expected_id;
    }
    if (expected_id != kGfx3820ResourceCount + 1U) {
        throw std::runtime_error("manifest does not enumerate ResourceIDs 1..107");
    }
    return manifest;
}

std::array<std::optional<ReplacementEntry>, kManifestRows> parse_replacements(
    const std::filesystem::path& path) {
    const auto normalized = normalize_newlines(read_text(path));
    std::istringstream input(normalized);
    std::string line;
    if (!std::getline(input, line) || line != "OASIS_GFX3820_REPLACEMENTS_V1" ||
        !std::getline(input, line) || line != kReplacementHeader) {
        throw std::runtime_error("replacement manifest schema mismatch");
    }
    std::array<std::optional<ReplacementEntry>, kManifestRows> replacements{};
    std::uint16_t previous_id = 0U;
    while (std::getline(input, line)) {
        const auto fields = split_tabs(line);
        if (fields.size() != 4U) throw std::runtime_error("replacement row field count mismatch");
        const auto id = parse_number(fields[0]);
        const auto size = parse_number(fields[1]);
        if (id == 0U || id > kGfx3820ResourceCount || id <= previous_id || size == 0U ||
            !valid_sha256(fields[2]) || !valid_sha256(fields[3])) {
            throw std::runtime_error("replacement row is invalid or duplicated");
        }
        replacements[id] = ReplacementEntry{size, std::string(fields[2]),
                                             std::string(fields[3])};
        previous_id = static_cast<std::uint16_t>(id);
    }
    return replacements;
}

void validate_rom(std::span<const std::uint8_t> rom) {
    if (rom.size() != kRomSize || calculate_sha256(rom) != kCanonicalRomSha256) {
        throw std::runtime_error("graphics resources require the canonical USA ROM");
    }
}

LoadedResource decode_rom_resource(std::span<const std::uint8_t> rom,
                                   std::uint16_t id,
                                   std::uint32_t table_entry,
                                   std::uint32_t input_start,
                                   const ManifestEntry* manifest_entry,
                                   std::string rejection_reason) {
    auto available = rom.subspan(input_start);
    if (manifest_entry != nullptr) {
        if (manifest_entry->rom_start != input_start ||
            manifest_entry->compressed_size > available.size()) {
            throw std::runtime_error("ROM pointer differs from exact resource manifest");
        }
        available = available.first(manifest_entry->compressed_size);
    }
    std::vector<std::uint8_t> output(kResourceOutputSize);
    const auto decoded = decompress_graphics(available, output);
    output.resize(decoded.output_size);
    if (decoded.output_size != kResourceOutputSize || decoded.source_consumed == 0U) {
        throw std::runtime_error("ROM graphics stream violated the 4096-byte family contract");
    }
    if (manifest_entry != nullptr &&
        (manifest_entry->rom_start != input_start ||
         manifest_entry->compressed_size != decoded.source_consumed ||
         calculate_sha256(available.first(decoded.source_consumed)) !=
             manifest_entry->compressed_sha256 ||
         calculate_sha256(output) != manifest_entry->decoded_sha256)) {
        throw std::runtime_error("ROM graphics stream differs from its accepted manifest row");
    }
    const auto compressed = available.first(decoded.source_consumed);
    const auto compressed_hash = calculate_sha256(compressed);
    const auto end = input_start + static_cast<std::uint32_t>(decoded.source_consumed);
    return {id, table_entry, input_start, end, decoded.source_consumed,
            compressed_hash, calculate_sha256(output), std::move(output),
            "ROM_BACKED", "EXTERNALIZABLE", "NONE",
            std::move(rejection_reason), {},
            decoded.source_consumed, false, false};
}

} // namespace

void validate_resource_id3_output(std::size_t compressed_size,
                                  std::span<const std::uint8_t> output) {
    if (compressed_size != kResourceCompressedSize) {
        throw std::runtime_error("resource ID 3 compressed size mismatch");
    }
    if (output.size() != kResourceOutputSize) {
        throw std::runtime_error("resource ID 3 output size mismatch");
    }
    if (calculate_sha256(output) != kResourceOutputSha256) {
        throw std::runtime_error("resource ID 3 output hash mismatch");
    }
}

LoadedResource load_graphics_resource(std::span<const std::uint8_t> rom,
                                      std::uint16_t resource_id,
                                      const std::filesystem::path& override_directory) {
    validate_rom(rom);
    if (resource_id == 0U || resource_id > kGfx3820ResourceCount) {
        throw std::out_of_range("ResourceID is outside the proven 1..107 family");
    }
    const auto table_entry = kResourceTableAddress +
                             static_cast<std::size_t>(resource_id) * 4U;
    const auto input_start = read_be32(rom, table_entry);
    if (input_start == 0U || input_start >= rom.size()) {
        throw std::runtime_error("ResourceID does not resolve to a canonical ROM stream");
    }

    std::optional<Manifest> manifest;
    const ManifestEntry* manifest_entry = nullptr;
    std::string rejection_reason;
    const auto override_path = override_directory / (std::to_string(resource_id) + ".bin");
    std::error_code error;
    const bool override_exists = std::filesystem::is_regular_file(override_path, error);
    if (error) {
        rejection_reason = "cannot inspect override file";
    }
    const auto manifest_path = override_directory / kManifestName;
    error.clear();
    const bool manifest_exists = std::filesystem::is_regular_file(manifest_path, error);
    if (error && rejection_reason.empty()) {
        rejection_reason = "cannot inspect override manifest";
    }
    if (manifest_exists) {
        try {
            manifest = parse_manifest(manifest_path);
            manifest_entry = &manifest->entries[resource_id];
            if (manifest_entry->rom_start != input_start) {
                throw std::runtime_error("manifest pointer does not match canonical table");
            }
        } catch (const std::exception& exception) {
            if (rejection_reason.empty()) rejection_reason = exception.what();
            manifest_entry = nullptr;
            manifest.reset();
        }
    }

    if (override_exists) {
        try {
            if (manifest_entry == nullptr) {
                throw std::runtime_error(manifest_exists ? rejection_reason
                                                         : "override manifest absent");
            }

            const auto replacements_path = override_directory / kReplacementName;
            std::array<std::optional<ReplacementEntry>, kManifestRows> replacements{};
            if (std::filesystem::exists(replacements_path, error)) {
                if (error) throw std::runtime_error("cannot inspect replacement manifest");
                replacements = parse_replacements(replacements_path);
            } else if (error) {
                throw std::runtime_error("cannot inspect replacement manifest");
            }

            const auto* replacement = replacements[resource_id]
                                         ? &*replacements[resource_id] : nullptr;
            const auto expected_size = replacement == nullptr
                                           ? manifest_entry->compressed_size
                                           : replacement->compressed_size;
            const auto compressed = read_binary(override_path, expected_size);
            const auto compressed_hash = calculate_sha256(compressed);
            const auto expected_compressed_hash = replacement == nullptr
                                                      ? manifest_entry->compressed_sha256
                                                      : replacement->compressed_sha256;
            const auto expected_decoded_hash = replacement == nullptr
                                                   ? manifest_entry->decoded_sha256
                                                   : replacement->decoded_sha256;
            if (replacement != nullptr &&
                replacement->compressed_sha256 == manifest_entry->compressed_sha256) {
                throw std::runtime_error("replacement mode must use a distinct explicit hash");
            }
            if (compressed_hash != expected_compressed_hash) {
                throw std::runtime_error("compressed SHA-256 mismatch");
            }

            std::vector<std::uint8_t> output(manifest_entry->decoded_size);
            const auto decoded = decompress_graphics(compressed, output);
            output.resize(decoded.output_size);
            if (decoded.source_consumed != compressed.size()) {
                throw std::runtime_error("decoder did not consume the complete external file");
            }
            if (decoded.output_size != manifest_entry->decoded_size ||
                decoded.output_size != kResourceOutputSize) {
                throw std::runtime_error("external decoded size mismatch");
            }
            const auto output_hash = calculate_sha256(output);
            if (output_hash != expected_decoded_hash) {
                throw std::runtime_error("decoded SHA-256 mismatch");
            }

            return {resource_id, static_cast<std::uint32_t>(table_entry), input_start,
                    manifest_entry->rom_end, compressed.size(), compressed_hash,
                    output_hash, std::move(output), "EXTERNAL_FILE_ACTIVE",
                    "EXTERNAL_FILE_ACTIVE",
                    replacement == nullptr ? "EXACT_EXTERNAL_COPY"
                                            : "MODIFIED_EXTERNAL_RESOURCE",
                    {}, override_path.string(), 0U, true, true};
        } catch (const std::exception& exception) {
            rejection_reason = exception.what();
        }
    } else if (rejection_reason.empty()) {
        rejection_reason = "override file absent";
    }

    return decode_rom_resource(rom, resource_id,
                               static_cast<std::uint32_t>(table_entry), input_start,
                               manifest_entry, std::move(rejection_reason));
}

LoadedResource load_verified_resource(
    std::span<const std::uint8_t> rom,
    std::uint8_t resource_id,
    const std::filesystem::path& override_directory) {
    if (resource_id != kVerifiedResourceId) {
        throw std::out_of_range("legacy verified-resource API supports ID 3 only");
    }
    auto resource = load_graphics_resource(rom, resource_id, override_directory);
    validate_resource_id3_output(resource.compressed_size, resource.bytes);
    return resource;
}

} // namespace oasis::game
