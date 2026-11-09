#pragma once

#include "genesis/vdp_state.hpp"

#include <algorithm>
#include <cstdint>

namespace oasis::game::camera {

// Camera configuration parameters matching authentic Beyond Oasis ROM routine 0x00CEEA.
struct CameraConfig {
    int deadzone_left{120};          // 0x78: left threshold
    int deadzone_right{200};         // 0xC8: right threshold
    int deadzone_top{104};           // 0x68: top threshold
    int deadzone_bottom{128};        // 0x80: bottom threshold
    int min_camera_x{0};             // Viewport min X clamp
    int max_camera_x{192};           // 0x00FF177A: beach scene max camera X (0xC0)
    int min_camera_y{0};             // Viewport min Y clamp
    int max_camera_y{288};           // 0x00FF177C: beach scene max camera Y (0x120)
    int max_step_per_frame{3};       // 0x00CEEA standard tracking step clamp
    bool smooth_tracking{true};      // Enable authentic smoothing clamp
    int initial_camera_x{96};        // 0x00FF1716: default beach viewport origin X
    int initial_camera_y{3};         // 0x00FF171A: default beach viewport origin Y
    int initial_hscroll_a{0};        // Default Plane A horizontal scroll
    int initial_hscroll_b{0};        // Default Plane B horizontal scroll
    int initial_vscroll_a{139};      // 0x00FF1726: default Plane A vertical scroll
    int initial_vscroll_b{0};        // Default Plane B vertical scroll
};

// Current camera and VDP plane scroll state.
struct CameraState {
    int camera_world_x{96};          // Camera world origin X
    int camera_world_y{3};           // Camera world origin Y
    int prev_camera_world_x{96};     // Previous frame camera X
    int prev_camera_world_y{3};      // Previous frame camera Y
    int prev2_camera_world_x{96};    // 2-frame latch for VDP VRAM double-buffering
    int prev2_camera_world_y{3};     // 2-frame latch for VDP VSRAM double-buffering

    int plane_a_hscroll{0};          // VRAM HScroll table Plane A (signed 16-bit)
    int plane_b_hscroll{0};          // VRAM HScroll table Plane B (signed 16-bit)
    int plane_a_vscroll{139};        // VSRAM Plane A (signed 10-bit)
    int plane_b_vscroll{0};          // VSRAM Plane B (signed 10-bit)

    bool tracking_enabled{true};     // Whether camera tracks player
    bool moved_this_frame{false};    // Whether camera position shifted this frame
};

// Native Beyond Oasis Camera Controller.
// Operates on player world coordinates, enforces authentic 0x00CEEA deadzone rules,
// updates camera world origin and VDP plane scroll registers (HScroll table at 0xDC00 and VSRAM).
class CameraController {
public:
    explicit CameraController(CameraConfig config = {}) noexcept;

    // Resets camera to default configuration or custom starting coordinates.
    void reset(int initial_x = 96, int initial_y = 3) noexcept;

    // Updates camera position based on player world coordinates (and altitude).
    // Follows authentic deadzone bounding and smoothing from ROM routine 0x00CEEA.
    void update(int player_world_x, int player_world_y, int player_world_z = 0, bool active_cadence = true) noexcept;

    // Directly set camera world position (e.g. for scripted panning or fixed camera).
    void set_camera_position(int world_x, int world_y) noexcept;

    // Transform world coordinates to viewport screen space.
    [[nodiscard]] int world_to_screen_x(int world_x) const noexcept {
        return world_x - state_.camera_world_x;
    }
    [[nodiscard]] int world_to_screen_y(int world_y, int world_z = 0) const noexcept {
        return world_y - world_z - state_.camera_world_y;
    }

    // Transform viewport screen space to world coordinates.
    [[nodiscard]] int screen_to_world_x(int screen_x) const noexcept {
        return screen_x + state_.camera_world_x;
    }
    [[nodiscard]] int screen_to_world_y(int screen_y) const noexcept {
        return screen_y + state_.camera_world_y;
    }

    // Commits calculated horizontal and vertical scroll values directly to VDP state:
    // - VRAM HScroll table at base address determined by Register 13 (0xDC00)
    // - VSRAM at offset 0 and 2 for Plane A and Plane B
    void apply_to_vdp(genesis::VdpState& vdp) const noexcept;

    // State and config accessors
    [[nodiscard]] const CameraState& state() const noexcept { return state_; }
    [[nodiscard]] const CameraConfig& config() const noexcept { return config_; }
    [[nodiscard]] int camera_x() const noexcept { return state_.camera_world_x; }
    [[nodiscard]] int camera_y() const noexcept { return state_.camera_world_y; }
    [[nodiscard]] int display_camera_x() const noexcept { return state_.prev2_camera_world_x; }
    [[nodiscard]] int display_camera_y() const noexcept { return state_.prev2_camera_world_y; }
    [[nodiscard]] int world_to_display_x(int world_x) const noexcept {
        return world_x - state_.prev2_camera_world_x;
    }
    [[nodiscard]] int world_to_display_y(int world_y, int world_z = 0) const noexcept {
        return world_y - world_z - state_.prev2_camera_world_y;
    }
    [[nodiscard]] bool tracking_enabled() const noexcept { return state_.tracking_enabled; }
    void set_tracking_enabled(bool enabled) noexcept { state_.tracking_enabled = enabled; }

private:
    CameraConfig config_{};
    CameraState state_{};
};

} // namespace oasis::game::camera
