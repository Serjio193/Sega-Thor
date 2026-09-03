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

    grid[2U * 8U + 3U] = 0x05U; // terrain code 5 is rejected by the table
    PlayerState blocked_player{3 * 8 * 0x10000, 2 * 8 * 0x10000, 2};
    const auto blocked_x = blocked_player.x_fixed;
    const auto blocked_move = try_move(blocked_player, controller, terrain, config);
    assert(!blocked_move.moved);
    assert(blocked_move.blocked);
    assert(blocked_player.x_fixed == blocked_x);
    assert(blocked_player.movement_state == 2);

    return 0;
}
