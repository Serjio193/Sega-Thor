#include "game/entities/solid_entity_sweep.hpp"

#include <algorithm>

namespace oasis::game::entities {

namespace {

[[nodiscard]] constexpr bool intervals_overlap(
    std::int32_t a_min,
    std::int32_t a_max,
    std::int32_t b_min,
    std::int32_t b_max) noexcept {
    return a_max > b_min && a_min < b_max;
}

} // namespace

SolidSweepResult sweep_solid_x_pos(
    const SolidSweepBlocker& mover,
    std::size_t mover_index,
    std::int32_t dx,
    std::span<const SolidSweepBlocker> slots) noexcept {
    SolidSweepResult out{};
    out.clamped_dx = dx;
    if (dx <= 0) {
        return out;
    }

    const std::int32_t mover_right = mover.x + mover.half_w;
    const std::int32_t mover_y_center = mover.y + mover.floor_z;
    const std::int32_t mover_y_min = mover_y_center - mover.half_h;
    const std::int32_t mover_y_max = mover_y_center + mover.half_h;
    const std::int32_t mover_z_min = mover.z + mover.floor_z;
    const std::int32_t mover_z_max = mover_z_min + mover.height_z;

    const std::size_t count = std::min(slots.size(), kSolidSweepSlotCount);
    for (std::size_t i = 0U; i < count; ++i) {
        if (i == mover_index) {
            continue;
        }
        const auto& b = slots[i];
        if (!b.solid) {
            continue;
        }

        const std::int32_t b_y_center = b.y + b.floor_z;
        const std::int32_t b_y_min = b_y_center - b.half_h;
        const std::int32_t b_y_max = b_y_center + b.half_h;
        if (!intervals_overlap(mover_y_min, mover_y_max, b_y_min, b_y_max)) {
            continue;
        }

        const std::int32_t b_z_min = b.z + b.floor_z;
        const std::int32_t b_z_max = b_z_min + b.height_z;
        if (!intervals_overlap(mover_z_min, mover_z_max, b_z_min, b_z_max)) {
            continue;
        }

        const std::int32_t b_left = b.x - b.half_w;
        const std::int32_t clearance = b_left - mover_right;
        if (clearance >= 0 && clearance < out.clamped_dx) {
            out.clamped_dx = clearance;
            out.blocker_index = i;
            out.col_flag_bit7 = true;
            out.push_timer = kPushContactTimerFrames;
            out.nudge_blocker = (clearance == 0 && b.pushable);
        }
    }

    return out;
}

} // namespace oasis::game::entities
