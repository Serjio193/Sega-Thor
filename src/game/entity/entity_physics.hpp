#pragma once

#include "game/entity/native_entity.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::entity {

// Authentic 3D solid-body repulsion & airborne death physics extracted from ROM
// routines 0x00A0C8..0x00A188 (solid separation), 0x0092B0..0x009390 (velocity integration),
// 0x00BF28..0x00BF96 (knockback tiers), and 0x00E502..0x00E530 (gravity & Z=0 landing).
struct SolidActorBox {
    std::int32_t world_x{0};
    std::int32_t world_y{0};
    std::int16_t world_z{0};
    std::int16_t half_width{10};
    std::int16_t half_depth{10};
    std::int16_t height{40};
    bool movement_locked{false}; // bit 5 of +0x37
};

// Initializes 16.16 fixed-point position fields from integer world coordinates if uninitialized.
void sync_fixed_point_from_world(NativeEntity& ent) noexcept;

// Syncs integer world_x/y/z coordinates from arithmetic-shifted 16.16 fixed-point accumulators.
void sync_world_from_fixed_point(NativeEntity& ent) noexcept;

// Evaluates ROM 0x00A0C8..0x00A188 solid-body push repulsion against entity `ent`.
// When actor overlaps entity in X & Z and the Y edge residual d6 == 0 (e.g. actor_y - ent_y == 17
// for player half_d=10 and 0x002C half_d=7), applies subq.w #1 to entity's Y (-1.0 px push).
// Returns true if repulsion shifted the entity.
bool apply_solid_body_repulsion(const SolidActorBox& actor, NativeEntity& ent) noexcept;

// Clamps player X/Y motion (ROM 0x009DB0..0x00A188) against solid entities on all 4 cardinal
// directions while preserving fractional subpixel accumulators and pushing touching entities.
void clamp_player_motion_against_solid_entities(std::int32_t old_x_fixed,
                                                 std::int32_t old_y_fixed,
                                                 std::int32_t& new_x_fixed,
                                                 std::int32_t& new_y_fixed,
                                                 std::span<NativeEntity> entities,
                                                 bool player_movement_locked = false,
                                                 std::int16_t player_z = 0) noexcept;

// Executes one 30Hz physics integration + death lifecycle tick for Entity 0x002C:
// - Integrates vx_fp / vy_fp (clamping Y >= y_min boundary, Town curb 136)
// - Integrates vz_fp & applies gravity -0xC000 (-0.75/tick) with ROM 0x00E51E zero-apex
//   subq.w #1 preservation (0 -> 0x0000FFFF)
// - Transitions State 0x06 -> State 0x08 (Anim 0x06, flags38 |= 0x04, sound 0x0080035A) on z <= 0
// - Counts down corpse dwell ticks in State 0x08 before clearing type_id = 0x0000 / active = false.
void step_002c_death_and_velocity(NativeEntity& ent, std::int32_t y_min_bound = 136) noexcept;

} // namespace oasis::game::entity
