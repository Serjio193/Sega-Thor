#include "game/ui/debug_warp_menu.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace {

using namespace oasis::game::ui;

void write_be_u16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t val) {
    buf[off] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void write_be_s16(std::vector<std::uint8_t>& buf, std::size_t off, std::int16_t val) {
    write_be_u16(buf, off, static_cast<std::uint16_t>(val));
}

std::vector<std::uint8_t> build_synthetic_spawn_table_rom() {
    // Layout at offset 0:
    //   0x00..0x3F: 32 area relative s16 offsets (areas 0, 2, 31 populated; others 0)
    //   0x40..0x47: Area 0: max_room=1, room0 rel=0 (empty), room1 rel=4 -> (16, 16) -> (128, 128)
    //   0x48..0x51: Area 2: max_room=2, room0..2 -> (20, 15), (10, 12), (30, 40)
    //   0x52..0x57: Area 31: max_room=0, room0 -> (8, 9) -> (64, 72)
    std::vector<std::uint8_t> rom(0x60U, 0U);

    // Area 0 at 0x40 (slot at 0x00 -> rel = 0x40)
    write_be_s16(rom, 0x00U, 0x40);
    write_be_u16(rom, 0x40U, 1U); // max_room_id = 1
    write_be_s16(rom, 0x42U, 0);  // room 0 empty
    write_be_s16(rom, 0x44U, 2);  // room 1 at 0x44 + 2 = 0x46
    rom[0x46U] = 16U;             // 16 << 3 = 128
    rom[0x47U] = 16U;             // 16 << 3 = 128

    // Area 2 at 0x48 (slot at 0x04 -> rel = 0x44)
    write_be_s16(rom, 0x04U, 0x44);
    write_be_u16(rom, 0x48U, 2U); // max_room_id = 2
    write_be_s16(rom, 0x4AU, 6);  // room 0 at 0x50
    write_be_s16(rom, 0x4CU, 6);  // room 1 at 0x52
    write_be_s16(rom, 0x4EU, 6);  // room 2 at 0x54
    rom[0x50U] = 20U;
    rom[0x51U] = 15U;
    rom[0x52U] = 10U;
    rom[0x53U] = 12U;
    rom[0x54U] = 30U;
    rom[0x55U] = 40U;

    // Area 31 at 0x56 (slot at 0x3E -> rel = 0x18)
    write_be_s16(rom, 0x3EU, 0x18);
    write_be_u16(rom, 0x56U, 0U); // max_room_id = 0
    write_be_s16(rom, 0x58U, 2);  // room 0 at 0x5A
    rom[0x5AU] = 8U;
    rom[0x5BU] = 9U;

    return rom;
}

void test_parse_rom_spawn_table() {
    const auto rom = build_synthetic_spawn_table_rom();
    const auto table = DebugWarpSpawnTable::parse_from_rom(rom, 0U);

    assert(table.active_area_count() == 3U);
    assert(table.has_area(0U));
    assert(!table.has_area(1U));
    assert(table.has_area(2U));
    assert(table.has_area(31U));

    assert(table.max_room_id(0U) == 1U);
    assert(!table.find_spawn(0U, 0U).has_value());
    const auto a0r1 = table.find_spawn(0U, 1U);
    assert(a0r1.has_value());
    assert(a0r1->spawn_x_px == 128U);
    assert(a0r1->spawn_y_px == 128U);

    const auto a2r2 = table.find_spawn(2U, 2U);
    assert(a2r2.has_value());
    assert(a2r2->spawn_x_px == 240U);
    assert(a2r2->spawn_y_px == 320U);

    const auto a31r0 = table.find_spawn(31U, 0U);
    assert(a31r0.has_value());
    assert(a31r0->spawn_x_px == 64U);
    assert(a31r0->spawn_y_px == 72U);
}

void test_menu_navigation_repeat_and_warp() {
    const auto rom = build_synthetic_spawn_table_rom();
    const auto table = DebugWarpSpawnTable::parse_from_rom(rom, 0U);

    DebugWarpMenuState state{0U, 0U, 0U};

    // Up from room 0 in area 0 (max_room=1) wraps to 1, sets initial repeat = 10
    auto res = step_debug_warp_menu(table, state, kDebugWarpBtnUp);
    assert(res.outcome == DebugWarpOutcome::Pending);
    assert(res.next_state.area_id == 0U);
    assert(res.next_state.room_id == 1U);
    assert(res.next_state.repeat_timer == kDebugWarpRepeatInitialFrames);
    assert(res.sound_cmd == kDebugWarpMoveSfx);
    state = res.next_state;

    // While repeat_timer > 0, input is ignored and timer decrements
    res = step_debug_warp_menu(table, state, kDebugWarpBtnDown);
    assert(res.next_state.room_id == 1U);
    assert(res.next_state.repeat_timer == 9U);
    assert(res.sound_cmd == 0U);

    // Clear timer and test Right: skips empty area 1 -> lands on area 2
    state.repeat_timer = 0U;
    res = step_debug_warp_menu(table, state, kDebugWarpBtnRight);
    assert(res.next_state.area_id == 2U);
    assert(res.next_state.room_id == 1U);
    assert(res.next_state.repeat_timer == 10U);
    assert(res.sound_cmd == kDebugWarpMoveSfx);
    state = res.next_state;

    // Held Down: room 1 -> 2, repeat_timer = 3
    state.repeat_timer = 0U;
    res = step_debug_warp_menu(table, state, kDebugWarpBtnDownHeld);
    assert(res.next_state.room_id == 2U);
    assert(res.next_state.repeat_timer == kDebugWarpRepeatHeldFrames);
    assert(res.sound_cmd == kDebugWarpMoveSfx);
    state = res.next_state;

    // Held Right from area 2 -> skips empty 3..30 -> lands on 31, clamps room 2 -> 0
    state.repeat_timer = 0U;
    res = step_debug_warp_menu(table, state, kDebugWarpBtnRightHeld);
    assert(res.next_state.area_id == 31U);
    assert(res.next_state.room_id == 0U);
    assert(res.next_state.repeat_timer == 3U);
    state = res.next_state;

    // Left from area 31 wraps back to area 2
    state.repeat_timer = 0U;
    res = step_debug_warp_menu(table, state, kDebugWarpBtnLeft);
    assert(res.next_state.area_id == 2U);
    state = res.next_state;

    // A on valid room confirms warp with 0x00A00256
    state.repeat_timer = 0U;
    res = step_debug_warp_menu(table, state, kDebugWarpBtnA);
    assert(res.outcome == DebugWarpOutcome::ConfirmedWarp);
    assert(res.sound_cmd == kDebugWarpConfirmSfx);
    assert(res.warp_target.has_value());
    assert(res.warp_target->area_id == 2U);
    assert(res.warp_target->room_id == 0U);
    assert(res.warp_target->spawn_x_px == 160U);
    assert(res.warp_target->spawn_y_px == 120U);

    // A on empty room (area 0, room 0) does nothing
    res = step_debug_warp_menu(table, DebugWarpMenuState{0U, 0U, 0U}, kDebugWarpBtnA);
    assert(res.outcome == DebugWarpOutcome::Pending);
    assert(res.sound_cmd == 0U);

    // B/C cancels with 0x00A00347
    res = step_debug_warp_menu(table, state, 0U, kDebugWarpBtnCancelMask);
    assert(res.outcome == DebugWarpOutcome::Cancelled);
    assert(res.sound_cmd == kDebugWarpCancelSfx);
}

} // namespace

int main() {
    test_parse_rom_spawn_table();
    test_menu_navigation_repeat_and_warp();
    return 0;
}
