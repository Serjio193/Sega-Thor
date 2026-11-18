#include "map_scene_dispatcher.hpp"

namespace oasis::game::map {

namespace {

inline std::int16_t read_be16_signed(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    if (offset + 1U >= data.size()) {
        return 0;
    }
    const std::uint16_t u = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8) |
        static_cast<std::uint16_t>(data[offset + 1U]));
    return static_cast<std::int16_t>(u);
}

inline std::uint16_t read_be16(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ram[off]) << 8) |
                                      static_cast<std::uint16_t>(ram[off + 1U]));
}

inline void write_be16(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint32_t read_be32(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(ram[off]) << 24) |
           (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
           (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
           static_cast<std::uint32_t>(ram[off + 3U]);
}

inline void write_be32(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint32_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
    ram[off + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint8_t read_u8(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return 0U;
    }
    return ram[off];
}

} // namespace

std::uint32_t resolve_scene_script_pointer(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept {
    const std::uint8_t area_id = static_cast<std::uint8_t>((packed_id >> 8) & 0xFFU);
    const std::uint8_t scene_id = static_cast<std::uint8_t>(packed_id & 0xFFU);

    if (area_id >= MapSceneDispatcherConfig::kAreaTableCount) {
        return 0U;
    }

    const std::uint32_t area_base = MapSceneDispatcherConfig::kAreaTable[area_id];
    const std::size_t table_entry_addr = static_cast<std::size_t>(area_base + scene_id * 2U);
    if (table_entry_addr + 2U > rom.size()) {
        return 0U;
    }

    const std::int16_t scene_offset = read_be16_signed(rom, table_entry_addr);
    const auto script_ptr = static_cast<std::int32_t>(table_entry_addr) + scene_offset;
    return static_cast<std::uint32_t>(script_ptr);
}

std::uint32_t load_scene_script(
    MapState& state,
    std::span<const std::uint8_t> rom,
    std::uint16_t target_id) {
    if (target_id != state.current_id) {
        state.area_flags.fill(0U);

        const std::uint8_t target_area = static_cast<std::uint8_t>((target_id >> 8) & 0xFFU);
        const std::uint8_t current_area = static_cast<std::uint8_t>((state.current_id >> 8) & 0xFFU);
        if (target_area != current_area) {
            state.map_flags.fill(0U);
            state.trigger_flags.fill(0U);
        }

        state.current_id = target_id;
    }

    const std::uint32_t script_ptr = resolve_scene_script_pointer(rom, target_id);
    state.active_script_ptr = script_ptr;
    return script_ptr + MapSceneDispatcherConfig::kScriptInitOffset;
}

bool can_tick_script(const MapState& state) noexcept {
    return !state.script_paused;
}

std::uint32_t get_script_tick_target(
    std::span<const std::uint8_t> rom,
    std::uint32_t script_ptr) noexcept {
    const std::size_t off = static_cast<std::size_t>(script_ptr);
    if (off + 2U > rom.size()) {
        return 0U;
    }

    const std::int16_t offset = read_be16_signed(rom, off);
    const auto target = static_cast<std::int32_t>(script_ptr) + offset;
    return static_cast<std::uint32_t>(target);
}

std::uint32_t get_script_subtick_target(
    std::span<const std::uint8_t> rom,
    std::uint32_t script_ptr) noexcept {
    const std::size_t off = static_cast<std::size_t>(script_ptr + 2U);
    if (off + 2U > rom.size()) {
        return 0U;
    }

    const std::int16_t offset = read_be16_signed(rom, off);
    const auto target = static_cast<std::int32_t>(script_ptr + 2U) + offset;
    return static_cast<std::uint32_t>(target);
}

bool update_dialogue_caller_anim(
    std::span<std::uint8_t> ram,
    const std::function<void(std::uint32_t caller_ptr)>& anim_callback) {
    if (ram.size() < 0x10000U) {
        return false;
    }

    const std::uint32_t caller_ptr = read_be32(ram, MapSceneDispatcherConfig::kDialogueCallerEntityRam);
    if (caller_ptr == 0U) {
        return false;
    }

    write_be32(ram, MapSceneDispatcherConfig::kCallerAnimHookPtrRam,
               MapSceneDispatcherConfig::kCallerAnimHookAddress);
    if (anim_callback) {
        anim_callback(caller_ptr);
    }
    return true;
}

std::uint32_t load_map_scene_script_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom,
    std::uint16_t target_id) {
    if (ram.size() < 0x10000U) {
        return 0U;
    }

    const std::uint16_t current_id = read_be16(ram, MapSceneDispatcherConfig::kCurrentMapIdRam);
    if (target_id != current_id) {
        // Clear 16-byte area flags $FF0E2C..$FF0E3C
        const std::size_t area_flags_off = MapSceneDispatcherConfig::kAreaStateFlagsRam & 0xFFFFU;
        for (std::size_t i = 0; i < 16U; ++i) {
            ram[area_flags_off + i] = 0U;
        }

        const std::uint8_t target_area = static_cast<std::uint8_t>((target_id >> 8) & 0xFFU);
        const std::uint8_t current_area = static_cast<std::uint8_t>((current_id >> 8) & 0xFFU);
        if (target_area != current_area) {
            // Clear 16-byte map flags $FF0E1C..$FF0E2C
            const std::size_t map_flags_off = MapSceneDispatcherConfig::kMapStateFlagsRam & 0xFFFFU;
            for (std::size_t i = 0; i < 16U; ++i) {
                ram[map_flags_off + i] = 0U;
            }

            // Clear 3-byte trigger flags $FF0DB3..$FF0DB6
            const std::size_t trigger_flags_off = MapSceneDispatcherConfig::kAreaTriggerFlagsRam & 0xFFFFU;
            for (std::size_t i = 0; i < 3U; ++i) {
                ram[trigger_flags_off + i] = 0U;
            }
        }

        write_be16(ram, MapSceneDispatcherConfig::kCurrentMapIdRam, target_id);
    }

    const std::uint32_t script_ptr = resolve_scene_script_pointer(rom, target_id);
    write_be32(ram, MapSceneDispatcherConfig::kActiveScriptPtrRam, script_ptr);
    return script_ptr + MapSceneDispatcherConfig::kScriptInitOffset;
}

std::uint32_t exec_map_script_tick_raw(
    std::span<const std::uint8_t> ram,
    std::span<const std::uint8_t> rom) {
    if (ram.size() < 0x10000U) {
        return 0U;
    }

    const std::uint8_t pause_flags = read_u8(ram, MapSceneDispatcherConfig::kScriptPauseFlagsRam);
    if ((pause_flags & 0x02U) != 0U) {
        return 0U; // Paused
    }

    const std::uint32_t script_ptr = read_be32(ram, MapSceneDispatcherConfig::kActiveScriptPtrRam);
    return get_script_tick_target(rom, script_ptr);
}

std::uint32_t exec_map_script_subtick_raw(
    std::span<const std::uint8_t> ram,
    std::span<const std::uint8_t> rom) {
    if (ram.size() < 0x10000U) {
        return 0U;
    }

    const std::uint8_t pause_flags = read_u8(ram, MapSceneDispatcherConfig::kScriptPauseFlagsRam);
    if ((pause_flags & 0x02U) != 0U) {
        return 0U; // Paused
    }

    const std::uint32_t script_ptr = read_be32(ram, MapSceneDispatcherConfig::kActiveScriptPtrRam);
    return get_script_subtick_target(rom, script_ptr);
}

} // namespace oasis::game::map
