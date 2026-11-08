#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game {

struct AudioFormatADescriptor {
    std::uint8_t bank_id{};
    std::uint8_t entry_index{};
    std::uint8_t selector_id{};
    std::uint8_t mode{};
    std::uint16_t logical_start{};
    std::uint32_t physical_start{};
    std::uint32_t physical_end{};
};

struct AudioFormatADecodeResult {
    std::size_t compressed_bytes_consumed{};
    std::vector<std::uint8_t> pcm_samples;
};

[[nodiscard]] std::vector<AudioFormatADescriptor> parse_audio_format_a_descriptors(
    std::span<const std::uint8_t> rom);

[[nodiscard]] AudioFormatADecodeResult decode_audio_format_a(
    std::span<const std::uint8_t> compressed,
    std::uint8_t mode,
    std::uint8_t initial_accumulator = 0x80U);

} // namespace oasis::game
