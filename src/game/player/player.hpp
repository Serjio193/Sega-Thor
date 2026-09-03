#pragma once

#include "core/runtime.hpp"
#include "game/world/byte_grid.hpp"

#include <cstdint>

namespace oasis::game::player {

inline constexpr std::uint32_t kPlayerEntityRamAddress = 0x00FF19E8;
inline constexpr std::uint32_t kPlayerUpdateRoutineAddress = 0x0000557A;
inline constexpr std::uint32_t kPlayerStateDispatchTableAddress = 0x000059BA;
inline constexpr std::uint32_t kPlayerMovementIntentRoutineAddress = 0x000061F6;
inline constexpr std::uint32_t kPlayerDirectionRoutineAddress = 0x000085E2;
inline constexpr std::uint32_t kPlayerMovementRoutineAddress = 0x00008F12;
inline constexpr std::uint32_t kPlayerFootprintRoutineAddress = 0x00009BF2;
inline constexpr std::uint32_t kPlayerTerrainGateAddress = 0x0000938E;

enum class Direction : std::uint8_t {
    none,
    up,
    down,
    left,
    right,
    up_left,
    up_right,
    down_left,
    down_right,
};

struct PlayerMovementConfig {
    // These are 16.16 fixed-point deltas recovered from 0x85E2.
    std::int32_t cardinal_x_speed{0x36000};
    std::int32_t cardinal_y_speed{0x30000};
    std::int32_t diagonal_x_speed{0x2A000};
    std::int32_t diagonal_y_speed{0x25800};
    std::uint16_t footprint_radius{10};
    std::uint8_t entity_flags{};
};

struct PlayerState {
    // Positions use the same 16.16 fixed-point convention as movement deltas.
    std::int32_t x_fixed{};
    std::int32_t y_fixed{};
    std::int8_t terrain_state{-1};
    // Entity +0x04 selects the movement/update state in the 0x59BA table.
    std::uint16_t movement_state{};
};

struct MovementVector {
    Direction direction{Direction::none};
    std::int32_t x_fixed{};
    std::int32_t y_fixed{};
};

struct MovementResult {
    MovementVector vector{};
    bool moved{};
    bool blocked{};
};

[[nodiscard]] Direction direction_from_input(std::uint8_t input_nibble) noexcept;

[[nodiscard]] MovementVector movement_vector(
    std::uint8_t input_nibble,
    const PlayerMovementConfig& config = {}) noexcept;

[[nodiscard]] MovementResult try_move(
    PlayerState& player,
    const core::ControllerState& controller,
    const world::ByteGridView& terrain,
    const PlayerMovementConfig& config = {}) noexcept;

} // namespace oasis::game::player
