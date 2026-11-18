#include "game/combat/combat_hit_response.hpp"

namespace oasis::game::combat {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    if (offset + 2U > data.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] inline std::int16_t read_be16_signed(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(data, offset));
}

[[nodiscard]] inline std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    if (offset + 4U > data.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

inline void write_be16(std::span<std::uint8_t> data, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= data.size()) {
        data[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        data[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_be32(std::span<std::uint8_t> data, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 4U <= data.size()) {
        data[offset] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
        data[offset + 1U] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
        data[offset + 2U] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        data[offset + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

[[nodiscard]] inline std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

std::pair<std::int32_t, std::int32_t> project_velocity_8way(
    std::uint16_t h_speed_x,
    std::uint16_t h_speed_y,
    std::uint16_t direction) noexcept {
    auto scale_diag = [](std::int32_t v) noexcept -> std::int32_t {
        std::int32_t half = v >> 1;
        std::int32_t quarter = half >> 1;
        return half + quarter;
    };

    const auto sx = static_cast<std::int32_t>(static_cast<std::uint32_t>(h_speed_x) << 8U);
    const auto sy = static_cast<std::int32_t>(static_cast<std::uint32_t>(h_speed_y) << 8U);
    const std::uint16_t dir = direction & 7U;

    switch (dir) {
        case 0: return {0, -sy};
        case 1: return {sx, 0};
        case 2: return {0, sy};
        case 3: return {-sx, 0};
        case 4: return {scale_diag(sx), -scale_diag(sy)};
        case 5: return {scale_diag(sx), scale_diag(sy)};
        case 6: return {-scale_diag(sx), scale_diag(sy)};
        case 7: return {-scale_diag(sx), -scale_diag(sy)};
        default: return {0, 0};
    }
}

KnockbackVelocity calculate_knockback(
    KnockbackMode mode,
    std::uint16_t attacker_facing,
    bool is_airborne) noexcept {
    KnockbackVelocity v{};
    switch (mode) {
        case KnockbackMode::Light: {
            if (!is_airborne) {
                const auto [vx, vy] = project_velocity_8way(0x0300U, 0x02C0U, attacker_facing);
                v.vel_x = vx;
                v.vel_y = vy;
                v.vel_z = 0;
                v.air_juggle = false;
            } else {
                v.air_juggle = true;
                const auto [vx, vy] = project_velocity_8way(0x0400U, 0x03C0U, attacker_facing);
                v.vel_x = vx;
                v.vel_y = vy;
                v.vel_z = 0x00028000;
            }
            break;
        }
        case KnockbackMode::VerticalPopup: {
            v.vel_x = 0;
            v.vel_y = 0;
            v.vel_z = 0x00070000;
            v.air_juggle = false;
            break;
        }
        case KnockbackMode::Heavy: {
            const auto [vx, vy] = project_velocity_8way(0x0400U, 0x03C0U, attacker_facing);
            v.vel_x = vx;
            v.vel_y = vy;
            v.vel_z = 0x00028000;
            v.air_juggle = false;
            break;
        }
        case KnockbackMode::Severe: {
            const auto [vx, vy] = project_velocity_8way(0x0700U, 0x0680U, attacker_facing);
            v.vel_x = vx;
            v.vel_y = vy;
            v.vel_z = 0x00030000;
            v.air_juggle = false;
            break;
        }
    }
    return v;
}

HitResponseResult apply_combat_hit_response(
    HitEntityState& target,
    HitEntityState& attacker,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx,
    bool enable_hud_tracking) {
    HitResponseResult result{};

    // HUD tracking check (0x00BE98..0x00BEB4)
    if (enable_hud_tracking &&
        target.address != CombatHitResponseConfig::kPlayerEntityRam &&
        (target.flags_3a & 0x01U) == 0U) {
        result.hud_tracked = true;
    }

    // Shield guard & facing evaluation (0x00BEBA..0x00BED0)
    if (target.guard_6d != 0U) {
        if (is_shield_facing(attacker.facing, target.facing)) {
            result.blocked = true;
        } else {
            // Guard broken on flank or rear attack
            target.guard_6d = 0U;
            result.guard_broken = true;
        }
    }

    const auto damage_word = static_cast<std::uint16_t>((packed_a4 >> 16U) & 0xFFFFU);
    const auto stun_duration = static_cast<std::uint8_t>(packed_a4 & 0xFFU);

    if (result.blocked) {
        // Blocked / Shield Hit Handling (0x00BFAE..0x00C010)
        if (attacker.address == CombatHitResponseConfig::kPlayerEntityRam &&
            (attack_type == 6U || attack_type == 10U)) {
            result.durability_reduced = true;
        }

        target.owner_7e = attacker.address;
        target.cooldown_82 = stun_duration;
        attacker.cooldown_82 = stun_duration;
        target.damage_86 = damage_word;
        target.stun_9d = 0U;
        target.hit_facing_84 = attacker.facing;

        const bool was_hit = ((attacker.flags_37 & 0x08U) != 0U);
        attacker.flags_37 |= 0x08U;
        if (!was_hit) {
            result.sfx_played = CombatHitResponseConfig::kShieldBlockSfx;
        }
        // Zero knockback velocity applied
    } else {
        // Unblocked Hit Handling (0x00BED4..0x00BFAC)
        target.owner_7e = attacker.address;
        target.cooldown_82 = stun_duration;
        attacker.cooldown_82 = stun_duration;
        target.damage_86 = damage_word;
        target.reaction_83 = 6U;
        target.stun_9d = 0U;
        target.hit_facing_84 = attacker.facing;

        const bool was_hit = ((attacker.flags_37 & 0x08U) != 0U);
        attacker.flags_37 |= 0x08U;
        if (!was_hit) {
            result.sfx_played = hit_sfx;
        }

        result.knockback_mode = resolve_knockback_mode(damage_word);
        const bool is_airborne = (target.elev_z != 0);
        result.knockback = calculate_knockback(result.knockback_mode, attacker.facing, is_airborne);

        if (result.knockback.air_juggle) {
            target.damage_86 |= 0x8000U;
        }

        target.vel_x = result.knockback.vel_x;
        target.vel_y = result.knockback.vel_y;
        target.vel_z = result.knockback.vel_z;
    }

    return result;
}

HitEntityState read_hit_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr) {
    const std::size_t off = to_ram_offset(slot_addr);
    if (off + 188U > ram.size()) {
        return HitEntityState{.address = slot_addr};
    }

    return HitEntityState{
        .address = slot_addr,
        .world_x = read_be16_signed(ram, off + CombatHitResponseConfig::kWorldXOffset),
        .world_y = read_be16_signed(ram, off + CombatHitResponseConfig::kWorldYOffset),
        .elev_z = read_be16_signed(ram, off + CombatHitResponseConfig::kElevZOffset),
        .ground_h = read_be16_signed(ram, off + CombatHitResponseConfig::kGroundHOffset),
        .facing = read_be16(ram, off + CombatHitResponseConfig::kFacingOffset),
        .flags_37 = ram[off + CombatHitResponseConfig::kFlags37Offset],
        .flags_38 = ram[off + CombatHitResponseConfig::kFlags38Offset],
        .flags_3a = ram[off + CombatHitResponseConfig::kFlags3AOffset],
        .guard_6d = ram[off + CombatHitResponseConfig::kGuard6DOffset],
        .owner_7e = read_be32(ram, off + CombatHitResponseConfig::kOwner7EOffset),
        .cooldown_82 = ram[off + CombatHitResponseConfig::kCooldown82Offset],
        .reaction_83 = ram[off + CombatHitResponseConfig::kReaction83Offset],
        .hit_facing_84 = read_be16(ram, off + CombatHitResponseConfig::kHitFacing84Offset),
        .damage_86 = read_be16(ram, off + CombatHitResponseConfig::kDamage86Offset),
        .stun_9d = ram[off + CombatHitResponseConfig::kStun9DOffset],
        .vel_x = static_cast<std::int32_t>(read_be32(ram, off + CombatHitResponseConfig::kVelXOffset)),
        .vel_y = static_cast<std::int32_t>(read_be32(ram, off + CombatHitResponseConfig::kVelYOffset)),
        .vel_z = static_cast<std::int32_t>(read_be32(ram, off + CombatHitResponseConfig::kVelZOffset))
    };
}

void write_hit_entity_raw(
    std::span<std::uint8_t> ram,
    const HitEntityState& entity) {
    const std::size_t off = to_ram_offset(entity.address);
    if (off + 188U > ram.size()) {
        return;
    }

    ram[off + CombatHitResponseConfig::kFlags37Offset] = entity.flags_37;
    ram[off + CombatHitResponseConfig::kGuard6DOffset] = entity.guard_6d;
    write_be32(ram, off + CombatHitResponseConfig::kOwner7EOffset, entity.owner_7e);
    ram[off + CombatHitResponseConfig::kCooldown82Offset] = entity.cooldown_82;
    ram[off + CombatHitResponseConfig::kReaction83Offset] = entity.reaction_83;
    write_be16(ram, off + CombatHitResponseConfig::kHitFacing84Offset, entity.hit_facing_84);
    write_be16(ram, off + CombatHitResponseConfig::kDamage86Offset, entity.damage_86);
    ram[off + CombatHitResponseConfig::kStun9DOffset] = entity.stun_9d;
    write_be32(ram, off + CombatHitResponseConfig::kVelXOffset, static_cast<std::uint32_t>(entity.vel_x));
    write_be32(ram, off + CombatHitResponseConfig::kVelYOffset, static_cast<std::uint32_t>(entity.vel_y));
    write_be32(ram, off + CombatHitResponseConfig::kVelZOffset, static_cast<std::uint32_t>(entity.vel_z));
}

HitResponseResult apply_combat_hit_response_tracked_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t target_addr,
    std::uint32_t attacker_addr,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx) {
    HitEntityState target = read_hit_entity_raw(ram, target_addr);
    HitEntityState attacker = read_hit_entity_raw(ram, attacker_addr);

    const HitResponseResult res = apply_combat_hit_response(
        target, attacker, packed_a4, attack_type, hit_sfx, true);

    if (res.hud_tracked) {
        const std::size_t t_off = to_ram_offset(CombatHitResponseConfig::kHudTrackingTimerRam);
        const std::size_t p_off = to_ram_offset(CombatHitResponseConfig::kHudTrackingTargetRam);
        if (t_off < ram.size()) {
            ram[t_off] = CombatHitResponseConfig::kDefaultHudTrackingTimer;
        }
        if (p_off + 4U <= ram.size()) {
            write_be32(ram, p_off, target_addr);
        }
    }

    write_hit_entity_raw(ram, target);
    write_hit_entity_raw(ram, attacker);
    return res;
}

HitResponseResult apply_combat_hit_response_direct_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t target_addr,
    std::uint32_t attacker_addr,
    std::uint32_t packed_a4,
    std::uint16_t attack_type,
    std::uint32_t hit_sfx) {
    HitEntityState target = read_hit_entity_raw(ram, target_addr);
    HitEntityState attacker = read_hit_entity_raw(ram, attacker_addr);

    const HitResponseResult res = apply_combat_hit_response(
        target, attacker, packed_a4, attack_type, hit_sfx, false);

    write_hit_entity_raw(ram, target);
    write_hit_entity_raw(ram, attacker);
    return res;
}

} // namespace oasis::game::combat
