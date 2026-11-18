#include "game/map/map_scene_dispatcher.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::map;

static void test_constants_and_addresses() {
    static_assert(MapSceneDispatcherConfig::kEntryDialogueCallerAnimAddr == 0x00C88CU);
    static_assert(MapSceneDispatcherConfig::kEntryLoadSceneScriptAddr == 0x00C8AAU);
    static_assert(MapSceneDispatcherConfig::kEntryGetScriptPointerAddr == 0x00C90EU);
    static_assert(MapSceneDispatcherConfig::kEntryAreaTableAddr == 0x00C92CU);
    static_assert(MapSceneDispatcherConfig::kEntryExecScriptTickAddr == 0x00C980U);
    static_assert(MapSceneDispatcherConfig::kEntryExecScriptSubtickAddr == 0x00C998U);
    static_assert(MapSceneDispatcherConfig::kRangeEndAddr == 0x00C9B2U);
    static_assert(MapSceneDispatcherConfig::kRoutineSizeBytes == 294U);

    static_assert(MapSceneDispatcherConfig::kAreaTableCount == 21U);
    static_assert(MapSceneDispatcherConfig::kScriptInitOffset == 0x1AU);
    static_assert(MapSceneDispatcherConfig::kCallerAnimHookAddress == 0x00C8A8U);

    static_assert(MapSceneDispatcherConfig::kAreaTable[0] == 0x0002C6B8U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[1] == 0x0002E56AU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[2] == 0x0002F1A4U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[3] == 0x0002F9B2U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[4] == 0x0002FF80U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[5] == 0x00030218U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[6] == 0x00030E56U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[7] == 0x00031B50U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[8] == 0x000328D6U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[9] == 0x0003361CU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[10] == 0x000341A4U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[11] == 0x00035528U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[12] == 0x00036286U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[13] == 0x00037B42U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[14] == 0x00038AACU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[15] == 0x0002C6F2U);
    static_assert(MapSceneDispatcherConfig::kAreaTable[16] == 0x00038FBAU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[17] == 0x0003957CU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[18] == 0x00039F7AU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[19] == 0x00039F7CU);
    static_assert(MapSceneDispatcherConfig::kAreaTable[20] == 0x00039F7EU);

    static_assert(MapSceneDispatcherConfig::kCurrentMapIdRam == 0x00FF0E40U);
    static_assert(MapSceneDispatcherConfig::kAreaStateFlagsRam == 0x00FF0E2CU);
    static_assert(MapSceneDispatcherConfig::kMapStateFlagsRam == 0x00FF0E1CU);
    static_assert(MapSceneDispatcherConfig::kAreaTriggerFlagsRam == 0x00FF0DB3U);
    static_assert(MapSceneDispatcherConfig::kActiveScriptPtrRam == 0x00FF16ECU);
    static_assert(MapSceneDispatcherConfig::kScriptPauseFlagsRam == 0x00FF17B8U);
    static_assert(MapSceneDispatcherConfig::kDialogueCallerEntityRam == 0x00FF1882U);
    static_assert(MapSceneDispatcherConfig::kCallerAnimHookPtrRam == 0x00FF193CU);

    std::cout << "[PASS] test_constants_and_addresses\n";
}

static void test_resolve_scene_script_pointer() {
    std::vector<std::uint8_t> rom(0x40000U, 0U);

    auto write_rom16 = [&](std::size_t addr, std::int16_t val) {
        rom[addr] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFFU);
        rom[addr + 1U] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    };

    // Area 00 (base 0x02C6B8), Scene 0: offset +0x0100
    write_rom16(0x02C6B8U, 0x0100);
    const std::uint32_t ptr0 = resolve_scene_script_pointer(rom, 0x0000U);
    assert(ptr0 == 0x02C6B8U + 0x0100U);

    // Area 00, Scene 1: offset -0x0020
    write_rom16(0x02C6B8U + 2U, -0x0020);
    const std::uint32_t ptr1 = resolve_scene_script_pointer(rom, 0x0001U);
    assert(ptr1 == (0x02C6B8U + 2U) - 0x0020U);

    // Area 03 (base 0x02F9B2), Scene 4: offset +0x0080
    write_rom16(0x02F9B2U + 8U, 0x0080);
    const std::uint32_t ptr3 = resolve_scene_script_pointer(rom, 0x0304U);
    assert(ptr3 == 0x02F9B2U + 8U + 0x0080U);

    // Area 20 (Area 14, base 0x039F7E), Scene 0: offset 0x0010
    write_rom16(0x039F7EU, 0x0010);
    const std::uint32_t ptr20 = resolve_scene_script_pointer(rom, 0x1400U);
    assert(ptr20 == 0x039F7EU + 0x0010U);

    // Invalid Area ID >= 21
    assert(resolve_scene_script_pointer(rom, 0x1500U) == 0U);
    assert(resolve_scene_script_pointer(rom, 0xFF00U) == 0U);

    std::cout << "[PASS] test_resolve_scene_script_pointer\n";
}

static void test_load_scene_script_state_transitions() {
    std::vector<std::uint8_t> rom(0x40000U, 0U);
    auto write_rom16 = [&](std::size_t addr, std::int16_t val) {
        rom[addr] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFFU);
        rom[addr + 1U] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    };

    // Script offsets
    write_rom16(0x02C6B8U, 0x0200);      // Area 00 Scene 00 -> 0x02C8B8
    write_rom16(0x02C6B8U + 2U, 0x0300); // Area 00 Scene 01 -> 0x02C9BA
    write_rom16(0x02E56AU, 0x0150);      // Area 01 Scene 00 -> 0x02E6BA

    MapState state;
    state.current_id = 0x0000U;
    state.area_flags.fill(0xAAU);
    state.map_flags.fill(0xBBU);
    state.trigger_flags.fill(0xCCU);

    // Case 1: Target ID == Current ID -> No flags cleared
    std::uint32_t init_entry = load_scene_script(state, rom, 0x0000U);
    assert(init_entry == (0x02C6B8U + 0x0200U + 0x1AU));
    assert(state.current_id == 0x0000U);
    assert(state.area_flags[0] == 0xAAU);
    assert(state.map_flags[0] == 0xBBU);
    assert(state.trigger_flags[0] == 0xCCU);

    // Case 2: Target ID changed, but Area SAME (0x0000 -> 0x0001)
    // Area flags cleared, Map & Trigger flags preserved
    init_entry = load_scene_script(state, rom, 0x0001U);
    assert(init_entry == (0x02C6B8U + 2U + 0x0300U + 0x1AU));
    assert(state.current_id == 0x0001U);
    for (std::size_t i = 0; i < 16; ++i) {
        assert(state.area_flags[i] == 0U);
        assert(state.map_flags[i] == 0xBBU);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        assert(state.trigger_flags[i] == 0xCCU);
    }

    // Set some area flags again
    state.area_flags.fill(0x55U);

    // Case 3: Area CHANGED (0x0001 -> 0x0100)
    // Area flags, Map flags, and Trigger flags all cleared!
    init_entry = load_scene_script(state, rom, 0x0100U);
    assert(init_entry == (0x02E56AU + 0x0150U + 0x1AU));
    assert(state.current_id == 0x0100U);
    for (std::size_t i = 0; i < 16; ++i) {
        assert(state.area_flags[i] == 0U);
        assert(state.map_flags[i] == 0U);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        assert(state.trigger_flags[i] == 0U);
    }

    std::cout << "[PASS] test_load_scene_script_state_transitions\n";
}

static void test_script_tick_and_subtick_targets() {
    std::vector<std::uint8_t> rom(0x40000U, 0U);
    auto write_rom16 = [&](std::size_t addr, std::int16_t val) {
        rom[addr] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFFU);
        rom[addr + 1U] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    };

    const std::uint32_t script_ptr = 0x0002C8B8U;
    write_rom16(script_ptr, 0x0040);       // Tick offset: +0x40
    write_rom16(script_ptr + 2U, 0x0050);  // Subtick offset: +0x50

    const std::uint32_t tick_tgt = get_script_tick_target(rom, script_ptr);
    assert(tick_tgt == script_ptr + 0x0040U);

    const std::uint32_t subtick_tgt = get_script_subtick_target(rom, script_ptr);
    assert(subtick_tgt == (script_ptr + 2U) + 0x0050U);

    // Negative offsets
    write_rom16(script_ptr, -0x0018);
    assert(get_script_tick_target(rom, script_ptr) == script_ptr - 0x0018U);

    std::cout << "[PASS] test_script_tick_and_subtick_targets\n";
}

static void test_script_pause_evaluation() {
    MapState state;
    state.script_paused = false;
    assert(can_tick_script(state));

    state.script_paused = true;
    assert(!can_tick_script(state));

    std::cout << "[PASS] test_script_pause_evaluation\n";
}

static void test_update_dialogue_caller_anim() {
    std::vector<std::uint8_t> ram(0x10000U, 0U);

    auto write_be32 = [&](std::uint32_t addr, std::uint32_t val) {
        const std::size_t off = addr & 0xFFFFU;
        ram[off] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        ram[off + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        ram[off + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        ram[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    };

    auto read_be32 = [&](std::uint32_t addr) -> std::uint32_t {
        const std::size_t off = addr & 0xFFFFU;
        return (static_cast<std::uint32_t>(ram[off]) << 24) |
               (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
               (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
               static_cast<std::uint32_t>(ram[off + 3U]);
    };

    // Caller ptr is null -> returns false, hook not set
    assert(!update_dialogue_caller_anim(ram));
    assert(read_be32(MapSceneDispatcherConfig::kCallerAnimHookPtrRam) == 0U);

    // Caller ptr set to 0x00FF2000
    write_be32(MapSceneDispatcherConfig::kDialogueCallerEntityRam, 0x00FF2000U);
    bool cb_called = false;
    std::uint32_t passed_caller = 0U;

    const bool result = update_dialogue_caller_anim(ram, [&](std::uint32_t ptr) {
        cb_called = true;
        passed_caller = ptr;
    });

    assert(result);
    assert(cb_called);
    assert(passed_caller == 0x00FF2000U);
    assert(read_be32(MapSceneDispatcherConfig::kCallerAnimHookPtrRam) ==
           MapSceneDispatcherConfig::kCallerAnimHookAddress);

    std::cout << "[PASS] test_update_dialogue_caller_anim\n";
}

static void test_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U);
    std::vector<std::uint8_t> rom(0x40000U, 0U);

    auto write_be16 = [&](std::uint32_t addr, std::uint16_t val) {
        const std::size_t off = addr & 0xFFFFU;
        ram[off] = static_cast<std::uint8_t>(val >> 8);
        ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    };

    auto read_be16 = [&](std::uint32_t addr) -> std::uint16_t {
        const std::size_t off = addr & 0xFFFFU;
        return static_cast<std::uint16_t>((ram[off] << 8) | ram[off + 1U]);
    };

    auto read_be32 = [&](std::uint32_t addr) -> std::uint32_t {
        const std::size_t off = addr & 0xFFFFU;
        return (static_cast<std::uint32_t>(ram[off]) << 24) |
               (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
               (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
               static_cast<std::uint32_t>(ram[off + 3U]);
    };

    auto write_rom16 = [&](std::size_t addr, std::int16_t val) {
        rom[addr] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFFU);
        rom[addr + 1U] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    };

    // Area 02 base: 0x0002F1A4
    write_rom16(0x0002F1A4U, 0x0120); // Scene 0 -> 0x0002F2C4
    // Script at 0x0002F2C4
    write_rom16(0x0002F2C4U, 0x0030);      // Tick -> 0x0002F2F4
    write_rom16(0x0002F2C4U + 2U, 0x0040); // Subtick -> 0x0002F306

    // Initial RAM setup: current ID = 0x0100 (Area 1)
    write_be16(MapSceneDispatcherConfig::kCurrentMapIdRam, 0x0100U);
    for (std::size_t i = 0; i < 16; ++i) {
        ram[(MapSceneDispatcherConfig::kAreaStateFlagsRam & 0xFFFFU) + i] = 0x11U;
        ram[(MapSceneDispatcherConfig::kMapStateFlagsRam & 0xFFFFU) + i] = 0x22U;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        ram[(MapSceneDispatcherConfig::kAreaTriggerFlagsRam & 0xFFFFU) + i] = 0x33U;
    }

    // Load Area 02 Scene 00 (0x0200) -> Area change
    const std::uint32_t init_entry = load_map_scene_script_raw(ram, rom, 0x0200U);
    assert(init_entry == (0x0002F2C4U + 0x1AU));
    assert(read_be16(MapSceneDispatcherConfig::kCurrentMapIdRam) == 0x0200U);
    assert(read_be32(MapSceneDispatcherConfig::kActiveScriptPtrRam) == 0x0002F2C4U);

    // Verify all 3 flag buffers zeroed
    for (std::size_t i = 0; i < 16; ++i) {
        assert(ram[(MapSceneDispatcherConfig::kAreaStateFlagsRam & 0xFFFFU) + i] == 0U);
        assert(ram[(MapSceneDispatcherConfig::kMapStateFlagsRam & 0xFFFFU) + i] == 0U);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        assert(ram[(MapSceneDispatcherConfig::kAreaTriggerFlagsRam & 0xFFFFU) + i] == 0U);
    }

    // Test raw tick when NOT paused
    ram[MapSceneDispatcherConfig::kScriptPauseFlagsRam & 0xFFFFU] = 0x00U;
    assert(exec_map_script_tick_raw(ram, rom) == 0x0002F2C4U + 0x0030U);
    assert(exec_map_script_subtick_raw(ram, rom) == (0x0002F2C4U + 2U) + 0x0040U);

    // Test raw tick when paused (bit 1 set: 0x02)
    ram[MapSceneDispatcherConfig::kScriptPauseFlagsRam & 0xFFFFU] = 0x02U;
    assert(exec_map_script_tick_raw(ram, rom) == 0U);
    assert(exec_map_script_subtick_raw(ram, rom) == 0U);

    std::cout << "[PASS] test_ram_integration\n";
}

static void test_thor_script_namespace_aliases() {
    thor::script::MapState st;
    st.current_id = 0x0501U;
    st.script_paused = false;

    assert(thor::script::can_tick_script(st));

    std::cout << "[PASS] test_thor_script_namespace_aliases\n";
}

int main() {
    std::cout << "Starting M16.180 Map Scene Dispatcher tests...\n";
    test_constants_and_addresses();
    test_resolve_scene_script_pointer();
    test_load_scene_script_state_transitions();
    test_script_tick_and_subtick_targets();
    test_script_pause_evaluation();
    test_update_dialogue_caller_anim();
    test_ram_integration();
    test_thor_script_namespace_aliases();
    std::cout << "All M16.180 Map Scene Dispatcher tests PASS!\n";
    return 0;
}
