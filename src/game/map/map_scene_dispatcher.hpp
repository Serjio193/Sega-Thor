#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace oasis::game::map {

/// State structure tracking area, map, trigger flags, and scene script lifecycle.
struct MapState {
    std::uint16_t current_id{0U};
    std::array<std::uint8_t, 16> area_flags{};
    std::array<std::uint8_t, 16> map_flags{};
    std::array<std::uint8_t, 3> trigger_flags{};
    std::uint32_t active_script_ptr{0U};
    bool script_paused{false};
};

/// Configuration and constants for ROM Map & Scene Script Dispatcher,
/// 21-Area Script Table & State Manager (0x00C88C..0x00C9B2, 294 bytes).
struct MapSceneDispatcherConfig {
    static constexpr std::uint32_t kEntryDialogueCallerAnimAddr = 0x00C88CU;
    static constexpr std::uint32_t kEntryLoadSceneScriptAddr = 0x00C8AAU;
    static constexpr std::uint32_t kEntryGetScriptPointerAddr = 0x00C90EU;
    static constexpr std::uint32_t kEntryAreaTableAddr = 0x00C92CU;
    static constexpr std::uint32_t kEntryExecScriptTickAddr = 0x00C980U;
    static constexpr std::uint32_t kEntryExecScriptSubtickAddr = 0x00C998U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C9B2U;
    static constexpr std::size_t kRoutineSizeBytes = 294U;

    static constexpr std::size_t kAreaTableCount = 21U; // Areas 00..14 (0..20)
    static constexpr std::uint32_t kScriptInitOffset = 0x1AU;
    static constexpr std::uint32_t kCallerAnimHookAddress = 0x00C8A8U;

    // 21-Area ROM base addresses in table 0x00C92C
    static constexpr std::array<std::uint32_t, 21> kAreaTable = {
        0x0002C6B8U, // Area 00
        0x0002E56AU, // Area 01
        0x0002F1A4U, // Area 02
        0x0002F9B2U, // Area 03
        0x0002FF80U, // Area 04
        0x00030218U, // Area 05
        0x00030E56U, // Area 06
        0x00031B50U, // Area 07
        0x000328D6U, // Area 08
        0x0003361CU, // Area 09
        0x000341A4U, // Area 0A
        0x00035528U, // Area 0B
        0x00036286U, // Area 0C
        0x00037B42U, // Area 0D
        0x00038AACU, // Area 0E
        0x0002C6F2U, // Area 0F
        0x00038FBAU, // Area 10
        0x0003957CU, // Area 11
        0x00039F7AU, // Area 12
        0x00039F7CU, // Area 13
        0x00039F7EU  // Area 14
    };

    // Memory map addresses
    static constexpr std::uint32_t kCurrentMapIdRam = 0x00FF0E40U;
    static constexpr std::uint32_t kAreaStateFlagsRam = 0x00FF0E2CU;
    static constexpr std::uint32_t kMapStateFlagsRam = 0x00FF0E1CU;
    static constexpr std::uint32_t kAreaTriggerFlagsRam = 0x00FF0DB3U;
    static constexpr std::uint32_t kActiveScriptPtrRam = 0x00FF16ECU;
    static constexpr std::uint32_t kScriptPauseFlagsRam = 0x00FF17B8U;
    static constexpr std::uint32_t kDialogueCallerEntityRam = 0x00FF1882U;
    static constexpr std::uint32_t kCallerAnimHookPtrRam = 0x00FF193CU;
};

/// Resolves 32-bit absolute ROM address of a scene script for a given packed area/scene ID.
[[nodiscard]] std::uint32_t resolve_scene_script_pointer(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept;

/// Loads scene script, updates MapState flags across area/map boundaries, and returns init entrypoint (+0x1A).
std::uint32_t load_scene_script(
    MapState& state,
    std::span<const std::uint8_t> rom,
    std::uint16_t target_id);

/// Evaluates if script ticking is allowed (checks pause bit 1).
[[nodiscard]] bool can_tick_script(const MapState& state) noexcept;

/// Resolves target ROM address for the main script tick callback at offset +0x00.
[[nodiscard]] std::uint32_t get_script_tick_target(
    std::span<const std::uint8_t> rom,
    std::uint32_t script_ptr) noexcept;

/// Resolves target ROM address for the script subtick callback at offset +0x02.
[[nodiscard]] std::uint32_t get_script_subtick_target(
    std::span<const std::uint8_t> rom,
    std::uint32_t script_ptr) noexcept;

/// Updates caller entity animation if caller entity pointer at $FF1882 is non-null (0x00C88C).
bool update_dialogue_caller_anim(
    std::span<std::uint8_t> ram,
    const std::function<void(std::uint32_t caller_ptr)>& anim_callback = {});

/// Raw 64KB RAM window integration helper for loading map scene script (0x00C8AA).
std::uint32_t load_map_scene_script_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom,
    std::uint16_t target_id);

/// Raw 64KB RAM window integration helper for executing script tick (0x00C980).
std::uint32_t exec_map_script_tick_raw(
    std::span<const std::uint8_t> ram,
    std::span<const std::uint8_t> rom);

/// Raw 64KB RAM window integration helper for executing script subtick (0x00C998).
std::uint32_t exec_map_script_subtick_raw(
    std::span<const std::uint8_t> ram,
    std::span<const std::uint8_t> rom);

} // namespace oasis::game::map

namespace thor::script {
using MapState = oasis::game::map::MapState;
using MapSceneDispatcherConfig = oasis::game::map::MapSceneDispatcherConfig;
using oasis::game::map::can_tick_script;
using oasis::game::map::get_script_subtick_target;
using oasis::game::map::get_script_tick_target;
using oasis::game::map::load_scene_script;
using oasis::game::map::resolve_scene_script_pointer;
} // namespace thor::script
