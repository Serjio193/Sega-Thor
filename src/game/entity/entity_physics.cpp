#include "game/entity/entity_physics.hpp"

#include <algorithm>

namespace oasis::game::entity {

namespace {

constexpr std::int32_t kGravityDeltaFp = 0x0000C000; // ROM 0x00E516: subi.l #$C000, d1
constexpr std::uint32_t kDeathLandingSound = 0x0080035AU; // ROM 0x00DF1C -> 0x00E7E8
constexpr std::uint8_t kCorpseReleaseTicks = 9U; // F825->F843 (9 logic ticks)

} // namespace

void sync_fixed_point_from_world(NativeEntity& ent) noexcept {
    if ((ent.x_fp >> 16) != ent.world_x) {
        ent.x_fp = ent.world_x * 0x10000;
    }
    if ((ent.y_fp >> 16) != ent.world_y) {
        ent.y_fp = ent.world_y * 0x10000;
    }
    if ((ent.z_fp >> 16) != ent.world_z) {
        ent.z_fp = static_cast<std::int32_t>(ent.world_z) * 0x10000;
    }
}

void sync_world_from_fixed_point(NativeEntity& ent) noexcept {
    ent.world_x = ent.x_fp >> 16;
    ent.world_y = ent.y_fp >> 16;
    ent.world_z = static_cast<std::int16_t>(ent.z_fp >> 16);
}

bool apply_solid_body_repulsion(const SolidActorBox& actor, NativeEntity& ent) noexcept {
    if (!ent.active || ent.type_id == 0U || actor.movement_locked) {
        return false;
    }
    if ((ent.flags38 & 0x04U) != 0U) { // ROM 0x00A0E8: btst #2, $38(a5)
        return false;
    }
    if (ent.state != 0x00U && ent.state != 0x02U) {
        return false;
    }

    sync_fixed_point_from_world(ent);

    // ROM 0x00A0F2..0x00A118: Y edge proximity calculation
    // d2 = actor.world_y - actor.half_depth, d0 = actor.half_depth * 2?
    // Actually in ROM:
    // d4 = actor.world_y + actor.half_depth (bottom edge when pushing up from below)
    // d6 = ent.world_y + ent.half_depth
    // We require ent_bottom < actor_bottom (pushing northward) AND
    // d6 = -( (ent.world_y + ent.half_depth) - (actor.world_y - actor.half_depth) )
    // Wait: at F777: actor.y=200 (half_d=10), ent.y=183 (half_d=7):
    // ent_top_vs_actor: (actor.y - 10) - (ent.y + 7) = 190 - 190 = 0 (d6 == 0!)
    const std::int32_t actor_top = actor.world_y - actor.half_depth;
    const std::int32_t ent_bottom = ent.world_y + ent.half_depth;
    if (ent.world_y >= actor.world_y) {
        return false;
    }
    std::int32_t d6 = ent_bottom - actor_top;
    d6 = -d6;
    if (d6 < 0) {
        d6 = 0;
    }
    if (d6 >= actor.half_depth * 2) {
        return false;
    }

    // X AABB overlap (ROM 0x00A11C..0x00A132)
    const std::int32_t actor_left = actor.world_x - actor.half_width;
    const std::int32_t actor_right = actor.world_x + actor.half_width;
    const std::int32_t ent_left = ent.world_x - ent.half_width;
    const std::int32_t ent_right = ent.world_x + ent.half_width;
    if (ent_right <= actor_left || ent_left >= actor_right) {
        return false;
    }

    // Z AABB overlap (ROM 0x00A136..0x00A152)
    const std::int32_t actor_z_top = actor.world_z + actor.height;
    const std::int32_t ent_z_top = ent.world_z + ent.height;
    if (ent_z_top <= actor.world_z || ent.world_z >= actor_z_top) {
        return false;
    }

    // ROM 0x00A166..0x00A180: Only executes subq.w #1, $76(a5) when d6 == 0!
    if (d6 != 0) {
        return false;
    }

    ent.y_fp -= 0x10000;
    ent.world_y = ent.y_fp >> 16;
    return true;
}

void clamp_player_motion_against_solid_entities(std::int32_t old_x_fixed,
                                                 std::int32_t old_y_fixed,
                                                 std::int32_t& new_x_fixed,
                                                 std::int32_t& new_y_fixed,
                                                 std::span<NativeEntity> entities,
                                                 bool player_movement_locked,
                                                 std::int16_t player_z) noexcept {
    constexpr std::int32_t kHW = 10, kHD = 10, kH = 40;
    const auto is_solid = [&](const NativeEntity& e) {
        return e.active && e.type_id >= 0x0016U && (e.flags38 & 0x04U) == 0U &&
               (e.flags3a & 0x02U) == 0U && (e.state == 0x00U || e.state == 0x02U) &&
               (e.world_z + e.height > player_z) && (e.world_z < player_z + kH);
    };

    const std::int32_t old_x = old_x_fixed >> 16;
    const std::int32_t old_y = old_y_fixed >> 16;
    const std::int32_t req_y = new_y_fixed >> 16;
    std::int32_t final_y = req_y;
    if (req_y != old_y) {
        const bool up = (req_y < old_y);
        std::int32_t allowed = up ? (old_y - req_y) : (req_y - old_y);
        const std::int32_t p_left = old_x - kHW, p_right = old_x + kHW;
        for (auto& ent : entities) {
            if (!is_solid(ent)) continue;
            if (up ? (ent.world_y >= old_y) : (ent.world_y <= old_y)) continue;
            const std::int32_t gap = up ? ((old_y - kHD) - (ent.world_y + ent.half_depth))
                                        : ((ent.world_y - ent.half_depth) - (old_y + kHD));
            const std::int32_t d6 = std::max<std::int32_t>(0, gap);
            if (d6 >= kHD * 2) continue;
            if (ent.world_x + ent.half_width <= p_left || ent.world_x - ent.half_width >= p_right) continue;
            allowed = std::min(allowed, d6);
            if (d6 == 0 && !player_movement_locked && (ent.flags37 & 0x40U) == 0U) {
                sync_fixed_point_from_world(ent);
                ent.y_fp += up ? -0x10000 : 0x10000;
                ent.world_y = ent.y_fp >> 16;
            }
        }
        final_y = up ? (old_y - allowed) : (old_y + allowed);
        new_y_fixed = static_cast<std::int32_t>((static_cast<std::uint32_t>(final_y) << 16) |
                                                (static_cast<std::uint32_t>(new_y_fixed) & 0xFFFFU));
    }

    const std::int32_t req_x = new_x_fixed >> 16;
    if (req_x != old_x) {
        const bool left = (req_x < old_x);
        std::int32_t allowed = left ? (old_x - req_x) : (req_x - old_x);
        const std::int32_t p_top = final_y - kHD, p_bot = final_y + kHD;
        for (auto& ent : entities) {
            if (!is_solid(ent)) continue;
            if (left ? (ent.world_x >= old_x) : (ent.world_x <= old_x)) continue;
            const std::int32_t gap = left ? ((old_x - kHW) - (ent.world_x + ent.half_width))
                                          : ((ent.world_x - ent.half_width) - (old_x + kHW));
            const std::int32_t d6 = std::max<std::int32_t>(0, gap);
            if (d6 >= kHW * 2) continue;
            if (ent.world_y + ent.half_depth <= p_top || ent.world_y - ent.half_depth >= p_bot) continue;
            allowed = std::min(allowed, d6);
            if (d6 == 0 && !player_movement_locked && (ent.flags37 & 0x40U) == 0U) {
                sync_fixed_point_from_world(ent);
                ent.x_fp += left ? -0x10000 : 0x10000;
                ent.world_x = ent.x_fp >> 16;
            }
        }
        const std::int32_t final_x = left ? (old_x - allowed) : (old_x + allowed);
        new_x_fixed = static_cast<std::int32_t>((static_cast<std::uint32_t>(final_x) << 16) |
                                                (static_cast<std::uint32_t>(new_x_fixed) & 0xFFFFU));
    }
}

void step_002c_death_and_velocity(NativeEntity& ent, std::int32_t y_min_bound) noexcept {
    if (!ent.active || ent.type_id == 0U) {
        return;
    }
    sync_fixed_point_from_world(ent);

    if (ent.state == 0x06U) {
        // Horizontal / depth knockback integration during airborne death state
        ent.x_fp += ent.vx_fp;
        ent.y_fp += ent.vy_fp;
        const std::int32_t min_y_fp = y_min_bound * 0x10000;
        if (ent.y_fp < min_y_fp) {
            ent.y_fp = min_y_fp;
        }
    }

    if (ent.state == 0x06U) {
        // ROM 0x00E512..0x00E524: vz_fp -= 0xC000; if zero result -> subq.w #1 (0x0000FFFF)
        std::int32_t next_vz = ent.vz_fp - kGravityDeltaFp;
        if (next_vz == 0) {
            next_vz = 0x0000FFFF;
        }
        ent.vz_fp = next_vz;
        ent.z_fp += ent.vz_fp;

        if (ent.z_fp <= 0) {
            ent.z_fp = 0;
            ent.vz_fp = 0;
            // F821: lands z=0; F823: still state 0x06; F825: state 0x08 + 0x0080035A
            if (ent.ground_linger_ticks == 0U) {
                ent.ground_linger_ticks = 2U;
            } else {
                --ent.ground_linger_ticks;
                if (ent.ground_linger_ticks == 0U) {
                    ent.state = 0x08U;
                    ent.anim_frame = 0x06U;
                    ent.flags38 |= 0x04U; // bits 2 & 3 set = 0x0C
                    ent.ground_linger_ticks = kCorpseReleaseTicks;
                    ent.last_sound_cmd = kDeathLandingSound;
                }
            }
        }
    } else if (ent.state == 0x08U) {
        if (ent.ground_linger_ticks > 0U) {
            --ent.ground_linger_ticks;
            if (ent.ground_linger_ticks == 0U) {
                ent.type_id = 0x0000U;
                ent.active = false;
            }
        }
    }

    sync_world_from_fixed_point(ent);
}

} // namespace oasis::game::entity
