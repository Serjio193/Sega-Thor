#include "game/resource_loader.hpp"

#include "core/rom_identity.hpp"
#include "game/graphics_decompress.hpp"

#include <stdexcept>

namespace oasis::game {
namespace {

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4U) {
        throw std::out_of_range("resource table read out of range");
    }
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
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

LoadedResource load_verified_resource(std::span<const std::uint8_t> rom,
                                      std::uint8_t resource_id) {
    if (resource_id != kVerifiedResourceId) {
        throw std::out_of_range("only verified resource ID 3 is supported");
    }
    if (rom.size() != 0x300000U || calculate_sha256(rom) != kCanonicalRomSha256) {
        throw std::runtime_error("resource ID 3 requires the canonical USA ROM");
    }

    const auto table_entry = kResourceTableAddress +
                             static_cast<std::size_t>(resource_id) * 4U;
    const auto input_start = read_be32(rom, table_entry);
    const auto input_end = read_be32(rom, table_entry + 4U);
    if (input_start != kResourceInputAddress || input_end <= input_start ||
        input_end > rom.size()) {
        throw std::runtime_error("resource ID 3 table range mismatch");
    }

    std::vector<std::uint8_t> output(kResourceOutputSize);
    const auto result = decompress_graphics(
        rom.subspan(input_start, input_end - input_start), output);
    output.resize(result.output_size);
    validate_resource_id3_output(result.source_consumed, output);
    if (result.source_consumed != input_end - input_start) {
        throw std::runtime_error("resource decompressor did not consume table range");
    }

    return {resource_id, static_cast<std::uint32_t>(table_entry), input_start,
            result.source_consumed, std::move(output)};
}

} // namespace oasis::game
