#include "game/world/aabb_solid_sweep_3axis.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::world {

namespace {

[[nodiscard]] constexpr bool intervals_overlap(
    std::int32_t a_min,
    std::int32_t a_max,
    std::int32_t b_min,
    std::int32_t b_max) noexcept {
    return a_max > b_min && a_min < b_max;
}

[[nodiscard]] constexpr std::int32_t compute_axis_gap(
    SolidSweepAxis axis,
    const entities::SolidSweepBlocker& mover,
    const entities::SolidSweepBlocker& b) noexcept {
    if (axis == SolidSweepAxis::NegX) {
        return (mover.x - mover.half_w) - (b.x + b.half_w);
    }
    const std::int32_t mover_y = mover.y + mover.floor_z;
    const std::int32_t b_y = b.y + b.floor_z;
    if (axis == SolidSweepAxis::PosY) {
        return (b_y - b.half_h) - (mover_y + mover.half_h);
    }
    return (mover_y - mover.half_h) - (b_y + b.half_h);
}

[[nodiscard]] constexpr bool ortho_xy_overlaps(
    SolidSweepAxis axis,
    const entities::SolidSweepBlocker& mover,
    const entities::SolidSweepBlocker& b) noexcept {
    if (axis == SolidSweepAxis::NegX) {
        const std::int32_t my = mover.y + mover.floor_z;
        const std::int32_t by = b.y + b.floor_z;
        return intervals_overlap(my - mover.half_h, my + mover.half_h,
                                 by - b.half_h, by + b.half_h);
    }
    return intervals_overlap(mover.x - mover.half_w, mover.x + mover.half_w,
                             b.x - b.half_w, b.x + b.half_w);
}

} // namespace

SolidSweep3AxisResult sweep_aabb_solid_3axis(
    SolidSweepAxis axis,
    const entities::SolidSweepBlocker& mover,
    std::size_t mover_index,
    std::int32_t delta,
    std::span<const entities::SolidSweepBlocker> slots,
    bool mover_can_push) noexcept {
    SolidSweep3AxisResult out{};
    out.clamped_delta = delta;

    const bool expects_positive = (axis == SolidSweepAxis::PosY);
    if ((expects_positive && delta <= 0) || (!expects_positive && delta >= 0)) {
        return out;
    }

    const std::int32_t sign = expects_positive ? 1 : -1;
    std::int32_t min_gap = std::abs(delta);
    const std::int32_t mover_z_min = mover.z + mover.floor_z;
    const std::int32_t mover_z_max = mover_z_min + mover.height_z;

    const std::size_t count = std::min(slots.size(), entities::kSolidSweepSlotCount);
    for (std::size_t i = 0U; i < count; ++i) {
        if (i == mover_index) {
            continue;
        }
        const auto& b = slots[i];
        if (!b.solid || !ortho_xy_overlaps(axis, mover, b)) {
            continue;
        }
        const std::int32_t b_z_min = b.z + b.floor_z;
        const std::int32_t b_z_max = b_z_min + b.height_z;
        if (!intervals_overlap(mover_z_min, mover_z_max, b_z_min, b_z_max)) {
            continue;
        }

        const std::int32_t gap = compute_axis_gap(axis, mover, b);
        if (gap >= 0 && gap < min_gap) {
            min_gap = gap;
            out.clamped_delta = sign * gap;
            out.blocker_index = i;
            out.contact_type = kSolidContactType;
            out.hit_flag_bit7 = true;
            out.push_delta = (gap == 0 && b.pushable && mover_can_push)
                ? static_cast<std::int8_t>(sign)
                : 0;
        }
    }

    return out;
}

} // namespace oasis::game::world
