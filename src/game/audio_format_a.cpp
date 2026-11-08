#include "game/audio_format_a.hpp"

#include <array>
#include <limits>
#include <stdexcept>

namespace oasis::game {
namespace {

constexpr std::array<std::uint32_t, 8> kBankBases{
    0x080000U, 0x088000U, 0x090000U, 0x098000U,
    0x0A0000U, 0x0A8000U, 0x0B0000U, 0x0B8000U};
constexpr std::array<std::int16_t, 16> kDelta{
    0, 0, 1, 2, 6, 12, 24, 48, 96, -96, -48, -24, -12, -6, -2, -1};
constexpr std::size_t kBankSize = 0x8000U;
constexpr std::size_t kDescriptorSize = 5U;
constexpr std::size_t kDescriptorCount = 16U;
constexpr std::size_t kDescriptorTableSize = kDescriptorSize * kDescriptorCount;

std::uint16_t read_le16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2U) {
        throw std::out_of_range("audio descriptor word exceeds ROM");
    }
    return static_cast<std::uint16_t>(bytes[offset] |
        (static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U));
}

void append_sample(AudioFormatADecodeResult& result, std::uint8_t sample,
                   std::uint8_t mode) {
    result.pcm_samples.push_back(sample);
    if (mode == 1U) result.pcm_samples.push_back(sample);
}

} // namespace

std::vector<AudioFormatADescriptor> parse_audio_format_a_descriptors(
    std::span<const std::uint8_t> rom) {
    std::vector<AudioFormatADescriptor> descriptors;
    for (std::size_t bank_slot = 0; bank_slot < kBankBases.size(); ++bank_slot) {
        const auto bank_base = kBankBases[bank_slot];
        if (bank_base > rom.size() || rom.size() - bank_base < kDescriptorTableSize) {
            throw std::out_of_range("audio descriptor table exceeds ROM");
        }
        for (std::size_t entry = 0; entry < kDescriptorCount; ++entry) {
            const auto offset = static_cast<std::size_t>(bank_base) +
                                entry * kDescriptorSize;
            const auto logical_start = read_le16(rom, offset);
            const auto compressed_size = read_le16(rom, offset + 2U);
            if (logical_start == 0U || compressed_size == 0U) continue;
            if (logical_start < 0x8000U) {
                throw std::runtime_error("active audio descriptor has invalid bank address");
            }
            const auto mode = rom[offset + 4U];
            if (mode > 1U) throw std::runtime_error("unsupported audio descriptor mode");
            const auto physical_start = bank_base +
                static_cast<std::uint32_t>(logical_start - 0x8000U);
            const auto physical_end = physical_start + compressed_size;
            const auto bank_end = bank_base + static_cast<std::uint32_t>(kBankSize);
            if (physical_start < bank_base + kDescriptorTableSize ||
                physical_end < physical_start || physical_end > bank_end ||
                physical_end > rom.size()) {
                throw std::runtime_error("audio descriptor payload exceeds its bank");
            }
            const auto selector = bank_slot * kDescriptorCount + entry;
            descriptors.push_back({
                static_cast<std::uint8_t>(0x10U + bank_slot),
                static_cast<std::uint8_t>(entry),
                static_cast<std::uint8_t>(selector),
                mode,
                logical_start,
                physical_start,
                physical_end});
        }
    }
    return descriptors;
}

AudioFormatADecodeResult decode_audio_format_a(
    std::span<const std::uint8_t> compressed, std::uint8_t mode,
    std::uint8_t initial_accumulator) {
    if (mode > 1U) throw std::invalid_argument("unsupported Format-A mode");
    constexpr auto max_samples_per_byte = 12U;
    if (compressed.size() > std::numeric_limits<std::size_t>::max() /
                                max_samples_per_byte) {
        throw std::length_error("Format-A output size overflows address space");
    }

    AudioFormatADecodeResult result;
    result.pcm_samples.reserve(compressed.size() * max_samples_per_byte);
    std::uint8_t accumulator = initial_accumulator;
    std::int16_t previous_delta = 0;
    for (const auto byte : compressed) {
        for (const auto nibble : {static_cast<std::uint8_t>(byte >> 4U),
                                  static_cast<std::uint8_t>(byte & 0x0FU)}) {
            const auto ticks = nibble == 0U ? 3U : 1U;
            if (nibble != 0U) previous_delta = kDelta[nibble];
            for (unsigned tick = 0; tick < ticks; ++tick) {
                accumulator = static_cast<std::uint8_t>(
                    static_cast<unsigned>(accumulator) +
                    static_cast<std::uint8_t>(previous_delta));
                append_sample(result, accumulator, mode);
            }
        }
    }
    result.compressed_bytes_consumed = compressed.size();
    return result;
}

} // namespace oasis::game
