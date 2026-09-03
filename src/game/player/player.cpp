#include "game/player/player.hpp"

#include "game/world/terrain_collision.hpp"

#include <cstdint>

namespace oasis::game::player {
namespace {

struct DirectionEntry {
    Direction direction;
    std::int8_t x_sign;
    std::int8_t y_sign;
    bool diagonal;
    std::uint16_t rom_code;
};

// This is the observable result of the 0x85FA dispatch table. Values 3, C and
// F (and the impossible opposite-direction combinations) intentionally stop.
constexpr DirectionEntry kDirectionTable[16]{
    {Direction::none, 0, 0, false, 0},
    {Direction::up, 0, -1, false, 0},
    {Direction::down, 0, 1, false, 2},
    {Direction::none, 0, 0, false, 0},
    {Direction::left, -1, 0, false, 3},
    {Direction::up_left, -1, -1, true, 0},
    {Direction::down_left, -1, 1, true, 2},
    {Direction::left, -1, 0, false, 3},
    {Direction::right, 1, 0, false, 1},
    {Direction::up_right, 1, -1, true, 0},
    {Direction::down_right, 1, 1, true, 2},
    {Direction::right, 1, 0, false, 1},
    {Direction::none, 0, 0, false, 0},
    {Direction::up, 0, -1, false, 0},
    {Direction::down, 0, 1, false, 2},
    {Direction::none, 0, 0, false, 0},
};

std::int32_t world_coordinate(std::int32_t fixed) noexcept {
    return fixed / 0x10000;
}

void stop_movement(PlayerState& player) noexcept {
    player.intent_x_fixed = 0;
    player.intent_y_fixed = 0;
    player.movement_state = 0;
}

std::uint16_t direction_bit(std::uint16_t direction_code) noexcept {
    constexpr std::uint16_t bits[4]{0, 3, 1, 2};
    return bits[direction_code & 3U];
}

std::int32_t approach_zero(std::int32_t value) noexcept {
    constexpr std::int32_t step = 0xACCC;
    if (value > 0) return value > step ? value - step : 0;
    if (value < 0) return value < -step ? value + step : 0;
    return 0;
}

} // namespace

Direction direction_from_input(std::uint8_t input_nibble) noexcept {
    return kDirectionTable[input_nibble & 0x0FU].direction;
}

MovementVector movement_vector(std::uint8_t input_nibble,
                               const PlayerMovementConfig& config) noexcept {
    const auto entry = kDirectionTable[input_nibble & 0x0FU];
    if (entry.direction == Direction::none) {
        return {};
    }

    const auto x_speed = entry.diagonal ? config.diagonal_x_speed
                                        : config.cardinal_x_speed;
    const auto y_speed = entry.diagonal ? config.diagonal_y_speed
                                        : config.cardinal_y_speed;
    return MovementVector{entry.direction,
                          static_cast<std::int32_t>(entry.x_sign * x_speed),
                          static_cast<std::int32_t>(entry.y_sign * y_speed)};
}

MovementVector update_movement_state(PlayerState& player,
                                     std::uint8_t input_nibble,
                                     const PlayerMovementConfig& config) noexcept {
    const auto input = static_cast<std::uint8_t>(input_nibble & 0x0FU);
    if (player.movement_state == 4U) {
        const auto& entry = kDirectionTable[input];
        const auto selected_bit = direction_bit(player.direction_code);
        if ((input & static_cast<std::uint8_t>(1U << selected_bit)) == 0U) {
            if (player.turn_timer < 6U) {
                stop_movement(player);
                return {};
            }
            player.movement_state = 0x0CU;
            player.intent_x_fixed = approach_zero(player.intent_x_fixed);
            player.intent_y_fixed = approach_zero(player.intent_y_fixed);
            player.accumulated_x_fixed += player.intent_x_fixed;
            player.accumulated_y_fixed += player.intent_y_fixed;
            return {};
        }

        const auto diagonal_x = config.diagonal_x_speed;
        const auto diagonal_y = config.diagonal_y_speed;
        MovementVector result{entry.direction, 0, 0};
        if ((player.orientation_flags & 1U) == 0U) {
            player.accumulated_y_fixed += player.intent_y_fixed;
            if ((input & (1U << 2U)) != 0U) {
                player.accumulated_x_fixed -= diagonal_x;
                result.x_fixed -= diagonal_x;
            }
            if ((input & (1U << 3U)) != 0U) {
                player.accumulated_x_fixed += diagonal_x;
                result.x_fixed += diagonal_x;
            }
            result.y_fixed = player.intent_y_fixed;
        } else {
            player.accumulated_x_fixed += player.intent_x_fixed;
            if ((input & (1U << 0U)) != 0U) {
                player.accumulated_y_fixed -= diagonal_y;
                result.y_fixed -= diagonal_y;
            }
            if ((input & (1U << 1U)) != 0U) {
                player.accumulated_y_fixed += diagonal_y;
                result.y_fixed += diagonal_y;
            }
            result.x_fixed = player.intent_x_fixed;
        }
        ++player.turn_timer;
        return result;
    }

    const auto vector = movement_vector(input, config);
    if (vector.direction == Direction::none) {
        stop_movement(player);
        return {};
    }

    player.direction_code = kDirectionTable[input].rom_code;
    player.intent_x_fixed = vector.x_fixed;
    player.intent_y_fixed = vector.y_fixed;
    player.accumulated_x_fixed += vector.x_fixed;
    player.accumulated_y_fixed += vector.y_fixed;
    player.movement_state = 2;
    return vector;
}

MovementResult try_move(PlayerState& player,
                        const core::ControllerState& controller,
                        const world::ByteGridView& terrain,
                        const PlayerMovementConfig& config) noexcept {
    const auto input = static_cast<std::uint8_t>(controller.buttons & 0x0FU);
    const auto vector = update_movement_state(player, input, config);
    MovementResult result{vector, false, false};
    if (vector.direction == Direction::none && player.movement_state == 0U) {
        return result;
    }

    const auto candidate_x = player.x_fixed + player.accumulated_x_fixed;
    const auto candidate_y = player.y_fixed + player.accumulated_y_fixed;
    const auto aggregate = terrain.aggregate_world_square(
        world_coordinate(candidate_x), world_coordinate(candidate_y),
        config.footprint_radius);
    if (!aggregate) {
        player.accumulated_x_fixed = 0;
        player.accumulated_y_fixed = 0;
        result.blocked = true;
        return result;
    }
    player.footprint_any_bits = aggregate->any_bits;

    const auto prospective_state = world::terrain_state_from_code(
        aggregate->common_terrain_code());
    const auto gate = world::evaluate_terrain_gate(
        world::TerrainGateInput{config.entity_flags, player.terrain_state,
                                prospective_state, aggregate->any_bits});
    if (gate == world::TerrainGateResult::blocked) {
        player.accumulated_x_fixed = 0;
        player.accumulated_y_fixed = 0;
        result.blocked = true;
        return result;
    }

    player.x_fixed = candidate_x;
    player.y_fixed = candidate_y;
    player.terrain_state = prospective_state;
    player.accumulated_x_fixed = 0;
    player.accumulated_y_fixed = 0;
    result.moved = true;
    return result;
}

} // namespace oasis::game::player
