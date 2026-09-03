#include "game/world/screen_descriptor.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

void write_u16(std::vector<std::uint8_t>& data, std::size_t offset, std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>(value >> 8U);
    data[offset + 1] = static_cast<std::uint8_t>(value);
}

void write_u32(std::vector<std::uint8_t>& data, std::size_t offset, std::uint32_t value) {
    data[offset] = static_cast<std::uint8_t>(value >> 24U);
    data[offset + 1] = static_cast<std::uint8_t>(value >> 16U);
    data[offset + 2] = static_cast<std::uint8_t>(value >> 8U);
    data[offset + 3] = static_cast<std::uint8_t>(value);
}

} // namespace

int main() {
    using oasis::game::world::ScreenId;
    using oasis::game::world::load_screen_descriptor;

    constexpr std::size_t group_table = 0xC92C;
    constexpr std::size_t group_base = 0xCA00;
    constexpr std::size_t entry = group_base + 3U * 2U;
    constexpr std::size_t descriptor = 0xCB20;

    std::vector<std::uint8_t> rom(0xCC00, 0);
    write_u32(rom, group_table + 2U * 4U, group_base);
    write_u16(rom, entry, static_cast<std::uint16_t>(descriptor - entry));

    write_u32(rom, descriptor + 4, 0x00123456U);
    rom[descriptor + 8] = 3;
    rom[descriptor + 9] = 4;
    rom[descriptor + 10] = 5;
    rom[descriptor + 11] = 6;
    rom[descriptor + 12] = 7;
    rom[descriptor + 13] = 0xFE;
    rom[descriptor + 14] = 9;
    rom[descriptor + 15] = 0xFF;
    for (std::size_t i = 0; i < 5; ++i) {
        write_u16(rom, descriptor + 16 + i * 2, static_cast<std::uint16_t>(0x100 + i));
    }

    const auto result = load_screen_descriptor(rom, ScreenId{2, 3});
    assert(result.rom_address == descriptor);
    assert(result.primary_stream_pointer() == 0x00123456U);
    assert((result.resource_ids() == std::array<std::uint8_t, 4>{3, 4, 5, 6}));
    assert((result.signed_parameters() == std::array<std::int8_t, 4>{7, -2, 9, -1}));
    assert(result.trailing_words()[0] == 0x100U);
    assert(result.trailing_words()[4] == 0x104U);
    assert(result.init_code_address == descriptor + 26U);

    bool threw = false;
    try {
        (void)load_screen_descriptor(rom, ScreenId{21, 0});
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    return 0;
}
