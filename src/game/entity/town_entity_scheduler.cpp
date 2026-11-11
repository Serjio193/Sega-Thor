#include "game/entity/town_entity_scheduler.hpp"

#include "game/combat/damage_arithmetic.hpp"
#include "game/entity/entity_physics.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::entity {

namespace {

constexpr std::int32_t kGravityDeltaFp = 0x0000C000;
constexpr std::int32_t kTownCurbMinY = 136;

void compute_dir_velocity(std::uint16_t dir, std::int32_t speed_x_fp, std::int32_t speed_y_fp,
                          std::int32_t& out_vx, std::int32_t& out_vy) noexcept {
    // ROM 0x00E2D4: swaps & shifts speed word left by 8 into 16.16 fixed point
    const std::int32_t vx_mag = (speed_x_fp & 0xFFFF) << 8;
    const std::int32_t vy_mag = (speed_y_fp & 0xFFFF) << 8;
    switch (dir & 7U) {
    case 0: out_vx = 0;       out_vy = -vy_mag; break;
    case 1: out_vx = vx_mag;  out_vy = 0;       break;
    case 2: out_vx = 0;       out_vy = vy_mag;  break;
    case 3: out_vx = -vx_mag; out_vy = 0;       break;
    case 4: out_vx = (vx_mag * 3) / 4;  out_vy = -(vy_mag * 3) / 4; break;
    case 5: out_vx = (vx_mag * 3) / 4;  out_vy = (vy_mag * 3) / 4;  break;
    case 6: out_vx = -(vx_mag * 3) / 4; out_vy = (vy_mag * 3) / 4;  break;
    default: out_vx = -(vx_mag * 3) / 4; out_vy = -(vy_mag * 3) / 4; break;
    }
}

std::uint16_t facing_toward_delta(std::int32_t dx, std::int32_t dy) noexcept {
    // ROM 0x00E338: selects dominant cardinal axis 0=Up, 1=Right, 2=Down, 3=Left
    if (std::abs(dx) <= std::abs(dy)) {
        return (dy < 0) ? 0U : 2U;
    }
    return (dx < 0) ? 3U : 1U;
}

void emit_sound(NativeEntity& src, TownSchedulerContext& ctx, combat::SoundCommandQueue& q,
                std::uint32_t raw_cmd) noexcept {
    const std::uint32_t att = combat::attenuate_spatial_sound(
        raw_cmd, src.world_x, src.world_y, ctx.player_x, ctx.player_y, true);
    if (att != 0U) {
        src.last_sound_cmd = att;
        q.push(ctx.frame_60hz, att);
    }
}

std::int32_t rom_dist_f0d2(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2) noexcept {
    // ROM 0x00F0D2 -> 0x00E6BA exact integer Euclidean distance isqrt(dx^2 + dy^2).
    const std::int64_t dx = static_cast<std::int64_t>(x2) - x1;
    const std::int64_t dy = static_cast<std::int64_t>(y2) - y1;
    const std::uint64_t sum = static_cast<std::uint64_t>(dx * dx + dy * dy);
    std::uint64_t res = 0, bit = 1ULL << 30;
    while (bit > sum) bit >>= 2;
    std::uint64_t n = sum;
    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return static_cast<std::int32_t>(res);
}

void decide_002c_action(NativeEntity& ent, std::vector<NativeEntity>& entities,
                        TownSchedulerContext& ctx, combat::SoundCommandQueue& q) noexcept {
    // ROM 0x0255D8
    const std::uint16_t var = ent.variant_96 & 3U;
    if (var != 0U && var != 3U && !ctx.player_invisible) {
        // ROM 0x00E398 computes Manhattan distance |dx| + |dy| into d5
        const std::int32_t dist = std::abs(ctx.player_x - ent.world_x) + std::abs(ctx.player_y - ent.world_y);
        bool try_throw = false;
        if (var == 1U) {
            if (dist < 0x50 && prng_mod(ctx.prng_seed, 4U) == 0U) {
                try_throw = true;
            }
        } else if (dist < 0x80 && (step_prng(ctx.prng_seed) & 1U) != 0U) {
            try_throw = true;
        }
        if (try_throw) {
            for (auto& obj : entities) {
                if (!obj.active || obj.type_id == 0U) {
                    obj.type_id = 0x002EU;
                    obj.active = true;
                    obj.ai_timer = 0x003CU;
                    obj.frame_dir = 0xFFFFU;
                    ent.linked_slot_9e = static_cast<std::int16_t>(obj.slot_index);
                    ent.state = 0x0AU;
                    ent.anim_frame = 0x0AU;
                    return;
                }
            }
        }
    }

    // ROM 0x02562C: Idle vs Hop choice via prng_mask_a0
    if ((step_prng(ctx.prng_seed) & ent.prng_mask_a0) != 0U) {
        ent.state = 0x00U;
        ent.anim_frame = 0x00U;
        ent.ai_timer = 0x001EU; // 30 ticks idle
        return;
    }

    const std::uint16_t dir = prng_mod(ctx.prng_seed, 4U);
    ent.facing = dir;
    compute_dir_velocity(dir, 0x0240, 0x0200, ent.vx_fp, ent.vy_fp);
    ent.state = 0x02U;
    ent.anim_frame = 0x02U;
    ent.vz_fp = (static_cast<std::int32_t>(step_prng(ctx.prng_seed)) << 2) + 0x00040000;
    emit_sound(ent, ctx, q, combat::kSndEnemy002cHop);
}

void update_half_extents(NativeEntity& ent) noexcept {
    // ROM 0x00DF20..0x00DF46
    std::uint16_t d1 = ent.base_half_w;
    std::uint16_t d2 = ent.base_half_d;
    if ((ent.facing & 1U) != 0U) {
        std::swap(d1, d2);
    }
    ent.half_width = static_cast<std::int16_t>(d1);
    d2 = static_cast<std::uint16_t>((d2 + 1U) >> 1U);
    d2 = static_cast<std::uint16_t>(d2 + (d2 >> 1U));
    ent.half_depth = static_cast<std::int16_t>(d2);
}

void apply_entity_002c_damage(NativeEntity& ent, TownSchedulerContext& ctx,
                              combat::SoundCommandQueue& q) noexcept {
    const std::uint16_t raw = ent.damage_mailbox;
    ent.damage_mailbox = 0U;
    ent.facing = (ent.previous_state != 0U)
        ? static_cast<std::uint16_t>(ent.previous_state & 3U)
        : facing_toward_delta(ctx.player_x - ent.world_x, ctx.player_y - ent.world_y);
    const auto dmg_res = combat::compute_m68k_damage_step(raw, ent.defense, ent.hp);
    if (dmg_res.sound_04d9) {
        ent.flags39 |= 0x08U;
        emit_sound(ent, ctx, q, combat::kSndHeavyBit13Hit);
    }
    ent.hp = dmg_res.hp_after;

    std::uint16_t lethal_bits = static_cast<std::uint16_t>(raw & 0xC000U);
    const std::uint16_t knockback_dir = static_cast<std::uint16_t>(ent.facing ^ 2U);
    if (dmg_res.zero_hp || lethal_bits == 0xC000U) {
        if (lethal_bits == 0xC000U) {
            compute_dir_velocity(knockback_dir, 0x0700, 0x0680, ent.vx_fp, ent.vy_fp);
        } else {
            compute_dir_velocity(knockback_dir, 0x0300, 0x02E0, ent.vx_fp, ent.vy_fp);
        }
        ent.vz_fp = 0x00030000;
        lethal_bits |= 0xC000U;
    }

    if (lethal_bits == 0U) {
        compute_dir_velocity(knockback_dir, 0x0300, 0x02C0, ent.vx_fp, ent.vy_fp);
        ent.vz_fp = 0;
        ent.state = 0x04U;
        init_entity_animation(ent, 0x08U);
        ent.flash_timer = 4U;
    } else {
        ent.world_z = static_cast<std::int16_t>(ent.world_z + 16);
        ent.z_fp = static_cast<std::int32_t>(ent.world_z) * 0x10000;
        ent.flash_timer = 0U;
        ent.flags38 |= 0x08U;
        ent.invulnerable = true;
        ent.state = 0x06U;
        init_entity_animation(ent, 0x04U);
    }
}

void step_entity_002c(NativeEntity& ent, std::vector<NativeEntity>& entities,
                      std::span<const std::uint8_t> rom,
                      TownSchedulerContext& ctx, combat::SoundCommandQueue& q) noexcept {
    sync_fixed_point_from_world(ent);

    // 0x0256B8 -> 0x00E682 yield check
    if (combat::tick_entity_action_timer_yield(ent)) {
        step_entity_animation(ent, rom, kAnimTable002C);
        const std::uint16_t ddir = (ent.facing == 3U) ? 1U : (ent.facing & 3U);
        ent.frame_dir = (ent.state == 0x14U) ? 0xFFFFU : static_cast<std::uint16_t>(ent.frame_id + ddir);
        return;
    }
    if (ent.damage_mailbox != 0U) {
        apply_entity_002c_damage(ent, ctx, q);
    }

    switch (ent.state) {
    case 0x00U: {
        if (ent.ai_timer > 0U) {
            --ent.ai_timer;
        }
        if (ent.ai_timer == 0U) {
            decide_002c_action(ent, entities, ctx, q);
        }
        break;
    }
    case 0x02U: {
        if (ent.anim_key_off <= 4U) {
            break;
        }
        if (ent.vz_fp > 0) {
            std::int32_t d0 = ent.vz_fp - kGravityDeltaFp;
            if (d0 <= 0) {
                if (d0 == 0) {
                    --d0;
                }
                ent.anim_key_off = 0x000CU;
                ent.anim_step = 0U;
            }
            ent.vz_fp = d0;
            ent.z_fp += d0;
            ent.x_fp += ent.vx_fp;
            ent.y_fp += ent.vy_fp;
        } else if (ent.vz_fp < 0) {
            std::int32_t d0 = ent.vz_fp - kGravityDeltaFp;
            ent.vz_fp = d0;
            ent.z_fp += d0;
            if (ent.z_fp <= 0) {
                ent.anim_key_off = 0x0014U;
                ent.anim_step = 0U;
                ent.z_fp = 0;
                ent.vz_fp = 0;
                if ((ent.flags39 & 0x02U) != 0U) {
                    emit_sound(ent, ctx, q, 0x00500218U);
                }
            } else {
                ent.x_fp += ent.vx_fp;
                ent.y_fp += ent.vy_fp;
            }
        } else if ((ent.flags37 & 0x02U) != 0U) {
            decide_002c_action(ent, entities, ctx, q);
        }
        break;
    }
    case 0x04U: {
        if (ent.flash_timer > 0U) {
            --ent.flash_timer;
        }
        if (ent.flash_timer == 0U) {
            ent.flags39 &= ~0x08U;
            ent.flags36 &= ~0x40U;
            decide_002c_action(ent, entities, ctx, q);
        } else {
            ent.flags36 |= 0x40U;
            ent.x_fp += ent.vx_fp;
            ent.y_fp += ent.vy_fp;
        }
        break;
    }
    case 0x06U: {
        std::int32_t d1 = ent.vz_fp - kGravityDeltaFp;
        if (d1 == 0) {
            d1 = 0x0000FFFF;
        }
        ent.vz_fp = d1;
        ent.z_fp += d1;
        if (ent.z_fp > 0) {
            ent.x_fp += ent.vx_fp;
            ent.y_fp += ent.vy_fp;
            if (ent.y_fp < kTownCurbMinY * 0x10000) {
                ent.y_fp = kTownCurbMinY * 0x10000;
            }
        } else {
            ent.z_fp = 0;
            ent.vz_fp = 0;
            ent.flags38 &= ~0x08U;
            ent.flags39 &= ~0x08U;
            ent.invulnerable = false;
            if (ent.hp == 0) {
                if (ent.ground_linger_ticks == 0U) {
                    ent.ground_linger_ticks = 2U;
                } else {
                    --ent.ground_linger_ticks;
                    if (ent.ground_linger_ticks == 0U) {
                        init_entity_animation(ent, 0x06U);
                        ent.state = 0x08U;
                        ent.flags39 &= ~0x04U;
                        ent.flags38 |= 0x0CU;
                        emit_sound(ent, ctx, q, combat::kSndEnemy002cLandThud);
                    }
                }
            } else {
                decide_002c_action(ent, entities, ctx, q);
            }
        }
        break;
    }
    case 0x08U: {
        if ((ent.flags37 & 0x02U) != 0U) {
            // ROM 0x0259B6 -> 0x00F476 drop table {0x0CCC: 0x2301, 0x0147: 0x1A01}
            if ((ent.flags39 & 0x80U) == 0U) {
                const std::uint16_t r1 = step_prng(ctx.prng_seed);
                if (r1 > 0x0CCCU) {
                    (void)step_prng(ctx.prng_seed);
                }
            }
            ent.type_id = 0U;
            ent.active = false;
        }
        break;
    }
    case 0x0AU: {
        if (ent.anim_key_off == 0x000CU && ent.anim_step == 0U && ent.linked_slot_9e >= 0) {
            for (auto& proj : entities) {
                if (static_cast<int>(proj.slot_index) == ent.linked_slot_9e && proj.type_id == 0x002EU) {
                    proj.type_id = 0x002CU;
                    proj.state = 0x0CU;
                    proj.anim_frame = 0x0CU;
                    proj.shadow_idx = 0U;
                    proj.flags38 = 0x0EU;
                    proj.flags3a |= 0x08U;
                    proj.world_x = ent.world_x;
                    proj.world_y = ent.world_y;
                    proj.world_z = 24;
                    proj.x_fp = proj.world_x * 0x10000;
                    proj.y_fp = proj.world_y * 0x10000;
                    proj.z_fp = 24 * 0x10000;
                    proj.vx_fp = (ctx.player_x * 0x10000 - proj.x_fp) >> 4;
                    proj.vy_fp = (ctx.player_y * 0x10000 - proj.y_fp) >> 4;
                    proj.vz_fp = 0x00060000;
                    proj.facing = facing_toward_delta(proj.vx_fp, proj.vy_fp);
                    emit_sound(proj, ctx, q, combat::kSndEnemy002cLunge);
                    break;
                }
            }
        }
        if ((ent.flags37 & 0x02U) != 0U) {
            decide_002c_action(ent, entities, ctx, q);
        }
        break;
    }
    case 0x0CU: {
        const std::int32_t dx = std::abs(ent.world_x - ctx.player_x);
        const std::int32_t dy = std::abs(ent.world_y - ctx.player_y);
        const bool z_overlap = (ent.world_z >= ctx.player_z) && (ent.world_z <= ctx.player_z + 40);
        if (dx <= 11 && dy <= 11 && z_overlap) {
            ctx.pending_player_damage += 2;
            emit_sound(ent, ctx, q, combat::kSndEnemy002cBodyHit1);
            emit_sound(ent, ctx, q, combat::kSndEnemy002cBodyHit2);
            ent.z_fp = 0;
            ent.state = 0x0EU;
            ent.anim_frame = 0x0EU;
        } else {
            ent.vz_fp -= kGravityDeltaFp;
            ent.z_fp += ent.vz_fp;
            if (ent.z_fp > 0) {
                ent.x_fp += ent.vx_fp;
                ent.y_fp += ent.vy_fp;
            } else {
                ent.z_fp = 0;
                ent.state = 0x0EU;
                ent.anim_frame = 0x0EU;
            }
        }
        break;
    }
    case 0x0EU: {
        if ((ent.flags37 & 0x02U) != 0U) {
            ent.type_id = 0U;
            ent.active = false;
        }
        break;
    }
    case 0x10U:
    case 0x14U: {
        // ROM 0x0257EE: player must not be invisible ($FF1A20 bit 3) and must be on ground (player_z == 0)
        if (!ctx.player_invisible && ctx.player_z == 0) {
            const std::int32_t dist = rom_dist_f0d2(ctx.player_x, ctx.player_y, ent.world_x, ent.world_y);
            if (dist <= 0x30) {
                ent.flags36 &= ~0x02U;
                ent.flags37 &= ~0x01U;
                ent.state = (ent.state == 0x10U) ? 0x12U : 0x16U;
                ent.vz_fp = 0; // ROM 0x025826..0x025838: starts at 0, accelerates by 0xC000/tick in state 0x16
            }
        }
        break;
    }
    case 0x16U: {
        // ROM 0x02583E: z_fp -= vz_fp; if (z_fp <= 0) land; else vz_fp += 0xC000
        ent.z_fp -= ent.vz_fp;
        if (ent.z_fp <= 0) {
            ent.z_fp = 0;
            ent.vz_fp = 0;
            ent.anim_step = 0U;
            ent.state = 0x18U;
            emit_sound(ent, ctx, q, (ent.flags39 & 0x02U) != 0U ? 0x00800270U : 0x00500342U);
        } else {
            ent.vz_fp += kGravityDeltaFp;
        }
        break;
    }
    case 0x12U:
    case 0x18U: {
        if ((ent.flags37 & 0x02U) != 0U) {
            ent.flags38 &= ~0x0CU;
            ent.invulnerable = false;
            ent.height = 16;
            decide_002c_action(ent, entities, ctx, q);
        }
        break;
    }
    default:
        break;
    }

    sync_world_from_fixed_point(ent);
    step_entity_animation(ent, rom, kAnimTable002C);
    const std::uint16_t ddir = (ent.facing == 3U) ? 1U : (ent.facing & 3U);
    ent.frame_dir = (ent.state == 0x14U) ? 0xFFFFU : static_cast<std::uint16_t>(ent.frame_id + ddir);
}

} // namespace

void step_town_scheduler(std::vector<NativeEntity>& entities,
                         std::span<const std::uint8_t> rom,
                         std::span<const std::uint8_t> terrain_collision,
                         TownSchedulerContext& ctx,
                         combat::SoundCommandQueue& sound_queue,
                         bool advance_logic_step) noexcept {
    if (!advance_logic_step) {
        return;
    }

    const SolidActorBox actor{ctx.player_x, ctx.player_y, ctx.player_z, 10, 10, 40,
                              ctx.player_state == 0x36U};

    // ROM 0x00DEEC scheduler pass in exact slot order
    for (auto& ent : entities) {
        if (!ent.active || ent.type_id < 0x0016U) {
            continue;
        }
        ent.last_sound_cmd = 0U;

        if (ent.type_id == 0x002EU) {
            if (ent.ai_timer > 0U) {
                --ent.ai_timer;
                if (ent.ai_timer == 0U) {
                    ent.type_id = 0U;
                    ent.active = false;
                }
            }
            continue;
        }

        if (ent.type_id == 0x002CU) {
            if (!ctx.skip_post_move_repulsion) {
                apply_solid_body_repulsion(actor, ent);
            }
            step_entity_002c(ent, entities, rom, ctx, sound_queue);

            // ROM 0x02E8C8 water channel current & splash flag check
            ent.flags39 &= static_cast<std::uint8_t>(~0x02U);
            if (!terrain_collision.empty()) {
                const int tx = std::clamp(ent.world_x >> 3, 0, 63);
                const int ty = std::clamp(ent.world_y >> 3, 0, 63);
                const std::uint8_t cell = terrain_collision[static_cast<std::size_t>(ty) * 64U + static_cast<std::size_t>(tx)];
                if ((cell & 0x20U) != 0U && (ent.flags37 & 0x40U) == 0U) {
                    ent.flags39 |= 0x02U;
                    if (ent.world_z == 0 && ent.world_y < 0x1C0) {
                        ent.y_fp += (ent.world_y >= 0x1B0) ? 0x00010000 : 0x00020000;
                        sync_world_from_fixed_point(ent);
                    }
                }
            }
            if ((ent.flags39 & 0x02U) != 0U && ent.world_z == 0) {
                ent.water_ripple_tick = static_cast<std::uint8_t>((ent.water_ripple_tick + 1U) % 9U);
            }
        } else if (ent.type_id == 0x0060U) {
            step_entity_animation(ent, rom, kAnimTable0060);
            ent.frame_dir = ent.frame_id;
        }

        update_half_extents(ent);
    }
}

} // namespace oasis::game::entity