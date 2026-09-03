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
inline constexpr std::uint32_t kPlayerVelocityAdjustRoutineAddress = 0x000064C4;
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

struct VelocityAdjustContext {
    // These names intentionally preserve the observed external RAM sources.
    bool available{};
    bool flag_ff1985{};
    bool flag_ff1984{};
    bool flag_ff16f1_bit4{};
    std::uint8_t footprint_any_bits{};
};

struct PlayerState {
    // Positions use the same 16.16 fixed-point convention as movement deltas.
    std::int32_t x_fixed{};
    std::int32_t y_fixed{};
    std::int8_t terrain_state{-1};
    // Entity +0x04 selects the movement/update state in the 0x59BA table.
    std::uint16_t movement_state{};
    // These fields mirror only offsets observed by the verified movement path.
    std::uint16_t direction_code{}; // entity +0x16
    std::uint8_t orientation_flags{}; // entity +0x17
    std::int32_t intent_x_fixed{}; // entity +0x4E
    std::int32_t intent_y_fixed{}; // entity +0x52
    std::int32_t accumulated_x_fixed{}; // entity +0x72
    std::int32_t accumulated_y_fixed{}; // entity +0x76
    std::uint8_t footprint_any_bits{}; // entity +0x6F, produced by 0x9BF2
    std::uint16_t turn_timer{}; // FF197E in the state-4 branch
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

// Update the confirmed input/state portion of the player path. Collision is
// intentionally left to try_move, which consumes the accumulated deltas.
[[nodiscard]] MovementVector update_movement_state(
    PlayerState& player,
    std::uint8_t input_nibble,
    const PlayerMovementConfig& config = {},
    const VelocityAdjustContext& velocity_context = {}) noexcept;

void adjust_velocity_for_context(
    std::int32_t& x_fixed,
    std::int32_t& y_fixed,
    const VelocityAdjustContext& context) noexcept;

[[nodiscard]] MovementResult try_move(
    PlayerState& player,
    const core::ControllerState& controller,
    const world::ByteGridView& terrain,
    const PlayerMovementConfig& config = {},
    const VelocityAdjustContext& velocity_context = {}) noexcept;

} // namespace oasis::game::player
