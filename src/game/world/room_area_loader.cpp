#include "game/world/room_area_loader.hpp"

namespace oasis::game::world {

RoomTransitionOutcome transition_room_flags(
    RoomAreaState& state,
    std::uint16_t new_room_id) noexcept {
    if (new_room_id == state.current_room_id) {
        return {false, false};
    }

    const bool area_changed =
        (new_room_id >> 8U) != (state.current_room_id >> 8U);
    state.room_flags.fill(0U);
    if (area_changed) {
        state.area_flags.fill(0U);
        state.area_extra_3b.fill(0U);
    }
    state.current_room_id = new_room_id;
    return {true, area_changed};
}

std::optional<std::uint32_t> resolve_room_header_addr(
    std::span<const std::uint8_t> rom,
    std::span<const std::uint32_t, 16> area_ptrs,
    std::uint16_t room_id) noexcept {
    const std::size_t area = static_cast<std::size_t>((room_id >> 8U) & 0x0FU);
    const std::uint32_t room = static_cast<std::uint32_t>(room_id & 0xFFU);
    const std::uint32_t entry_addr = area_ptrs[area] + room * 2U;
    if (static_cast<std::size_t>(entry_addr) + 2U > rom.size()) {
        return std::nullopt;
    }

    const std::uint16_t raw = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom[entry_addr]) << 8U) |
        static_cast<std::uint16_t>(rom[entry_addr + 1U]));
    const std::int16_t rel = static_cast<std::int16_t>(raw);
    const std::int64_t header =
        static_cast<std::int64_t>(entry_addr) + static_cast<std::int64_t>(rel);
    if (header < 0 || static_cast<std::size_t>(header) >= rom.size()) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(header);
}

} // namespace oasis::game::world
