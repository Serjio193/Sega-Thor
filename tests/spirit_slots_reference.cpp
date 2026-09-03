#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/spirits/spirit_slots.hpp"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

namespace {

void expect_bytes(const oasis::Rom& rom,
                  std::size_t offset,
                  std::initializer_list<std::uint8_t> expected,
                  const char* message) {
    if (offset + expected.size() > rom.size()) {
        throw std::runtime_error(message);
    }
    std::size_t index = 0;
    for (const auto value : expected) {
        if (rom.bytes()[offset + index++] != value) {
            throw std::runtime_error(message);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_spirit_slots_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status !=
            oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }

        // 0x7BE8: event 0x16..0x19 maps to FF0DBA bits 0..3.
        expect_bytes(rom, 0x7BE8,
                     {0x04, 0x00, 0x00, 0x16, 0x01, 0xF9, 0x00, 0xFF,
                      0x0D, 0xBA},
                     "spirit event-to-slot mapping mismatch");
        // 0x5202 and 0x522E: the four slot flags and event selectors are read
        // as a contiguous four-entry group.
        expect_bytes(rom, 0x5202,
                     {0x10, 0x39, 0x00, 0xFF, 0x0D, 0xBA},
                     "spirit slot flag read mismatch");
        expect_bytes(rom, 0x522E,
                     {0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19},
                     "spirit selector table mismatch");

        // 0x31B80: the observed active path checks +0x41 bits 3 and 1,
        // gates resource selector 0x13 on FF0DBA bit 1 and FF0DC4 bit 0,
        // then falls through to effect selector 0x15.
        expect_bytes(rom, 0x31B80,
                     {0x61, 0x00, 0x00, 0xB4, 0x4E, 0xB9, 0x00, 0x00,
                      0xC9, 0xB2, 0x41, 0xF9, 0x00, 0xFF, 0x19, 0xE8,
                      0x10, 0x28, 0x00, 0x41, 0x08, 0x00, 0x00, 0x03},
                     "spirit dispatch entry mismatch");
        expect_bytes(rom, 0x31BC4,
                     {0x08, 0x39, 0x00, 0x01, 0x00, 0xFF, 0x0D, 0xBA,
                      0x67, 0x24, 0x08, 0x39, 0x00, 0x00, 0x00, 0xFF,
                      0x0D, 0xC4},
                     "spirit dispatch gate mismatch");

        using namespace oasis::game::spirits;
        const auto trace = trace_observed_dispatch(
            {.input_41 = 0x0A, .slot_flags = 0x02});
        if (!trace.entered_observed_path || !trace.slot_available ||
            trace.selector_count != 2 ||
            trace.selectors[0] != kObservedResourceSelector ||
            trace.selectors[1] != kObservedEffectSelector ||
            (trace.guard_flags & 0x01U) == 0) {
            throw std::runtime_error("native spirit dispatch trace mismatch");
        }
        std::cout << "verified spirit slot mapping and dispatch trace\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
