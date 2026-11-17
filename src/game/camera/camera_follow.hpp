#pragma once

#include <cstdint>

namespace oasis::game::camera {

inline constexpr std::uint32_t kCameraFollowAddr = 0x00CEEAU;
inline constexpr std::int32_t kDeadzoneXMin = 0x78; // 120
inline constexpr std::int32_t kDeadzoneXMax = 0xC8; // 200
inline constexpr std::int32_t kDeadzoneYMin = 0x68; // 104
inline constexpr std::int32_t kDeadzoneYMax = 0x80; // 128
inline constexpr std::int32_t kHudSlideLeftThresh = 0x50;  // 80
inline constexpr std::int32_t kHudSlideRightThresh = 0xF0; // 240
inline constexpr std::uint16_t kHudSlideRevSnap = 0x0138U;
inline constexpr std::uint16_t kHudSlideFwdSnap = 0x0020U;
inline constexpr std::int32_t kSmoothStepPx = 3;
inline constexpr std::int32_t kCatchupThresholdPx = 0x20; // 32
inline constexpr std::int32_t kCatchupBiasPx = 0x1E;      // 30
inline constexpr std::int32_t kCatchupUpMinDelta = -6;
inline constexpr std::int32_t kLockedCenterX = 0xA0; // 160
inline constexpr std::int32_t kLockedCenterY = 0x78; // 120

struct CameraFollowState {
    std::int32_t cam_x{0};
    std::int32_t cam_y{0};
    std::int32_t cam_max_x{0}; // $FF177A
    std::int32_t cam_max_y{0}; // $FF177C

    std::int32_t player_x{0};  // $FF19F0
    std::int32_t player_y{0};  // $FF19F4
    std::int32_t player_z{0};  // $FF19F8
    bool ignore_z_ff17bb{false};
    bool instant_snap_ff17ba{false};

    bool locked_center_mode{false}; // $FF16F1 & 8 != 0
    std::int32_t lock_offset_x{0};
    std::int32_t lock_offset_y{0};

    std::uint8_t shake_timer_ff17c4{0};     // $FF17C4
    std::uint8_t hud_slide_dir_ff1859{0};   // $FF1859
    std::uint16_t hud_slide_offset_ff185a{kHudSlideFwdSnap}; // $FF185A
};

/// Models 0x00CEEA..0x00D114: deadzone follow, smooth +/-3 or +/-0x1E catch-up
/// ramp, HUD slide threshold/motion triggers, locked-center mode, and screen shake.
void step_camera_follow(CameraFollowState& state) noexcept;

} // namespace oasis::game::camera
