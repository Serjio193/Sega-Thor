#include "game/combat/hit_reactions_and_sparks.hpp"

namespace oasis::game::combat {

namespace {

void apply_directional_xy(
    HitReactionPhysicsState& target,
    std::int32_t mag_x,
    std::int32_t mag_y,
    std::uint8_t attacker_facing) noexcept {
    switch (attacker_facing & 3U) {
        case 0U: // Up
            target.vx = 0;
            target.vy = -mag_y;
            break;
        case 1U: // Right
            target.vx = mag_x;
            target.vy = 0;
            break;
        case 2U: // Down
            target.vx = 0;
            target.vy = mag_y;
            break;
        case 3U: // Left
            target.vx = -mag_x;
            target.vy = 0;
            break;
    }
}

} // namespace

void apply_hit_reaction_physics(
    HitReactionPhysicsState& target,
    HitReactionKind kind,
    std::uint8_t attacker_facing) noexcept {
    if (kind == HitReactionKind::LightGroundOrJuggle) {
        if (!target.airborne) {
            apply_directional_xy(target, 0x0300, 0x02C0, attacker_facing);
            target.vz = 0;
            return;
        }
        // Airborne fallthrough: set $86 bit 7 and fall through to MediumKnockback (4)
        target.flags_86 |= 0x80U;
        kind = HitReactionKind::MediumKnockback;
    }

    switch (kind) {
        case HitReactionKind::VerticalLauncher:
            target.vx = 0;
            target.vy = 0;
            target.vz = 0x70000;
            break;
        case HitReactionKind::MediumKnockback:
            apply_directional_xy(target, 0x0400, 0x03C0, attacker_facing);
            target.vz = 0x28000;
            break;
        case HitReactionKind::HeavyKnockback:
            apply_directional_xy(target, 0x0700, 0x0680, attacker_facing);
            target.vz = 0x30000;
            break;
        case HitReactionKind::LightGroundOrJuggle:
            break;
    }
}

void HitSparkPool4::reset() noexcept {
    slots_.fill(HitSparkSlot{});
}

bool HitSparkPool4::spawn(std::int32_t x, std::int32_t y) noexcept {
    for (auto& slot : slots_) {
        if (!slot.active) {
            slot.active = true;
            slot.type = kDefaultSparkType;
            slot.timer = kDefaultSparkTimer;
            slot.x = x;
            slot.y = y;
            return true;
        }
    }
    return false;
}

void HitSparkPool4::tick_sparks() noexcept {
    for (auto& slot : slots_) {
        if (!slot.active) {
            continue;
        }
        if (slot.timer > 0U) {
            --slot.timer;
        }
        if (slot.timer == 0U) {
            slot.active = false;
        }
    }
}

std::size_t HitSparkPool4::active_count() const noexcept {
    std::size_t count = 0U;
    for (const auto& slot : slots_) {
        if (slot.active) {
            ++count;
        }
    }
    return count;
}

ShieldParryResult apply_shield_parry(
    std::uint16_t weapon_ff198c,
    std::int32_t spark_x,
    std::int32_t spark_y,
    HitSparkPool4& pool) noexcept {
    ShieldParryResult res{};
    res.sfx_cmd = 0x00900202U;
    res.ali_recoil_triggered = (weapon_ff198c == 6U || weapon_ff198c == 10U);
    res.spark_spawned = pool.spawn(spark_x, spark_y);
    return res;
}

} // namespace oasis::game::combat
