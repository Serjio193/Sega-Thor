#include "game/controlled_screen.hpp"

#include "game/world/terrain_collision.hpp"

#include <algorithm>

namespace oasis::game::screen {
namespace {

constexpr std::uint8_t kFree = 0x02U;
constexpr std::uint8_t kWall = 0x05U;
constexpr std::uint32_t kBackground = 0x00101828U;
constexpr std::uint32_t kFreeCell = 0x001A2A3AU;
constexpr std::uint32_t kWallCell = 0x006D3548U;
constexpr std::uint32_t kGridLine = 0x002A3A4AU;
constexpr std::uint32_t kPlayer = 0x00F2D15CU;

void fill_rect(std::span<std::uint32_t> pixels,
               std::uint32_t color,
               int x,
               int y,
               int width,
               int height) noexcept {
    const auto max_x = static_cast<int>(render::SoftwareFramebuffer::kWidth);
    const auto max_y = static_cast<int>(render::SoftwareFramebuffer::kHeight);
    const auto x0 = std::max(0, x);
    const auto y0 = std::max(0, y);
    const auto x1 = std::min(max_x, x + width);
    const auto y1 = std::min(max_y, y + height);
    for (auto row = y0; row < y1; ++row) {
        for (auto column = x0; column < x1; ++column) {
            pixels[static_cast<std::size_t>(row) * max_x + column] = color;
        }
    }
}

} // namespace

bool operator==(const ScreenState& left, const ScreenState& right) noexcept {
    const auto& a = left.player;
    const auto& b = right.player;
    return left.frame_index == right.frame_index &&
           a.x_fixed == b.x_fixed && a.y_fixed == b.y_fixed &&
           a.terrain_state == b.terrain_state &&
           a.movement_state == b.movement_state &&
           a.direction_code == b.direction_code &&
           a.orientation_flags == b.orientation_flags &&
           a.intent_x_fixed == b.intent_x_fixed &&
           a.intent_y_fixed == b.intent_y_fixed &&
           a.accumulated_x_fixed == b.accumulated_x_fixed &&
           a.accumulated_y_fixed == b.accumulated_y_fixed &&
           a.footprint_any_bits == b.footprint_any_bits &&
           a.turn_timer == b.turn_timer &&
           left.movement.vector.direction == right.movement.vector.direction &&
           left.movement.vector.x_fixed == right.movement.vector.x_fixed &&
           left.movement.vector.y_fixed == right.movement.vector.y_fixed &&
           left.movement.moved == right.movement.moved &&
           left.movement.blocked == right.movement.blocked;
}

ControlledScreen::ControlledScreen()
    : terrain_(fixture_cells_, kGridWidth, 5) {
    movement_config_.footprint_radius = 6;
    state_.player.x_fixed = 48 * 0x10000;
    state_.player.y_fixed = 48 * 0x10000;
    state_.player.terrain_state = world::terrain_state_from_code(kFree);
    build_fixture();
}

void ControlledScreen::build_fixture() noexcept {
    fixture_cells_.fill(kFree);

    const auto set_cell = [this](std::uint16_t x, std::uint16_t y) {
        fixture_cells_[static_cast<std::size_t>(y) * kGridWidth + x] = kWall;
    };

    // Explicit test geometry: a vertical wall, a joined corner and an isolated block.
    for (std::uint16_t y = 5; y <= 18; ++y) {
        set_cell(20, y);
        set_cell(21, y);
    }
    for (std::uint16_t x = 8; x <= 21; ++x) {
        set_cell(x, 18);
        set_cell(x, 19);
    }
    for (std::uint16_t y = 8; y <= 10; ++y) {
        for (std::uint16_t x = 29; x <= 31; ++x) set_cell(x, y);
    }
}

void ControlledScreen::update(const core::FrameContext& frame) {
    state_.frame_index = frame.frame_index;
    state_.movement = player::try_move(state_.player, frame.input.port1,
                                       terrain_, movement_config_);
}

void ControlledScreen::render(std::span<std::uint32_t> destination) const noexcept {
    const auto expected = static_cast<std::size_t>(render::SoftwareFramebuffer::kWidth) *
                          render::SoftwareFramebuffer::kHeight;
    if (destination.size() < expected) return;
    destination = destination.first(expected);
    std::fill(destination.begin(), destination.end(), kBackground);

    for (std::uint16_t y = 0; y < kGridHeight; ++y) {
        for (std::uint16_t x = 0; x < kGridWidth; ++x) {
            const auto cell = fixture_cells_[static_cast<std::size_t>(y) * kGridWidth + x];
            fill_rect(destination, cell == kWall ? kWallCell : kFreeCell,
                      x * 8, y * 8, 8, 8);
        }
    }
    for (std::uint16_t y = 0; y <= kGridHeight; ++y) {
        fill_rect(destination, kGridLine, 0, y * 8, render::SoftwareFramebuffer::kWidth, 1);
    }
    for (std::uint16_t x = 0; x <= kGridWidth; ++x) {
        fill_rect(destination, kGridLine, x * 8, 0, 1, render::SoftwareFramebuffer::kHeight);
    }

    const auto x = state_.player.x_fixed / 0x10000;
    const auto y = state_.player.y_fixed / 0x10000;
    fill_rect(destination, kPlayer, x - 5, y - 5, 11, 11);
}

} // namespace oasis::game::screen
