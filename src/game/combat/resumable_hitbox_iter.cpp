#include "game/combat/resumable_hitbox_iter.hpp"

#include <algorithm>

namespace oasis::game::combat {

namespace {

[[nodiscard]] bool check_hit(
    const HitboxIteratorState& state,
    const HitboxEntity& entity) noexcept {
    const auto ex = entity.pos_x;
    const auto ey = static_cast<std::int16_t>(entity.pos_y + entity.elevation);
    const auto ez = static_cast<std::int16_t>(entity.pos_z + entity.elevation);

    const auto min_x = static_cast<std::int16_t>(ex - entity.half_width_x);
    const auto max_x = static_cast<std::int16_t>(ex + entity.half_width_x);
    if (state.px1 > max_x || state.px2 < min_x) {
        return false;
    }

    const auto min_y = static_cast<std::int16_t>(ey - entity.half_width_y);
    const auto max_y = static_cast<std::int16_t>(ey + entity.half_width_y);
    if (state.py1 > max_y || state.py2 < min_y) {
        return false;
    }

    const auto max_z = static_cast<std::int16_t>(ez + entity.height_z);
    if (state.is_box_probe) {
        // ROM 0x00B9AC..0x00B9B6: pz1 <= ez + hz && pz2 >= ez
        if (state.pz1 > max_z || state.pz2 < ez) {
            return false;
        }
    } else {
        // ROM 0x00B8D8..0x00B8E2: pz >= ez && pz <= ez + hz
        if (state.pz1 < ez || state.pz1 > max_z) {
            return false;
        }
    }

    return true;
}

} // namespace

HitboxMatchResult find_next_3d_hit(
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept {
    if (!iter_state.active) {
        return HitboxMatchResult{.hit = false, .entity_index = 0U, .ram_addr = 0U};
    }

    const std::size_t max_slots = std::min({entities.size(), ResumableHitboxConfig::kEntitySlotCount});

    for (std::size_t i = iter_state.next_slot_index; i < max_slots; ++i) {
        // ROM 0x00B87A: lsr.l #1, d0 / bcc
        if (((iter_state.slot_mask >> i) & 1U) == 0U) {
            continue;
        }

        const auto& ent = entities[i];

        // ROM 0x00B880..0x00B88C: tst.w 0(a0) / ble; cmpa.l a0, a6 / beq
        if (ent.state <= 0 || i == iter_state.self_index) {
            continue;
        }

        if (check_hit(iter_state, ent)) {
            // Save state for resumption: next slot index is i + 1
            iter_state.next_slot_index = i + 1U;
            return HitboxMatchResult{
                .hit = true,
                .entity_index = i,
                .ram_addr = ResumableHitboxConfig::kEntityPoolRam + static_cast<std::uint32_t>(i * ResumableHitboxConfig::kEntityStrideBytes)
            };
        }
    }

    iter_state.active = false;
    return HitboxMatchResult{.hit = false, .entity_index = 0U, .ram_addr = 0U};
}

HitboxMatchResult find_first_3d_point_hit_abs(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept {
    iter_state = HitboxIteratorState{
        .slot_mask = slot_mask,
        .next_slot_index = 0U,
        .self_index = self_index,
        .px1 = px1,
        .py1 = py1,
        .px2 = px2,
        .py2 = py2,
        .pz1 = pz,
        .pz2 = pz,
        .is_box_probe = false,
        .active = true
    };

    return find_next_3d_hit(entities, iter_state);
}

HitboxMatchResult find_first_3d_point_hit(
    std::uint32_t slot_mask,
    std::size_t self_index,
    const HitboxEntity& caller,
    const PointProbe3D& rel_probe,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept {
    // ROM 0x00B856..0x00B870: Add caller position & elevation bias
    const auto px1 = static_cast<std::int16_t>(caller.pos_x + rel_probe.x1);
    const auto px2 = static_cast<std::int16_t>(caller.pos_x + rel_probe.x2);
    const auto caller_y = static_cast<std::int16_t>(caller.pos_y + caller.elevation);
    const auto py1 = static_cast<std::int16_t>(caller_y + rel_probe.y1);
    const auto py2 = static_cast<std::int16_t>(caller_y + rel_probe.y2);
    const auto caller_z = static_cast<std::int16_t>(caller.pos_z + caller.elevation);
    const auto pz = static_cast<std::int16_t>(caller_z + rel_probe.z);

    return find_first_3d_point_hit_abs(
        slot_mask, self_index, px1, py1, px2, py2, pz, entities, iter_state);
}

HitboxMatchResult find_first_3d_box_hit_abs(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz1,
    std::int16_t pz2,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept {
    iter_state = HitboxIteratorState{
        .slot_mask = slot_mask,
        .next_slot_index = 0U,
        .self_index = self_index,
        .px1 = px1,
        .py1 = py1,
        .px2 = px2,
        .py2 = py2,
        .pz1 = pz1,
        .pz2 = pz2,
        .is_box_probe = true,
        .active = true
    };

    return find_next_3d_hit(entities, iter_state);
}

HitboxMatchResult find_first_3d_box_hit(
    std::uint32_t slot_mask,
    std::size_t self_index,
    const HitboxEntity& caller,
    const BoxProbe3D& rel_probe,
    std::span<const HitboxEntity> entities,
    HitboxIteratorState& iter_state) noexcept {
    // ROM 0x00B926..0x00B944: Add caller position & elevation bias
    const auto px1 = static_cast<std::int16_t>(caller.pos_x + rel_probe.x1);
    const auto px2 = static_cast<std::int16_t>(caller.pos_x + rel_probe.x2);
    const auto caller_y = static_cast<std::int16_t>(caller.pos_y + caller.elevation);
    const auto py1 = static_cast<std::int16_t>(caller_y + rel_probe.y1);
    const auto py2 = static_cast<std::int16_t>(caller_y + rel_probe.y2);
    const auto caller_z = static_cast<std::int16_t>(caller.pos_z + caller.elevation);
    const auto pz1 = static_cast<std::int16_t>(caller_z + rel_probe.z1);
    const auto pz2 = static_cast<std::int16_t>(caller_z + rel_probe.z2);

    return find_first_3d_box_hit_abs(
        slot_mask, self_index, px1, py1, px2, py2, pz1, pz2, entities, iter_state);
}

std::vector<std::size_t> collect_all_3d_point_hits(
    std::uint32_t slot_mask,
    std::size_t self_index,
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const HitboxEntity> entities) {
    std::vector<std::size_t> hits;
    HitboxIteratorState iter_state{};

    auto res = find_first_3d_point_hit_abs(
        slot_mask, self_index, px1, py1, px2, py2, pz, entities, iter_state);

    while (res.hit) {
        hits.push_back(res.entity_index);
        res = find_next_3d_hit(entities, iter_state);
    }

    return hits;
}

} // namespace oasis::game::combat
