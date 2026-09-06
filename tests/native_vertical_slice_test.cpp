#include "core/rom_identity.hpp"
#include "core/runtime.hpp"
#include "game/controlled_screen.hpp"
#include "game/render/framebuffer.hpp"
#include "game/world/byte_grid.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

using oasis::core::Button;
using oasis::core::ControllerState;
using oasis::core::InputSnapshot;
using oasis::game::player::Direction;
using oasis::game::player::PlayerMovementConfig;
using oasis::game::player::PlayerState;
using oasis::game::player::try_move;
using oasis::game::render::SoftwareFramebuffer;
using oasis::game::screen::ControlledScreen;
using oasis::game::screen::ScreenState;
using oasis::game::world::ByteGridView;

ControllerState controller_for(std::uint8_t nibble) {
    ControllerState result{};
    result.buttons = nibble & 0x0FU;
    return result;
}

std::uint8_t replay_input(std::size_t frame) {
    constexpr std::array<std::uint8_t, 16> inputs{
        0x0, 0x1, 0x2, 0x4, 0x8, 0x5, 0x6, 0x9,
        0xA, 0x3, 0x7, 0xB, 0xD, 0xE, 0xF, 0x0,
    };
    return inputs[frame % inputs.size()];
}

std::vector<ScreenState> run_replay(std::size_t render_interval,
                                    std::string* framebuffer_hash) {
    ControlledScreen screen;
    oasis::core::RuntimeLoop runtime(screen);
    SoftwareFramebuffer framebuffer;
    std::vector<ScreenState> states;
    states.reserve(600);
    for (std::size_t frame = 0; frame < 600; ++frame) {
        runtime.step(InputSnapshot{controller_for(replay_input(frame)), {}});
        states.push_back(screen.state());
        if (frame % render_interval == 0) screen.render(framebuffer.pixels());
    }
    screen.render(framebuffer.pixels());
    if (framebuffer_hash != nullptr) {
        std::vector<std::uint8_t> canonical_bytes;
        canonical_bytes.reserve(framebuffer.pixels().size() * 4U);
        for (const auto pixel : framebuffer.pixels()) {
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 24U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 16U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel >> 8U));
            canonical_bytes.push_back(static_cast<std::uint8_t>(pixel));
        }
        *framebuffer_hash = oasis::calculate_sha256(canonical_bytes);
    }
    assert(runtime.frame_index() == 600);
    return states;
}

} // namespace

int main() {
    ControlledScreen screen;
    const auto fixture = screen.fixture_cells();
    assert(fixture.size() == ControlledScreen::kGridWidth * ControlledScreen::kGridHeight);
    assert(fixture[6U * ControlledScreen::kGridWidth + 20U] == 0x05U);
    assert(fixture[18U * ControlledScreen::kGridWidth + 20U] == 0x05U);
    assert(fixture[10U * ControlledScreen::kGridWidth + 29U] == 0x05U);

    const auto config = screen.movement_config();
    assert(config.footprint_radius != 0);
    assert(config.cardinal_x_speed == 0x36000);
    assert(config.diagonal_x_speed == 0x2A000);
    assert(config.diagonal_y_speed == 0x25800);

    constexpr std::array<Direction, 16> directions{
        Direction::none, Direction::up, Direction::down, Direction::none,
        Direction::left, Direction::up_left, Direction::down_left, Direction::left,
        Direction::right, Direction::up_right, Direction::down_right, Direction::right,
        Direction::none, Direction::up, Direction::down, Direction::none,
    };
    for (std::uint8_t nibble = 0; nibble < directions.size(); ++nibble) {
        assert(oasis::game::player::direction_from_input(nibble) == directions[nibble]);
    }

    std::array<std::uint8_t, 32U * 28U> terrain_bytes{};
    terrain_bytes.fill(0x02U);
    terrain_bytes[18U * 32U + 20U] = 0x05U;
    const ByteGridView terrain(terrain_bytes, 32, 5);
    PlayerMovementConfig collision_config = config;
    collision_config.footprint_radius = 0;

    PlayerState free_player{48 * 0x10000, 48 * 0x10000, 2};
    auto free_result = try_move(free_player, controller_for(0x8), terrain,
                                collision_config);
    assert(free_result.moved && !free_result.blocked);
    assert(free_player.x_fixed == 48 * 0x10000 + 0x36000);

    PlayerState wall_player{19 * 8 * 0x10000 + 7 * 0x10000,
                            17 * 8 * 0x10000 + 7 * 0x10000, 2};
    auto corner_result = try_move(wall_player, controller_for(0xA), terrain,
                                  collision_config);
    assert(!corner_result.moved && corner_result.blocked);

    PlayerState boundary_player{0, 0, 2};
    auto boundary_result = try_move(boundary_player, controller_for(0x5), terrain,
                                    collision_config);
    assert(!boundary_result.moved && boundary_result.blocked);
    assert(boundary_player.x_fixed == 0 && boundary_player.y_fixed == 0);

    PlayerState footprint_player{19 * 8 * 0x10000, 18 * 8 * 0x10000, 2};
    collision_config.footprint_radius = 6;
    auto footprint_result = try_move(footprint_player, controller_for(0x8), terrain,
                                     collision_config);
    assert(!footprint_result.moved && footprint_result.blocked);

    std::string first_hash;
    const auto first = run_replay(1, &first_hash);
    std::string second_hash;
    const auto second = run_replay(7, &second_hash);
    std::string third_hash;
    const auto third = run_replay(31, &third_hash);
    assert(first == second && second == third);
    assert(first_hash == second_hash && second_hash == third_hash);
    assert(first_hash == "3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da");
    assert(first.back().frame_index == 599);
    assert(first.back().player.movement_state == 0 ||
           first.back().player.movement_state == 2);
    return 0;
}
