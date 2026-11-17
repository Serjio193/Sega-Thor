#include "game/combat/weapon_hitbox_builder.hpp"

#include <algorithm>

namespace oasis::game::combat {

WorldHitbox3D build_world_weapon_hitbox(
    std::int16_t ex,
    std::int16_t ey,
    std::int16_t ez,
    std::int16_t y_off,
    std::uint16_t facing,
    const LocalHitbox& local_box) noexcept
{
    std::int16_t dx1 = local_box.dx1;
    std::int16_t dx2 = local_box.dx2;
    if (facing == WeaponHitboxConfig::kMirrorFacingId) {
        const std::int16_t orig_dx1 = dx1;
        dx1 = static_cast<std::int16_t>(-dx2);
        dx2 = static_cast<std::int16_t>(-orig_dx1);
    }

    const auto y_bias = static_cast<std::int16_t>(ey + y_off);
    const auto z_bias = static_cast<std::int16_t>(ez + y_off);

    return WorldHitbox3D{
        .x1 = static_cast<std::int16_t>(ex + dx1),
        .y1 = static_cast<std::int16_t>(y_bias + local_box.dy1),
        .x2 = static_cast<std::int16_t>(ex + dx2),
        .y2 = static_cast<std::int16_t>(y_bias + local_box.dy2),
        .z1 = static_cast<std::int16_t>(z_bias + local_box.dz1),
        .z2 = static_cast<std::int16_t>(z_bias + local_box.dz2),
    };
}

WorldHitbox3D build_world_weapon_hitbox_from_table(
    std::int16_t ex,
    std::int16_t ey,
    std::int16_t ez,
    std::int16_t y_off,
    std::uint16_t facing,
    std::span<const LocalHitbox, 3> table) noexcept
{
    std::size_t index = 0U;
    if (facing == WeaponHitboxConfig::kMirrorFacingId) {
        index = 1U;
    } else if (facing < table.size()) {
        index = facing;
    }
    return build_world_weapon_hitbox(ex, ey, ez, y_off, facing, table[index]);
}

FxMatchResult find_first_fx_pool_hit(
    const WeaponCallerEntity& caller,
    std::int16_t rx1,
    std::int16_t ry1,
    std::int16_t rx2,
    std::int16_t ry2,
    std::int16_t rz,
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept
{
    const auto ex = caller.pos_x;
    const auto ey_bias = static_cast<std::int16_t>(caller.pos_y + caller.elevation);
    const auto ez_bias = static_cast<std::int16_t>(caller.pos_z + caller.elevation);

    return find_first_fx_pool_hit_abs(
        static_cast<std::int16_t>(ex + rx1),
        static_cast<std::int16_t>(ey_bias + ry1),
        static_cast<std::int16_t>(ex + rx2),
        static_cast<std::int16_t>(ey_bias + ry2),
        static_cast<std::int16_t>(ez_bias + rz),
        fx_slots,
        iter_state);
}

FxMatchResult find_first_fx_pool_hit_abs(
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept
{
    iter_state.px1 = px1;
    iter_state.py1 = py1;
    iter_state.px2 = px2;
    iter_state.py2 = py2;
    iter_state.pz = pz;
    iter_state.next_slot_index = 0U;
    iter_state.active = true;

    return find_next_fx_pool_hit(fx_slots, iter_state);
}

FxMatchResult find_next_fx_pool_hit(
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept
{
    if (!iter_state.active) {
        return FxMatchResult{};
    }

    const std::size_t total_slots = std::min(fx_slots.size(), WeaponHitboxConfig::kFxPoolSlotCount);
    for (std::size_t i = iter_state.next_slot_index; i < total_slots; ++i) {
        const auto& fx = fx_slots[i];
        if (fx.state <= 0) {
            continue;
        }

        const auto fx_x = fx.pos_x;
        const auto rx = fx.half_width_x;
        if (iter_state.px1 > fx_x + rx || iter_state.px2 < fx_x - rx) {
            continue;
        }

        const auto fx_y = static_cast<std::int16_t>(fx.pos_y + fx.elevation);
        const auto ry = fx.half_width_y;
        if (iter_state.py1 > fx_y + ry || iter_state.py2 < fx_y - ry) {
            continue;
        }

        const auto fx_z = static_cast<std::int16_t>(fx.pos_z + fx.elevation);
        const auto hz = fx.height_z;
        if (iter_state.pz < fx_z || iter_state.pz > fx_z + hz) {
            continue;
        }

        iter_state.next_slot_index = i + 1U;
        return FxMatchResult{
            .hit = true,
            .slot_index = i,
            .ram_addr = WeaponHitboxConfig::kFxPoolRam + static_cast<std::uint32_t>(i * WeaponHitboxConfig::kFxPoolStrideBytes),
        };
    }

    iter_state.active = false;
    return FxMatchResult{};
}

WeaponHitResult scan_entities_for_weapon_hit(
    std::size_t self_index,
    const WorldHitbox3D& world_box,
    std::span<const WeaponTargetEntity> entities,
    std::size_t start_slot,
    std::size_t slot_count) noexcept
{
    const std::size_t end_slot = std::min(entities.size(), start_slot + slot_count);
    for (std::size_t i = start_slot; i < end_slot; ++i) {
        if (i == self_index) {
            continue;
        }

        const auto& target = entities[i];
        if (target.state <= 0) {
            continue;
        }
        if ((target.flags_38 & 0x08U) != 0U) {
            continue;
        }
        if ((target.flags_3a & 0x01U) != 0U) {
            continue;
        }

        const auto target_z_min = static_cast<std::int16_t>(target.pos_z + target.elevation);
        const auto target_z_max = static_cast<std::int16_t>(target_z_min + target.height_z);
        if (world_box.z2 < target_z_min || world_box.z1 > target_z_max) {
            continue;
        }

        const auto rx = target.half_width_x;
        if (world_box.x1 > target.pos_x + rx || world_box.x2 < target.pos_x - rx) {
            continue;
        }

        const auto target_y_base = static_cast<std::int16_t>(target.pos_y + target.elevation);
        const auto ry = target.half_width_y;
        if (world_box.y1 > target_y_base + ry || world_box.y2 < target_y_base - ry) {
            continue;
        }

        return WeaponHitResult{
            .hit = true,
            .slot_index = i,
            .ram_addr = WeaponHitboxConfig::kFullEntityPoolRam + static_cast<std::uint32_t>(i * WeaponHitboxConfig::kEntityStrideBytes),
            .world_box = world_box,
        };
    }

    return WeaponHitResult{.world_box = world_box};
}

WeaponHitResult scan_weapon_hitbox_all(
    std::size_t self_index,
    const WeaponCallerEntity& caller,
    std::span<const LocalHitbox, 3> table,
    std::span<const WeaponTargetEntity> entities,
    std::size_t slot_count) noexcept
{
    const WorldHitbox3D world_box = build_world_weapon_hitbox_from_table(
        caller.pos_x, caller.pos_y, caller.pos_z, caller.elevation, caller.facing, table);

    return scan_entities_for_weapon_hit(self_index, world_box, entities, 0U, slot_count);
}

WeaponHitResult scan_weapon_hitbox_enemies(
    std::size_t self_index,
    const WeaponCallerEntity& caller,
    std::span<const LocalHitbox, 3> table,
    std::span<const WeaponTargetEntity> entities) noexcept
{
    const WorldHitbox3D world_box = build_world_weapon_hitbox_from_table(
        caller.pos_x, caller.pos_y, caller.pos_z, caller.elevation, caller.facing, table);

    return scan_entities_for_weapon_hit(
        self_index, world_box, entities,
        WeaponHitboxConfig::kEnemyPoolStartIndex,
        WeaponHitboxConfig::kEnemyEntitySlotCount);
}

} // namespace oasis::game::combat
