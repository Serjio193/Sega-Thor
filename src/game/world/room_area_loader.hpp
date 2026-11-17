#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kLoadRoomAndJumpAddr = 0x00C8AAU;
inline constexpr std::uint32_t kGetRoomHeaderAddr = 0x00C90EU;
inline constexpr std::uint32_t kAreaTableRomAddr = 0x00C92CU;
inline constexpr std::uint16_t kRoomScriptEntryOffset = 0x001AU;
inline constexpr std::uint16_t kNewGameRoomId = 0x001BU;
inline constexpr std::uint32_t kNewGameRoomHeaderAddr = 0x0002E1D8U;

/// 16-entry Area Pointer Table at ROM 0x00C92C..0x00C96B (Areas 0..15).
inline constexpr std::array<std::uint32_t, 16> kAreaTablePointers = {
    0x0002C6B8U, 0x0002E56AU, 0x0002F1A4U, 0x0002F9B2U,
    0x0002FF80U, 0x00030218U, 0x00030E56U, 0x00031B50U,
    0x000328D6U, 0x0003361CU, 0x000341A4U, 0x00035528U,
    0x00036286U, 0x00037B42U, 0x00038AACU, 0x0002C6F2U,
};

struct RoomAreaState {
    std::uint16_t current_room_id{0xFFFFU};      // $FF0E40
    std::array<std::uint8_t, 16> room_flags{};   // $FF0E2C..$FF0E3B
    std::array<std::uint8_t, 16> area_flags{};   // $FF0E1C..$FF0E2B
    std::array<std::uint8_t, 3> area_extra_3b{}; // $FF0DB3..$FF0DB5
    std::uint32_t current_room_header_ptr{0};    // $FF16EC
};

struct RoomTransitionOutcome {
    bool room_changed{false};
    bool area_changed{false};
};

/// Models 0x00C8AA..0x00C90C: clears 16B room_flags on room change, also clears
/// 16B area_flags + 3B area_extra_3b on area change, and updates current_room_id.
RoomTransitionOutcome transition_room_flags(
    RoomAreaState& state,
    std::uint16_t new_room_id) noexcept;

/// Models 0x00C90E..0x00C92A: resolves entry_addr = area_ptrs[area & 0x0F] + room * 2
/// and returns entry_addr + int16_be(rom[entry_addr]).
[[nodiscard]] std::optional<std::uint32_t> resolve_room_header_addr(
    std::span<const std::uint8_t> rom,
    std::span<const std::uint32_t, 16> area_ptrs,
    std::uint16_t room_id) noexcept;

} // namespace oasis::game::world
