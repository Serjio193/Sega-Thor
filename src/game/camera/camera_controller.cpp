#include "game/camera/camera_controller.hpp"

namespace oasis::game::camera {

CameraController::CameraController(CameraConfig config) noexcept
    : config_(config) {
    reset(config_.initial_camera_x, config_.initial_camera_y);
}

void CameraController::reset(int initial_x, int initial_y) noexcept {
    state_.camera_world_x = initial_x;
    state_.camera_world_y = initial_y;
    state_.prev_camera_world_x = initial_x;
    state_.prev_camera_world_y = initial_y;
    state_.prev2_camera_world_x = initial_x;
    state_.prev2_camera_world_y = initial_y;

    state_.plane_a_hscroll = config_.initial_hscroll_a;
    state_.plane_b_hscroll = config_.initial_hscroll_b;
    state_.plane_a_vscroll = config_.initial_vscroll_a;
    state_.plane_b_vscroll = config_.initial_vscroll_b;

    state_.tracking_enabled = true;
    state_.moved_this_frame = false;
}

void CameraController::set_camera_position(int world_x, int world_y) noexcept {
    state_.prev2_camera_world_x = state_.prev_camera_world_x;
    state_.prev2_camera_world_y = state_.prev_camera_world_y;
    state_.prev_camera_world_x = state_.camera_world_x;
    state_.prev_camera_world_y = state_.camera_world_y;

    state_.camera_world_x = std::clamp(world_x, config_.min_camera_x, config_.max_camera_x);
    state_.camera_world_y = std::clamp(world_y, config_.min_camera_y, config_.max_camera_y);

    const int total_dx = state_.camera_world_x - config_.initial_camera_x;
    const int total_dy = state_.camera_world_y - config_.initial_camera_y;

    state_.plane_a_hscroll = static_cast<int>((config_.initial_hscroll_a - total_dx) & 0x01FF);
    state_.plane_b_hscroll = static_cast<int>((config_.initial_hscroll_b - (total_dx / 2)) & 0x01FF);
    state_.plane_a_vscroll = static_cast<int>((config_.initial_vscroll_a + total_dy) & 0x03FF);
    state_.plane_b_vscroll = static_cast<int>((config_.initial_vscroll_b + (total_dy / 2)) & 0x03FF);

    state_.moved_this_frame = (state_.camera_world_x != state_.prev_camera_world_x) ||
                              (state_.camera_world_y != state_.prev_camera_world_y);
}

void CameraController::update(int player_world_x, int player_world_y, int player_world_z, bool active_cadence) noexcept {
    state_.prev2_camera_world_x = state_.prev_camera_world_x;
    state_.prev2_camera_world_y = state_.prev_camera_world_y;
    state_.prev_camera_world_x = state_.camera_world_x;
    state_.prev_camera_world_y = state_.camera_world_y;
    state_.moved_this_frame = false;

    if (state_.tracking_enabled && active_cadence) {
        const int screen_x = player_world_x - state_.camera_world_x;
        const int screen_y = (player_world_y - player_world_z) - state_.camera_world_y;

        // Authentic ROM routine 0x00CEEA: Horizontal Tracking
        int delta_x = 0;
        if (screen_x < config_.deadzone_left) {
            const int d0 = screen_x - config_.deadzone_left;
            if (!config_.smooth_tracking) {
                delta_x = d0;
            } else if (d0 >= -3) {
                delta_x = d0;
            } else if (d0 >= -32) {
                delta_x = -3;
            } else {
                delta_x = d0 + 30; // Accelerated catch-up
            }
        } else if (screen_x > config_.deadzone_right) {
            const int d0 = screen_x - config_.deadzone_right;
            if (!config_.smooth_tracking) {
                delta_x = d0;
            } else if (d0 <= 3) {
                delta_x = d0;
            } else if (d0 <= 32) {
                delta_x = 3;
            } else {
                delta_x = d0 - 30; // Accelerated catch-up
            }
        }

        // Authentic ROM routine 0x00CEEA: Vertical Tracking
        int delta_y = 0;
        if (screen_y < config_.deadzone_top) {
            const int d0 = screen_y - config_.deadzone_top;
            if (!config_.smooth_tracking) {
                delta_y = d0;
            } else if (d0 >= -3) {
                delta_y = d0;
            } else if (d0 >= -32) {
                delta_y = -3;
            } else {
                delta_y = std::max(-6, d0 + 30);
            }
        } else if (screen_y > config_.deadzone_bottom) {
            const int d0 = screen_y - config_.deadzone_bottom;
            if (!config_.smooth_tracking) {
                delta_y = d0;
            } else if (d0 <= 3) {
                delta_y = d0;
            } else if (d0 <= 32) {
                delta_y = 3;
            } else {
                delta_y = d0 - 30;
            }
        }

        const int target_x = state_.camera_world_x + delta_x;
        const int target_y = state_.camera_world_y + delta_y;

        state_.camera_world_x = std::clamp(target_x, config_.min_camera_x, config_.max_camera_x);
        state_.camera_world_y = std::clamp(target_y, config_.min_camera_y, config_.max_camera_y);
    }

    // 2-frame latch for VDP VRAM and VSRAM registers (double-buffered display pipeline)
    const int total_dx = state_.prev2_camera_world_x - config_.initial_camera_x;
    const int total_dy = state_.prev2_camera_world_y - config_.initial_camera_y;

    state_.plane_a_hscroll = static_cast<int>((config_.initial_hscroll_a - total_dx) & 0x01FF);
    state_.plane_b_hscroll = static_cast<int>((config_.initial_hscroll_b - (total_dx / 2)) & 0x01FF);
    state_.plane_a_vscroll = static_cast<int>((config_.initial_vscroll_a + total_dy) & 0x03FF);
    state_.plane_b_vscroll = static_cast<int>((config_.initial_vscroll_b + (total_dy / 2)) & 0x03FF);

    state_.moved_this_frame = (state_.camera_world_x != state_.prev_camera_world_x) ||
                              (state_.camera_world_y != state_.prev_camera_world_y);
}

void CameraController::apply_to_vdp(genesis::VdpState& vdp) const noexcept {
    // Write Plane A and Plane B horizontal scroll to VRAM HScroll Table (Register 13: 0xDC00)
    const std::size_t hscroll_base = vdp.hscroll_base();
    if (hscroll_base + 4U <= vdp.vram.size()) {
        vdp.vram[hscroll_base + 0U] = static_cast<std::uint8_t>((state_.plane_a_hscroll >> 8U) & 0xFFU);
        vdp.vram[hscroll_base + 1U] = static_cast<std::uint8_t>(state_.plane_a_hscroll & 0xFFU);
        vdp.vram[hscroll_base + 2U] = static_cast<std::uint8_t>((state_.plane_b_hscroll >> 8U) & 0xFFU);
        vdp.vram[hscroll_base + 3U] = static_cast<std::uint8_t>(state_.plane_b_hscroll & 0xFFU);
    }

    // Write Plane A and Plane B vertical scroll to VSRAM (offset 0: Plane A, offset 2: Plane B)
    if (vdp.vsram.size() >= 4U) {
        vdp.vsram[0U] = static_cast<std::uint8_t>((state_.plane_a_vscroll >> 8U) & 0xFFU);
        vdp.vsram[1U] = static_cast<std::uint8_t>(state_.plane_a_vscroll & 0xFFU);
        vdp.vsram[2U] = static_cast<std::uint8_t>((state_.plane_b_vscroll >> 8U) & 0xFFU);
        vdp.vsram[3U] = static_cast<std::uint8_t>(state_.plane_b_vscroll & 0xFFU);
    }
}

} // namespace oasis::game::camera
