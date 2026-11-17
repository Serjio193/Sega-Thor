#include "core/rom.hpp"
#include "game/world/room_area_loader.hpp"

#include <cassert>
#include <filesystem>
#include <vector>

namespace {

using namespace oasis::game::world;

void test_area_table_and_synthetic_resolution() {
    assert(kAreaTablePointers[0] == 0x0002C6B8U);
    assert(kAreaTablePointers[13] == 0x00037B42U);
    assert(kAreaTablePointers[15] == 0x0002C6F2U);

    // Synthetic ROM test for positive and negative int16_be relative offsets
    std::vector<std::uint8_t> rom(0x030000U, 0x00U);
    // Area 0, Room 0x1B -> entry_addr = 0x0002C6B8 + 0x1B * 2 = 0x0002C6EE
    const std::size_t entry = 0x0002C6B8U + 0x1BU * 2U;
    // 0x0002E1D8 - 0x0002C6EE = 0x1AEA
    rom[entry] = 0x1AU;
    rom[entry + 1U] = 0xEAU;
    const auto hdr = resolve_room_header_addr(rom, kAreaTablePointers, 0x001BU);
    assert(hdr.has_value() && *hdr == 0x0002E1D8U);

    // Negative offset: Area 1, Room 0 -> entry = 0x0002E56A, rel = -16 (0xFFF0)
    const std::size_t entry1 = 0x0002E56AU;
    rom[entry1] = 0xFFU;
    rom[entry1 + 1U] = 0xF0U;
    const auto hdr1 = resolve_room_header_addr(rom, kAreaTablePointers, 0x0100U);
    assert(hdr1.has_value() && *hdr1 == 0x0002E55AU);
}

void test_room_and_area_flag_clearing() {
    RoomAreaState st{};
    st.current_room_id = 0x001BU;
    st.room_flags.fill(0xAAU);
    st.area_flags.fill(0xBBU);
    st.area_extra_3b.fill(0xCCU);

    // Same room -> no flags cleared
    const auto same = transition_room_flags(st, 0x001BU);
    assert(!same.room_changed && !same.area_changed);
    assert(st.room_flags[0] == 0xAAU && st.area_flags[0] == 0xBBU && st.area_extra_3b[0] == 0xCCU);

    // Same area, different room -> clears room_flags only
    const auto same_area = transition_room_flags(st, 0x001CU);
    assert(same_area.room_changed && !same_area.area_changed);
    assert(st.current_room_id == 0x001CU);
    assert(st.room_flags[0] == 0U && st.room_flags[15] == 0U);
    assert(st.area_flags[0] == 0xBBU && st.area_extra_3b[0] == 0xCCU);

    // Cross area -> clears room_flags, area_flags, and area_extra_3b
    st.room_flags.fill(0x55U);
    const auto cross = transition_room_flags(st, 0x0D00U);
    assert(cross.room_changed && cross.area_changed);
    assert(st.current_room_id == 0x0D00U);
    assert(st.room_flags[0] == 0U && st.area_flags[0] == 0U && st.area_extra_3b[2] == 0U);
}

void test_real_rom_new_game_room_header_if_available() {
    const std::filesystem::path rom_path = "local-roms/Beyond Oasis (USA).md";
    if (!std::filesystem::exists(rom_path)) {
        return;
    }
    const auto rom = oasis::Rom::load(rom_path);
    const auto hdr = resolve_room_header_addr(rom.bytes(), kAreaTablePointers, kNewGameRoomId);
    assert(hdr.has_value() && *hdr == kNewGameRoomHeaderAddr);
}

} // namespace

int main() {
    test_area_table_and_synthetic_resolution();
    test_room_and_area_flag_clearing();
    test_real_rom_new_game_room_header_if_available();
    return 0;
}
