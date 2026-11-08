#include "game/standalone_game_simulation.hpp"

#include <algorithm>

namespace oasis::game {
namespace {

void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t value) noexcept {
    if (offset + 1U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
    }
}

} // namespace

StandaloneGameSimulation::StandaloneGameSimulation() {
    terrain_buffer_.assign(128 * 128, 0x02U); // 0x02 = traversable ground
    terrain_ = world::ByteGridView(terrain_buffer_, 128, 128, 128, 7);
}

bool StandaloneGameSimulation::initialize(const genesis::VdpState& baseline_vdp,
                                          std::int32_t initial_x,
                                          std::int32_t initial_y) {
    baseline_vdp_ = baseline_vdp;
    initial_x_ = initial_x;
    initial_y_ = initial_y;
    camera_x_ = 125;
    camera_y_ = 46;
    camera_tracking_ = false;
    reset();
    initialized_ = true;
    return true;
}

void StandaloneGameSimulation::reset() {
    player_ = player::PlayerState{initial_x_ * 0x10000, initial_y_ * 0x10000, -1};
    player_.movement_state = 0;
    player_.direction_code = 1; // Facing right
    player_.orientation_flags = 1;
    player_.hp = 100;
    player_.max_hp = 100;

    movement_config_ = player::PlayerMovementConfig{};
    movement_config_.cardinal_x_speed = 0x36000;
    movement_config_.cardinal_y_speed = 0x30000;
    movement_config_.diagonal_x_speed = 0x2A000;
    movement_config_.diagonal_y_speed = 0x25800;
    movement_config_.footprint_radius = 0;

    velocity_context_ = player::VelocityAdjustContext{};
    walk_tick_ = 0;

    sim_state_.frame_counter = 0;
    sim_state_.player_x = static_cast<std::uint16_t>(initial_x_);
    sim_state_.player_y = static_cast<std::uint16_t>(initial_y_);
    sim_state_.paused = false;
}

void StandaloneGameSimulation::set_player_position(std::int32_t x, std::int32_t y) noexcept {
    player_.x_fixed = x * 0x10000;
    player_.y_fixed = y * 0x10000;
    sim_state_.player_x = static_cast<std::uint16_t>(x);
    sim_state_.player_y = static_cast<std::uint16_t>(y);
}

void StandaloneGameSimulation::step_frame(const FrameInput& input, genesis::VdpState& out_vdp) {
    ++sim_state_.frame_counter;

    // Advance player physics and movement via authentic reverse-engineered logic
    const auto move_result = player::try_move(player_, input.controller_1, terrain_,
                                              movement_config_, velocity_context_);

    if (move_result.moved || player_.movement_state != 0U) {
        walk_tick_ = (walk_tick_ + 1) % 16;
    } else {
        walk_tick_ = 0;
    }

    const auto cur_x = static_cast<std::int32_t>(player_.x_fixed / 0x10000);
    const auto cur_y = static_cast<std::int32_t>(player_.y_fixed / 0x10000);
    sim_state_.player_x = static_cast<std::uint16_t>(cur_x);
    sim_state_.player_y = static_cast<std::uint16_t>(cur_y);

    if (camera_tracking_) {
        camera_x_ = cur_x - 136;
        camera_y_ = cur_y - 61;
    }

    // Publish state to VDP
    out_vdp = baseline_vdp_;
    update_sat_sprites(out_vdp);
}

void StandaloneGameSimulation::update_sat_sprites(genesis::VdpState& vdp) const {
    const auto sat_base = vdp.sat_base();
    if (sat_base + (20U * 8U) > vdp.vram.size()) return;

    const int px = static_cast<int>(player_.x_fixed / 0x10000);
    const int py = static_cast<int>(player_.y_fixed / 0x10000);
    const int screen_x = px - camera_x_;
    const int screen_y = py - camera_y_;

    const bool hflip = (player_.direction_code == 3); // 3 = Left in ROM direction table
    const std::uint16_t flip_mask = hflip ? 0x0800U : 0x0000U;

    // Piece 13: Body / Legs (size 3x4 = 24x32 px)
    const std::uint32_t p13_off = sat_base + (13U * 8U);
    const auto p13_y = static_cast<std::uint16_t>((screen_y + 128) & 0x03FFU);
    const auto p13_x = static_cast<std::uint16_t>(((hflip ? (screen_x + 8) : screen_x) + 128) & 0x01FFU);
    write_be16(vdp.vram, p13_off + 0U, p13_y);
    write_be16(vdp.vram, p13_off + 4U, static_cast<std::uint16_t>(0x8479U | flip_mask));
    write_be16(vdp.vram, p13_off + 6U, p13_x);

    // Piece 14: Torso (size 3x2 = 24x16 px)
    const std::uint32_t p14_off = sat_base + (14U * 8U);
    const auto p14_y = static_cast<std::uint16_t>((screen_y - 16 + 128) & 0x03FFU);
    const auto p14_x = static_cast<std::uint16_t>(((hflip ? (screen_x + 8) : screen_x) + 128) & 0x01FFU);
    write_be16(vdp.vram, p14_off + 0U, p14_y);
    write_be16(vdp.vram, p14_off + 4U, static_cast<std::uint16_t>(0x8485U | flip_mask));
    write_be16(vdp.vram, p14_off + 6U, p14_x);

    // Piece 15: Head (size 2x2 = 16x16 px)
    const std::uint32_t p15_off = sat_base + (15U * 8U);
    const auto p15_y = static_cast<std::uint16_t>((screen_y - 32 + 128) & 0x03FFU);
    const auto p15_x = static_cast<std::uint16_t>(((hflip ? (screen_x + 16) : (screen_x + 8)) + 128) & 0x01FFU);
    write_be16(vdp.vram, p15_off + 0U, p15_y);
    write_be16(vdp.vram, p15_off + 4U, static_cast<std::uint16_t>(0x848BU | flip_mask));
    write_be16(vdp.vram, p15_off + 6U, p15_x);

    // Piece 16: Neck / Hair (size 1x1 = 8x8 px)
    const std::uint32_t p16_off = sat_base + (16U * 8U);
    const auto p16_y = static_cast<std::uint16_t>((screen_y - 24 + 128) & 0x03FFU);
    const auto p16_x = static_cast<std::uint16_t>(((hflip ? (screen_x + 8) : screen_x) + 128) & 0x01FFU);
    write_be16(vdp.vram, p16_off + 0U, p16_y);
    write_be16(vdp.vram, p16_off + 4U, static_cast<std::uint16_t>(0x848FU | flip_mask));
    write_be16(vdp.vram, p16_off + 6U, p16_x);
}

} // namespace oasis::game
