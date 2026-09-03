#include "core/rom.hpp"
#include "game/world/screen_descriptor.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

struct Case {
    oasis::game::world::ScreenId id;
    std::uint32_t descriptor;
    std::uint32_t primary_stream;
    std::array<std::uint8_t, 4> resource_ids;
};

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) return 2;

    const oasis::Rom rom = oasis::Rom::load(argv[1]);
    const std::array<Case, 4> cases{{
        {{0x00, 0x09}, 0x02CF82, 0x001F4E64, {21, 22, 5, 6}},
        {{0x00, 0x0C}, 0x02D3E8, 0x001FA32A, {7, 8, 86, 10}},
        {{0x07, 0x04}, 0x032144, 0x0022CC00, {31, 32, 33, 34}},
        {{0x07, 0x05}, 0x03285C, 0x0022E4A4, {31, 32, 33, 34}},
    }};

    for (const auto& expected : cases) {
        const auto actual = oasis::game::world::load_screen_descriptor(rom.bytes(), expected.id);
        assert(actual.rom_address == expected.descriptor);
        assert(actual.primary_stream_pointer() == expected.primary_stream);
        assert(actual.resource_ids() == expected.resource_ids);
        assert(actual.init_code_address == expected.descriptor + 26U);
        std::cout << "verified screen_id=0x" << std::hex
                  << static_cast<unsigned>(expected.id.group)
                  << static_cast<unsigned>(expected.id.index)
                  << " descriptor=0x" << actual.rom_address
                  << " stream=0x" << actual.primary_stream_pointer() << '\n';
    }

    return 0;
}
