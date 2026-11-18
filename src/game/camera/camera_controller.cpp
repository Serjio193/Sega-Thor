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
    state_.plane_b_hscroll = config_.plane_b_parallax
                                 ? static_cast<int>((config_.initial_hscroll_b - (total_dx / 2)) & 0x01FF)
                                 : config_.initial_hscroll_b;
    const int vscroll_mask = config_.vscroll_wrap_8bit ? 0x00FF : 0x03FF;
    state_.plane_a_vscroll = static_cast<int>((config_.initial_vscroll_a + total_dy) & vscroll_mask);
    state_.plane_b_vscroll = config_.plane_b_parallax
                                 ? static_cast<int>((config_.initial_vscroll_b + (total_dy / 2)) & vscroll_mask)
                                 : config_.initial_vscroll_b;

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
    state_.plane_b_hscroll = config_.plane_b_parallax
                                 ? static_cast<int>((config_.initial_hscroll_b - (total_dx / 2)) & 0x01FF)
                                 : config_.initial_hscroll_b;
    const int vscroll_mask = config_.vscroll_wrap_8bit ? 0x00FF : 0x03FF;
    state_.plane_a_vscroll = static_cast<int>((config_.initial_vscroll_a + total_dy) & vscroll_mask);
    state_.plane_b_vscroll = config_.plane_b_parallax
                                 ? static_cast<int>((config_.initial_vscroll_b + (total_dy / 2)) & vscroll_mask)
                                 : config_.initial_vscroll_b;

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

namespace thor::camera {

namespace {

[[nodiscard]] inline uint16_t read_be16(std::span<const uint8_t> data, size_t offset) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1]);
}

inline void write_be16(std::span<uint8_t> data, size_t offset, uint16_t value) noexcept {
    data[offset] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

} // namespace

void update_camera_deadzone(
    CameraState& cam,
    int16_t player_x,
    int16_t player_y,
    int16_t player_elev_z,
    bool warp_active,
    bool instant_snap,
    bool direct_follow_mode) noexcept
{
    int16_t effective_y = player_y;
    if (!warp_active) {
        effective_y = static_cast<int16_t>(effective_y - player_elev_z);
    }

    if (direct_follow_mode) {
        // 0x00D0C4: camera_fixed_axis_direct_follow
        cam.x = static_cast<int16_t>(player_x + cam.offset_x - 160);
        cam.y = static_cast<int16_t>(effective_y + cam.offset_y - 120);

        if (cam.shake_timer > 0) {
            cam.shake_timer -= 1;
            int16_t offset = static_cast<int16_t>(cam.shake_timer);
            if ((offset & 1) == 0) {
                offset = -offset;
            }
            cam.y = static_cast<int16_t>(cam.y + offset);
            if (cam.y < 0) {
                cam.y = 0;
            } else if (cam.y > cam.max_y) {
                cam.y = cam.max_y;
            }
        }
        return;
    }

    const int16_t rel_x = static_cast<int16_t>(player_x - cam.x);
    const int16_t rel_y = static_cast<int16_t>(effective_y - cam.y);

    // 0x00CF28: scroll bias direction flag
    if (cam.scroll_bias_dir == 0) {
        if (rel_x <= 80) {
            cam.scroll_bias_dir = 0xFF;
            if (instant_snap) {
                cam.scroll_bias_target = 0x0138;
            }
        }
    } else {
        if (rel_x >= 240) {
            cam.scroll_bias_dir = 0x00;
            if (instant_snap) {
                cam.scroll_bias_target = 0x0020;
            }
        }
    }

    const int16_t orig_cam_x = cam.x;

    // 0x00CF6E: Left edge panning
    if (rel_x < 120) {
        int16_t d0 = static_cast<int16_t>(rel_x - 120);
        if (!instant_snap) {
            if (d0 >= -3) {
                // keep d0
            } else if (d0 >= -32) {
                d0 = -3;
            } else {
                d0 = static_cast<int16_t>(d0 + 30);
            }
        }
        cam.x = static_cast<int16_t>(cam.x + d0);
    }
    if (cam.x < 0) {
        cam.x = 0;
    }

    // 0x00CFA8: Right edge panning
    if (rel_x > 200) {
        int16_t d0 = static_cast<int16_t>(rel_x - 200);
        if (!instant_snap) {
            if (d0 <= 3) {
                // keep d0
            } else if (d0 <= 32) {
                d0 = 3;
            } else {
                d0 = static_cast<int16_t>(d0 - 30);
            }
        }
        cam.x = static_cast<int16_t>(cam.x + d0);
    }
    if (cam.x > cam.max_x) {
        cam.x = cam.max_x;
    }

    // 0x00CFE8: Top edge panning
    if (rel_y < 104) {
        int16_t d0 = static_cast<int16_t>(rel_y - 104);
        if (!instant_snap) {
            if (d0 >= -3) {
                // keep d0
            } else if (d0 >= -32) {
                d0 = -3;
            } else {
                d0 = static_cast<int16_t>(d0 + 30);
                if (d0 < -6) {
                    d0 = -6;
                }
            }
        }
        cam.y = static_cast<int16_t>(cam.y + d0);
    }
    if (cam.y < 0) {
        cam.y = 0;
    }

    // 0x00D02C: Bottom edge panning
    if (rel_y > 128) {
        int16_t d0 = static_cast<int16_t>(rel_y - 128);
        if (!instant_snap) {
            if (d0 <= 3) {
                // keep d0
            } else if (d0 <= 32) {
                d0 = 3;
            } else {
                d0 = static_cast<int16_t>(d0 - 30);
            }
        }
        cam.y = static_cast<int16_t>(cam.y + d0);
    }
    if (cam.y > cam.max_y) {
        cam.y = cam.max_y;
    }

    // 0x00D068: Screen shake oscillation
    if (cam.shake_timer > 0) {
        cam.shake_timer -= 1;
        int16_t offset = static_cast<int16_t>(cam.shake_timer);
        if ((offset & 1) == 0) {
            offset = -offset;
        }
        cam.y = static_cast<int16_t>(cam.y + offset);
        if (cam.y < 0) {
            cam.y = 0;
        } else if (cam.y > cam.max_y) {
            cam.y = cam.max_y;
        }
    }

    // 0x00D098: Post-movement bias direction update
    if (!instant_snap) {
        const int16_t delta_x = static_cast<int16_t>(cam.x - orig_cam_x);
        if (delta_x < 0) {
            cam.scroll_bias_dir = 0xFF;
        } else if (delta_x > 0) {
            cam.scroll_bias_dir = 0x00;
        }
    }
}

void clamp_camera_viewport(
    CameraState& cam,
    uint16_t map_width_tiles,
    uint16_t map_height_tiles) noexcept
{
    if (cam.x < 0) {
        cam.x = 0;
    }
    const int16_t max_x = std::max<int16_t>(0, static_cast<int16_t>((static_cast<int16_t>(map_width_tiles) - 40) << 3));
    if (cam.x > max_x) {
        cam.x = max_x;
    }

    if (cam.y < 0) {
        cam.y = 0;
    }
    const int16_t max_y = std::max<int16_t>(0, static_cast<int16_t>((static_cast<int16_t>(map_height_tiles) - 28) << 3));
    if (cam.y > max_y) {
        cam.y = max_y;
    }
}

void update_camera_from_ram(
    CameraState& cam,
    std::span<uint8_t, 0x10000> ram) noexcept
{
    const int16_t player_x = static_cast<int16_t>(read_be16(ram, 0x19F0));
    const int16_t player_y = static_cast<int16_t>(read_be16(ram, 0x19F4));
    const int16_t player_elev_z = static_cast<int16_t>(read_be16(ram, 0x19F8));
    const bool warp_active = (ram[0x17BB] != 0);
    const bool instant_snap = (ram[0x17BA] != 0);
    const bool direct_follow = ((ram[0x16F1] & 0x08) != 0);

    cam.max_x = static_cast<int16_t>(read_be16(ram, 0x177A));
    cam.max_y = static_cast<int16_t>(read_be16(ram, 0x177C));
    cam.offset_x = static_cast<int16_t>(read_be16(ram, 0x177E));
    cam.offset_y = static_cast<int16_t>(read_be16(ram, 0x1780));
    cam.shake_timer = static_cast<uint8_t>(read_be16(ram, 0x17C4) & 0xFF);
    cam.scroll_bias_dir = ram[0x1859];
    cam.scroll_bias_target = read_be16(ram, 0x185A);

    // 0x00CF14: sf.b $ff17ba.l
    ram[0x17BA] = 0;

    update_camera_deadzone(
        cam,
        player_x,
        player_y,
        player_elev_z,
        warp_active,
        instant_snap,
        direct_follow);

    write_be16(ram, 0x17C4, static_cast<uint16_t>(cam.shake_timer));
    ram[0x1859] = cam.scroll_bias_dir;
    write_be16(ram, 0x185A, cam.scroll_bias_target);
}

} // namespace thor::camera
