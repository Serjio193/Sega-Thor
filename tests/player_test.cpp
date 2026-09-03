#include "core/runtime.hpp"
#include "game/player/player.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using oasis::core::Button;
    using oasis::core::ControllerState;
    using oasis::game::player::Direction;
    using oasis::game::player::PlayerMovementConfig;
    using oasis::game::player::PlayerState;
    using oasis::game::player::direction_from_input;
    using oasis::game::player::movement_vector;
    using oasis::game::player::try_move;
    using oasis::game::player::update_movement_state;
    using oasis::game::world::ByteGridView;

    assert(direction_from_input(0x1) == Direction::up);
    assert(direction_from_input(0x2) == Direction::down);
    assert(direction_from_input(0x4) == Direction::left);
    assert(direction_from_input(0x8) == Direction::right);
    assert(direction_from_input(0x5) == Direction::up_left);
    assert(direction_from_input(0x6) == Direction::down_left);
    assert(direction_from_input(0x9) == Direction::up_right);
    assert(direction_from_input(0xA) == Direction::down_right);
    assert(direction_from_input(0x3) == Direction::none);

    const auto right = movement_vector(0x8);
    assert(right.x_fixed == 0x36000);
    assert(right.y_fixed == 0);
    const auto diagonal = movement_vector(0xA);
    assert(diagonal.x_fixed == 0x2A000);
    assert(diagonal.y_fixed == 0x25800);

    std::array<std::uint8_t, 64> grid{};
    grid.fill(0x02U); // terrain code 2 -> traversable state 2
    const ByteGridView terrain(grid, 8, 3);
    PlayerMovementConfig config;
    config.footprint_radius = 0;

    PlayerState player{16 * 0x10000, 16 * 0x10000, -1};
    ControllerState controller;
    controller.set(Button::Right);
    const auto before_x = player.x_fixed;
    const auto free_move = try_move(player, controller, terrain, config);
    assert(free_move.moved);
    assert(!free_move.blocked);
    assert(player.x_fixed == before_x + 0x36000);
    assert(player.terrain_state == 2);
    assert(player.movement_state == 2);
    assert(player.footprint_any_bits == 0x02);

    grid[2U * 8U + 3U] = 0x05U; // terrain code 5 is rejected by the table
    PlayerState blocked_player{3 * 8 * 0x10000, 2 * 8 * 0x10000, 2};
    const auto blocked_x = blocked_player.x_fixed;
    const auto blocked_move = try_move(blocked_player, controller, terrain, config);
    assert(!blocked_move.moved);
    assert(blocked_move.blocked);
    assert(blocked_player.x_fixed == blocked_x);
    assert(blocked_player.movement_state == 2);

    PlayerState stopped_player{};
    stopped_player.movement_state = 2;
    stopped_player.intent_x_fixed = 0x36000;
    stopped_player.accumulated_x_fixed = 0x36000;
    const auto stopped = update_movement_state(stopped_player, 0U, config);
    assert(stopped.direction == Direction::none);
    assert(stopped_player.movement_state == 0);
    assert(stopped_player.intent_x_fixed == 0);
    assert(stopped_player.accumulated_x_fixed == 0x36000);

    PlayerState turning_player{};
    turning_player.movement_state = 4;
    turning_player.direction_code = 0; // state-4 direction bit 0
    turning_player.orientation_flags = 0; // preserve the Y axis
    turning_player.intent_y_fixed = 0x30000;
    const auto turning = update_movement_state(turning_player, 0x9U, config);
    assert(turning.direction == Direction::up_right);
    assert(turning_player.accumulated_y_fixed == 0x30000);
    assert(turning_player.accumulated_x_fixed == 0x2A000);
    assert(turning_player.movement_state == 4);
    assert(turning_player.turn_timer == 1);

    PlayerState turning_move_player{16 * 0x10000, 16 * 0x10000, -1};
    turning_move_player.movement_state = 4;
    turning_move_player.direction_code = 0;
    turning_move_player.orientation_flags = 0;
    turning_move_player.intent_y_fixed = 0x30000;
    ControllerState turning_controller;
    turning_controller.set(Button::Up);
    turning_controller.set(Button::Right);
    const auto turning_move = try_move(turning_move_player, turning_controller,
                                       terrain, config);
    assert(turning_move.moved);
    assert(turning_move.vector.direction == Direction::up_right);
    assert(turning_move_player.x_fixed == 16 * 0x10000 + 0x2A000);
    assert(turning_move_player.y_fixed == 16 * 0x10000 + 0x30000);
    assert(turning_move_player.accumulated_x_fixed == 0);
    assert(turning_move_player.accumulated_y_fixed == 0);

    return 0;
}
