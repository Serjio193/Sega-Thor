#include "game/ui/debug_warp_menu.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game::ui {

namespace {

[[nodiscard]] std::uint16_t read_be_u16(
    std::span<const std::uint8_t> rom,
    std::size_t offset) {
    if (offset + 2U > rom.size()) {
        throw std::out_of_range("DebugWarpSpawnTable: u16 read out of ROM bounds");
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom[offset]) << 8U) |
        static_cast<std::uint16_t>(rom[offset + 1U]));
}

[[nodiscard]] std::int16_t read_be_s16(
    std::span<const std::uint8_t> rom,
    std::size_t offset) {
    return static_cast<std::int16_t>(read_be_u16(rom, offset));
}

[[nodiscard]] std::uint8_t step_wrapped_room(
    std::uint8_t room_id,
    std::uint16_t max_room_id,
    int delta) noexcept {
    if (delta < 0) {
        return (room_id == 0U)
            ? static_cast<std::uint8_t>(max_room_id)
            : static_cast<std::uint8_t>(room_id - 1U);
    }
    return (static_cast<std::uint16_t>(room_id) >= max_room_id)
        ? 0U
        : static_cast<std::uint8_t>(room_id + 1U);
}

[[nodiscard]] std::uint8_t step_nonempty_area(
    const DebugWarpSpawnTable& table,
    std::uint8_t area_id,
    int delta) noexcept {
    std::uint8_t candidate = area_id;
    for (std::size_t i = 0U; i < kDebugWarpAreaSlotCount; ++i) {
        if (delta < 0) {
            candidate = (candidate == 0U) ? kDebugWarpMaxAreaIndex
                                          : static_cast<std::uint8_t>(candidate - 1U);
        } else {
            candidate = (candidate >= kDebugWarpMaxAreaIndex)
                ? 0U
                : static_cast<std::uint8_t>(candidate + 1U);
        }
        if (table.has_area(candidate)) {
            return candidate;
        }
    }
    return area_id;
}

} // namespace

DebugWarpSpawnTable DebugWarpSpawnTable::parse_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint32_t table_offset) {
    DebugWarpSpawnTable table;
    const std::size_t base = table_offset;
    for (std::size_t area_id = 0U; area_id < kDebugWarpAreaSlotCount; ++area_id) {
        const std::size_t slot_off = base + area_id * 2U;
        const std::int16_t rel = read_be_s16(rom, slot_off);
        if (rel == 0) {
            continue;
        }
        const std::ptrdiff_t area_off_signed =
            static_cast<std::ptrdiff_t>(slot_off) + static_cast<std::ptrdiff_t>(rel);
        if (area_off_signed < 0 || static_cast<std::size_t>(area_off_signed) + 2U > rom.size()) {
            throw std::out_of_range("DebugWarpSpawnTable: area offset out of ROM bounds");
        }
        const auto area_off = static_cast<std::size_t>(area_off_signed);
        const std::uint16_t max_room = read_be_u16(rom, area_off);

        DebugWarpAreaInfo area_info{};
        area_info.area_id = static_cast<std::uint8_t>(area_id);
        area_info.max_room_id = max_room;

        for (std::uint16_t room_id = 0U; room_id <= max_room; ++room_id) {
            const std::size_t room_slot_off = area_off + 2U + static_cast<std::size_t>(room_id) * 2U;
            const std::int16_t r_rel = read_be_s16(rom, room_slot_off);
            if (r_rel == 0) {
                continue;
            }
            const std::ptrdiff_t r_off_signed =
                static_cast<std::ptrdiff_t>(room_slot_off) + static_cast<std::ptrdiff_t>(r_rel);
            if (r_off_signed < 0 || static_cast<std::size_t>(r_off_signed) + 2U > rom.size()) {
                throw std::out_of_range("DebugWarpSpawnTable: room offset out of ROM bounds");
            }
            const auto r_off = static_cast<std::size_t>(r_off_signed);
            DebugWarpSpawnEntry entry{};
            entry.area_id = static_cast<std::uint8_t>(area_id);
            entry.room_id = static_cast<std::uint8_t>(room_id);
            entry.spawn_x_px = static_cast<std::uint16_t>(rom[r_off]) << kDebugWarpSpawnTileShift;
            entry.spawn_y_px = static_cast<std::uint16_t>(rom[r_off + 1U]) << kDebugWarpSpawnTileShift;
            area_info.rooms.push_back(entry);
        }
        table.areas_[area_id] = std::move(area_info);
    }
    return table;
}

DebugWarpSpawnTable DebugWarpSpawnTable::from_areas(std::vector<DebugWarpAreaInfo> areas) {
    DebugWarpSpawnTable table;
    for (auto& area : areas) {
        if (area.area_id <= kDebugWarpMaxAreaIndex) {
            table.areas_[area.area_id] = std::move(area);
        }
    }
    return table;
}

bool DebugWarpSpawnTable::has_area(std::uint8_t area_id) const noexcept {
    return area_id <= kDebugWarpMaxAreaIndex && areas_[area_id].has_value();
}

std::uint16_t DebugWarpSpawnTable::max_room_id(std::uint8_t area_id) const noexcept {
    if (!has_area(area_id)) {
        return 0U;
    }
    return areas_[area_id]->max_room_id;
}

std::optional<DebugWarpSpawnEntry> DebugWarpSpawnTable::find_spawn(
    std::uint8_t area_id,
    std::uint8_t room_id) const noexcept {
    if (!has_area(area_id)) {
        return std::nullopt;
    }
    for (const auto& entry : areas_[area_id]->rooms) {
        if (entry.room_id == room_id) {
            return entry;
        }
    }
    return std::nullopt;
}

std::size_t DebugWarpSpawnTable::active_area_count() const noexcept {
    std::size_t count = 0U;
    for (const auto& area : areas_) {
        if (area.has_value()) {
            ++count;
        }
    }
    return count;
}

DebugWarpStepResult step_debug_warp_menu(
    const DebugWarpSpawnTable& table,
    DebugWarpMenuState state,
    std::uint16_t joypad_word,
    std::uint8_t cancel_flags) noexcept {
    DebugWarpStepResult result{};
    result.next_state = state;

    if ((cancel_flags & kDebugWarpBtnCancelMask) != 0U) {
        result.outcome = DebugWarpOutcome::Cancelled;
        result.sound_cmd = kDebugWarpCancelSfx;
        return result;
    }

    if (state.repeat_timer > 0U) {
        result.next_state.repeat_timer = static_cast<std::uint16_t>(state.repeat_timer - 1U);
        return result;
    }

    const std::uint16_t max_room = table.max_room_id(state.area_id);
    if ((joypad_word & kDebugWarpBtnUp) != 0U) {
        result.next_state.room_id = step_wrapped_room(state.room_id, max_room, -1);
        result.next_state.repeat_timer = kDebugWarpRepeatInitialFrames;
        result.sound_cmd = kDebugWarpMoveSfx;
        return result;
    }
    if ((joypad_word & kDebugWarpBtnDown) != 0U) {
        result.next_state.room_id = step_wrapped_room(state.room_id, max_room, 1);
        result.next_state.repeat_timer = kDebugWarpRepeatInitialFrames;
        result.sound_cmd = kDebugWarpMoveSfx;
        return result;
    }

    auto apply_area_step = [&](int delta, std::uint16_t repeat_delay) {
        const std::uint8_t next_area = step_nonempty_area(table, state.area_id, delta);
        const std::uint16_t next_max_room = table.max_room_id(next_area);
        result.next_state.area_id = next_area;
        result.next_state.room_id = static_cast<std::uint8_t>(
            std::min<std::uint16_t>(state.room_id, next_max_room));
        result.next_state.repeat_timer = repeat_delay;
        result.sound_cmd = kDebugWarpMoveSfx;
    };

    if ((joypad_word & kDebugWarpBtnRight) != 0U) {
        apply_area_step(1, kDebugWarpRepeatInitialFrames);
        return result;
    }
    if ((joypad_word & kDebugWarpBtnLeft) != 0U) {
        apply_area_step(-1, kDebugWarpRepeatInitialFrames);
        return result;
    }
    if ((joypad_word & kDebugWarpBtnA) != 0U) {
        if (const auto spawn = table.find_spawn(state.area_id, state.room_id)) {
            result.outcome = DebugWarpOutcome::ConfirmedWarp;
            result.sound_cmd = kDebugWarpConfirmSfx;
            result.warp_target = spawn;
        }
        return result;
    }
    if ((joypad_word & kDebugWarpBtnUpHeld) != 0U) {
        result.next_state.room_id = step_wrapped_room(state.room_id, max_room, -1);
        result.next_state.repeat_timer = kDebugWarpRepeatHeldFrames;
        result.sound_cmd = kDebugWarpMoveSfx;
        return result;
    }
    if ((joypad_word & kDebugWarpBtnDownHeld) != 0U) {
        result.next_state.room_id = step_wrapped_room(state.room_id, max_room, 1);
        result.next_state.repeat_timer = kDebugWarpRepeatHeldFrames;
        result.sound_cmd = kDebugWarpMoveSfx;
        return result;
    }
    if ((joypad_word & kDebugWarpBtnRightHeld) != 0U) {
        apply_area_step(1, kDebugWarpRepeatHeldFrames);
        return result;
    }
    if ((joypad_word & kDebugWarpBtnLeftHeld) != 0U) {
        apply_area_step(-1, kDebugWarpRepeatHeldFrames);
        return result;
    }

    return result;
}

} // namespace oasis::game::ui
