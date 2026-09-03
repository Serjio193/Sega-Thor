#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/scripts/event_router.hpp"

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
        std::cerr << "usage: oasis_event_router_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status !=
            oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }

        // 0x82AE consumes a type-8 FF19E8 record and writes the event RAM
        // triple from raw offsets +0x32, +0x52, +0x04 and +0x4E.
        expect_bytes(rom, 0x82AE,
                     {0x72, 0x06, 0x36, 0x01, 0x44, 0x41, 0x34, 0x01,
                      0x38, 0x03, 0x42, 0x45, 0x61, 0x00, 0x37, 0x30},
                     "event producer query mismatch");
        expect_bytes(rom, 0x82C2,
                     {0x00, 0x08, 0x00, 0x00, 0x66, 0x2A, 0x42, 0x68,
                      0x00, 0x00, 0x30, 0x28, 0x00, 0x32, 0xE1, 0x48},
                     "event producer type gate mismatch");
        expect_bytes(rom, 0x82D6,
                     {0x33, 0xC0, 0x00, 0xFF, 0x19, 0x76, 0x33, 0xE8,
                      0x00, 0x04, 0x00, 0xFF, 0x19, 0x78, 0x23, 0xE8},
                     "event producer RAM write mismatch");

        // 0x7A28 consumes FF1976 and splits the bounded raw code ranges.
        expect_bytes(rom, 0x7A28,
                     {0x08, 0x2E, 0x00, 0x01, 0x00, 0x37, 0x67, 0x00,
                      0x00, 0xF8, 0x10, 0x39, 0x00, 0xFF, 0x19, 0x76},
                     "event router entry mismatch");
        expect_bytes(rom, 0x7A54,
                     {0x0C, 0x00, 0x00, 0x15, 0x63, 0x00, 0x01, 0x4A,
                      0x0C, 0x00, 0x00, 0x19, 0x63, 0x00, 0x01, 0x86},
                     "event router range mismatch");
        expect_bytes(rom, 0x7B2A,
                     {0x30, 0x3C, 0x00, 0x06, 0x4E, 0xB9, 0x00, 0x06,
                      0x00, 0x04, 0x02, 0x40, 0x01, 0xFF, 0x0C, 0x40,
                      0x01, 0xFF, 0x67, 0x02, 0x4E, 0x75},
                     "event flag-clear handler mismatch");
        expect_bytes(rom, 0x7B40,
                     {0x30, 0x3C, 0x00, 0x08, 0x4E, 0xB9, 0x00, 0x06,
                      0x00, 0x04, 0x02, 0x39, 0xFF, 0xF9, 0x00, 0xFF,
                      0x17, 0xB8},
                     "event flag-clear state update mismatch");
        expect_bytes(rom, 0x7B52,
                     {0x3D, 0x79, 0x00, 0xFF, 0x0D, 0x7E, 0x00, 0x06,
                      0x3D, 0x7C, 0xFF, 0xFF, 0x00, 0x5C, 0x60, 0x00,
                      0xE7, 0x6A},
                     "event flag-clear record update mismatch");
        expect_bytes(rom, 0x62CC,
                     {0x70, 0x00, 0x2D, 0x40, 0x00, 0x4E, 0x2D, 0x40,
                      0x00, 0x52, 0x3D, 0x7C, 0x00, 0x00, 0x00, 0x2A,
                      0x3D, 0x7C, 0x00, 0x00, 0x00, 0x04, 0x4E, 0x75},
                     "event flag-clear cleanup mismatch");

        using namespace oasis::game::scripts;
        const auto transfer = produce_observed_event({
            .raw_type = kObservedEventSourceType,
            .raw_32 = 0x0012,
            .raw_52 = 0x34,
            .raw_04 = 0x5678,
            .raw_4e = -0x123456,
        });
        if (!transfer || transfer->raw_event_code != 0x1234 ||
            transfer->raw_event_word != 0x5678 ||
            transfer->raw_event_long != -0x123456 ||
            !transfer->source_type_cleared) {
            throw std::runtime_error("native event producer mismatch");
        }
        if (route_observed_event(0x16, 0x02).handler_address != 0x7BE8 ||
            route_observed_event(0x16, 0).handler_address !=
                kObservedRouteFlagClearReturnAddress) {
            throw std::runtime_error("native event router mismatch");
        }
        const auto state_update = trace_flag_clear_handler(0xFFFF);
        if (state_update.first_selector != kObservedDriverFirstSelector ||
            !state_update.second_call_issued ||
            state_update.second_selector != kObservedDriverSecondSelector ||
            state_update.state_and_mask != kObservedDriverStateAndMask ||
            state_update.record_field_source != kObservedDriverSourceRamAddress ||
            state_update.timeout_field_value != 0xFFFF ||
            state_update.cleanup_handler_address != kObservedFlagClearCleanupAddress ||
            !state_update.cleanup_clears_record_fields) {
            throw std::runtime_error("native flag-clear trace mismatch");
        }
        std::cout << "verified event producer and raw router ranges\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
