#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/player/player.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

std::uint16_t read_u16(const oasis::Rom& rom, std::size_t offset) {
    if (offset + 2U > rom.size()) throw std::runtime_error("ROM read outside image");
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom.bytes()[offset]) << 8U) |
        static_cast<std::uint16_t>(rom.bytes()[offset + 1U]));
}

std::uint32_t read_u32(const oasis::Rom& rom, std::size_t offset) {
    if (offset + 4U > rom.size()) throw std::runtime_error("ROM read outside image");
    return (static_cast<std::uint32_t>(rom.bytes()[offset]) << 24U) |
           (static_cast<std::uint32_t>(rom.bytes()[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(rom.bytes()[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(rom.bytes()[offset + 3U]);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_player_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }

        // 0x85E2 masks the controller nibble and 0x85FA is its relative table.
        if (read_u32(rom, 0x85E2) != 0x362E0016U) {
            throw std::runtime_error("unexpected player direction routine bytes");
        }
        if (read_u16(rom, 0x59BA) != 0x083CU ||
            read_u16(rom, 0x59BC) != 0x0928U ||
            read_u16(rom, 0x59BE) != 0x0B58U) {
            throw std::runtime_error("player state dispatch table mismatch");
        }
        constexpr std::array<std::int16_t, 16> dispatch_offsets{
            0x20, 0x58, 0x46, 0x1A, 0x32, 0x9C, 0x86, 0x2C,
            0x1A, 0x6C, 0x56, 0x14, 0x08, 0x40, 0x2E, 0x02,
        };
        for (std::size_t i = 0; i < dispatch_offsets.size(); ++i) {
            if (read_u16(rom, 0x85FAU + i * 2U) !=
                static_cast<std::uint16_t>(dispatch_offsets[i])) {
                throw std::runtime_error("player direction dispatch table mismatch");
            }
        }

        if (read_u32(rom, 0x8628) != 0x00036000U ||
            read_u32(rom, 0x8638) != 0xFFFCA000U ||
            read_u32(rom, 0x864A) != 0x00030000U ||
            read_u32(rom, 0x865A) != 0xFFFD0000U) {
            throw std::runtime_error("player cardinal vector oracle mismatch");
        }

        const auto right = oasis::game::player::movement_vector(0x8U);
        const auto up_left = oasis::game::player::movement_vector(0x5U);
        if (right.x_fixed != 0x36000 || right.y_fixed != 0 ||
            up_left.x_fixed != -0x2A000 || up_left.y_fixed != -0x25800) {
            throw std::runtime_error("native player vectors differ from ROM oracle");
        }
        std::cout << "verified player direction table and cardinal/diagonal vectors\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
