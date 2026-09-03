#include "game/world/screen_descriptor.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game::world {
namespace {

constexpr std::size_t kGroupTable = 0x00C92C;
constexpr std::size_t kGroupCount = 21;

std::uint16_t read_u16(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 2) {
        throw std::out_of_range("screen descriptor word read out of range");
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1]));
}

std::uint32_t read_u32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4) {
        throw std::out_of_range("screen descriptor long read out of range");
    }
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3]);
}

} // namespace

std::uint32_t ScreenDescriptorRaw::primary_stream_pointer() const noexcept {
    return (static_cast<std::uint32_t>(bytes[4]) << 24U) |
           (static_cast<std::uint32_t>(bytes[5]) << 16U) |
           (static_cast<std::uint32_t>(bytes[6]) << 8U) |
           static_cast<std::uint32_t>(bytes[7]);
}

std::array<std::uint8_t, 4> ScreenDescriptorRaw::resource_ids() const noexcept {
    return {bytes[8], bytes[9], bytes[10], bytes[11]};
}

std::array<std::int8_t, 4> ScreenDescriptorRaw::signed_parameters() const noexcept {
    return {
        static_cast<std::int8_t>(bytes[12]),
        static_cast<std::int8_t>(bytes[13]),
        static_cast<std::int8_t>(bytes[14]),
        static_cast<std::int8_t>(bytes[15]),
    };
}

std::array<std::uint16_t, 5> ScreenDescriptorRaw::trailing_words() const noexcept {
    std::array<std::uint16_t, 5> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto offset = 16U + i * 2U;
        result[i] = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
            static_cast<std::uint16_t>(bytes[offset + 1]));
    }
    return result;
}

ScreenDescriptorRaw load_screen_descriptor(std::span<const std::uint8_t> rom, ScreenId id) {
    if (id.group >= kGroupCount) {
        throw std::out_of_range("screen group is outside confirmed dispatcher table");
    }

    const auto group_pointer_offset = kGroupTable + static_cast<std::size_t>(id.group) * 4U;
    const auto group_base = static_cast<std::size_t>(read_u32(rom, group_pointer_offset));
    const auto entry_address = group_base + static_cast<std::size_t>(id.index) * 2U;
    const auto relative_word = read_u16(rom, entry_address);
    const auto relative = static_cast<std::int16_t>(relative_word);
    const auto descriptor_signed = static_cast<std::int64_t>(entry_address) + relative;
    if (descriptor_signed < 0) {
        throw std::out_of_range("screen descriptor resolves before ROM start");
    }
    const auto descriptor = static_cast<std::size_t>(descriptor_signed);
    constexpr std::size_t required = ScreenDescriptorRaw::kSize + 4U;
    if (descriptor > rom.size() || rom.size() - descriptor < required) {
        throw std::out_of_range("screen descriptor resolves outside ROM");
    }

    ScreenDescriptorRaw result{};
    result.rom_address = static_cast<std::uint32_t>(descriptor);
    std::copy_n(rom.begin() + static_cast<std::ptrdiff_t>(descriptor),
                ScreenDescriptorRaw::kSize,
                result.bytes.begin());
    result.init_function = read_u32(rom, descriptor + ScreenDescriptorRaw::kSize);
    return result;
}

} // namespace oasis::game::world
