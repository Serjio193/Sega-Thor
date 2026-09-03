#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/entities/entity_pool.hpp"

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

void expect_word(const oasis::Rom& rom,
                 std::size_t offset,
                 std::uint16_t expected,
                 const char* message) {
    if (read_u16(rom, offset) != expected) throw std::runtime_error(message);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_entity_pool_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }

        // 0x8E90, 0x8EB2 and 0x8ED4 are the three observed pool loops.
        expect_word(rom, 0x8E90, 0x4DF9, "first entity pool loop mismatch");
        expect_word(rom, 0x8E96, 0x7E03, "first entity pool count mismatch");
        expect_word(rom, 0x8E98, 0x23FC, "first entity pool dispatcher mismatch");
        expect_word(rom, 0x8EA6, 0x6E00, "first entity active branch mismatch");
        expect_word(rom, 0x8EB2, 0x4DF9, "main entity pool loop mismatch");
        expect_word(rom, 0x8EB8, 0x7E14, "main entity pool count mismatch");
        expect_word(rom, 0x8EC4, 0x3C2E, "main entity active read mismatch");
        expect_word(rom, 0x8EC8, 0x6E00, "main entity active branch mismatch");
        expect_word(rom, 0x8ECE, 0x00BC, "main entity stride mismatch");
        expect_word(rom, 0x8ED4, 0x4DF9, "second entity pool loop mismatch");
        expect_word(rom, 0x8EDA, 0x7E05, "second entity pool count mismatch");
        expect_word(rom, 0x8EE6, 0x3C2E, "second entity active read mismatch");
        expect_word(rom, 0x8EF0, 0x005A, "second entity stride mismatch");

        using oasis::game::entities::kEntityPoolAtFf19e8;
        using oasis::game::entities::kEntityPoolAtFf2954;
        using oasis::game::entities::kEntityPoolAtFf2d8c;
        if (kEntityPoolAtFf2954.record_count != 4 ||
            kEntityPoolAtFf2954.record_stride != 0x5A ||
            kEntityPoolAtFf19e8.record_count != 21 ||
            kEntityPoolAtFf19e8.record_stride != 0xBC ||
            kEntityPoolAtFf2d8c.record_count != 6 ||
            kEntityPoolAtFf2d8c.record_stride != 0x5A) {
            throw std::runtime_error("native entity pool descriptors mismatch");
        }
        std::cout << "verified entity pool loops and descriptors\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
