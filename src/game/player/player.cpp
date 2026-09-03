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
};

// This is the observable result of the 0x85FA dispatch table. Values 3, C and
// F (and the impossible opposite-direction combinations) intentionally stop.
constexpr DirectionEntry kDirectionTable[16]{
    {Direction::none, 0, 0, false},
    {Direction::up, 0, -1, false},
    {Direction::down, 0, 1, false},
    {Direction::none, 0, 0, false},
    {Direction::left, -1, 0, false},
    {Direction::up_left, -1, -1, true},
    {Direction::down_left, -1, 1, true},
    {Direction::left, -1, 0, false},
    {Direction::right, 1, 0, false},
    {Direction::up_right, 1, -1, true},
    {Direction::down_right, 1, 1, true},
    {Direction::right, 1, 0, false},
    {Direction::none, 0, 0, false},
    {Direction::up, 0, -1, false},
    {Direction::down, 0, 1, false},
    {Direction::none, 0, 0, false},
};

std::int32_t world_coordinate(std::int32_t fixed) noexcept {
    return fixed / 0x10000;
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

MovementResult try_move(PlayerState& player,
                        const core::ControllerState& controller,
                        const world::ByteGridView& terrain,
                        const PlayerMovementConfig& config) noexcept {
    const auto input = static_cast<std::uint8_t>(controller.buttons & 0x0FU);
    const auto vector = movement_vector(input, config);
    MovementResult result{vector, false, false};
    if (vector.direction == Direction::none) {
        return result;
    }

    const auto candidate_x = player.x_fixed + vector.x_fixed;
    const auto candidate_y = player.y_fixed + vector.y_fixed;
    const auto aggregate = terrain.aggregate_world_square(
        world_coordinate(candidate_x), world_coordinate(candidate_y),
        config.footprint_radius);
    if (!aggregate) {
        result.blocked = true;
        return result;
    }

    const auto prospective_state = world::terrain_state_from_code(
        aggregate->common_terrain_code());
    const auto gate = world::evaluate_terrain_gate(
        world::TerrainGateInput{config.entity_flags, player.terrain_state,
                                prospective_state, aggregate->any_bits});
    if (gate == world::TerrainGateResult::blocked) {
        result.blocked = true;
        return result;
    }

    player.x_fixed = candidate_x;
    player.y_fixed = candidate_y;
    player.terrain_state = prospective_state;
    result.moved = true;
    return result;
}

} // namespace oasis::game::player
