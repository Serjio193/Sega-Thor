#include "game/camera/camera_follow.hpp"

#include <algorithm>

namespace oasis::game::camera {

namespace {

[[nodiscard]] std::int32_t smooth_axis_delta(
    std::int32_t delta,
    bool clamp_upward_y) noexcept {
    if (delta > 0) {
        if (delta <= kCatchupThresholdPx) {
            return std::min(delta, kSmoothStepPx);
        }
        return delta - kCatchupBiasPx;
    }
    if (delta < 0) {
        if (delta >= -kCatchupThresholdPx) {
            return std::max(delta, -kSmoothStepPx);
        }
        const std::int32_t ramped = delta + kCatchupBiasPx;
        return clamp_upward_y ? std::max(ramped, kCatchupUpMinDelta) : ramped;
    }
    return 0;
}

} // namespace

void step_camera_follow(CameraFollowState& state) noexcept {
    const std::int32_t target_x = state.player_x;
    const std::int32_t target_y =
        state.player_y - (state.ignore_z_ff17bb ? 0 : state.player_z);

    if (state.locked_center_mode) {
        state.cam_x = std::clamp(
            target_x + state.lock_offset_x - kLockedCenterX,
            0,
            state.cam_max_x);
        state.cam_y = std::clamp(
            target_y + state.lock_offset_y - kLockedCenterY,
            0,
            state.cam_max_y);
    } else {
        const std::int32_t rel_x = target_x - state.cam_x;
        const std::int32_t rel_y = target_y - state.cam_y;

        if (rel_x <= kHudSlideLeftThresh) {
            state.hud_slide_dir_ff1859 = 0xFFU;
            if (state.instant_snap_ff17ba) {
                state.hud_slide_offset_ff185a = kHudSlideRevSnap;
            }
        } else if (rel_x >= kHudSlideRightThresh) {
            state.hud_slide_dir_ff1859 = 0U;
            if (state.instant_snap_ff17ba) {
                state.hud_slide_offset_ff185a = kHudSlideFwdSnap;
            }
        }

        std::int32_t dx = 0;
        if (rel_x < kDeadzoneXMin) {
            dx = rel_x - kDeadzoneXMin;
        } else if (rel_x > kDeadzoneXMax) {
            dx = rel_x - kDeadzoneXMax;
        }

        std::int32_t dy = 0;
        if (rel_y < kDeadzoneYMin) {
            dy = rel_y - kDeadzoneYMin;
        } else if (rel_y > kDeadzoneYMax) {
            dy = rel_y - kDeadzoneYMax;
        }

        if (!state.instant_snap_ff17ba) {
            dx = smooth_axis_delta(dx, false);
            dy = smooth_axis_delta(dy, true);
        }

        const std::int32_t prev_cam_x = state.cam_x;
        state.cam_x = std::clamp(state.cam_x + dx, 0, state.cam_max_x);
        state.cam_y = std::clamp(state.cam_y + dy, 0, state.cam_max_y);

        if (state.cam_x < prev_cam_x) {
            state.hud_slide_dir_ff1859 = 0xFFU;
        } else if (state.cam_x > prev_cam_x) {
            state.hud_slide_dir_ff1859 = 0U;
        }
    }

    if (state.shake_timer_ff17c4 > 0U) {
        const std::int32_t t = static_cast<std::int32_t>(state.shake_timer_ff17c4);
        --state.shake_timer_ff17c4;
        const std::int32_t offset = ((t & 1) != 0) ? t : -t;
        state.cam_y = std::clamp(state.cam_y + offset, 0, state.cam_max_y);
    }
}

} // namespace oasis::game::camera
