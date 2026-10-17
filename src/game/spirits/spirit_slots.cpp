#include "game/spirits/spirit_slots.hpp"
#include "game/player/player.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::spirits {

namespace {

[[nodiscard]] std::int32_t read_i16(
    const entities::EntityRecordView& record,
    const std::size_t offset) noexcept {
    const auto value = record.read_u16(offset);
    return value ? static_cast<std::int16_t>(*value) : 0;
}

[[nodiscard]] bool horizontal_overlap(std::int32_t query_min,
                                      std::int32_t query_max,
                                      std::int32_t center,
                                      std::int32_t extent) noexcept {
    return query_min <= center + extent && query_max >= center - extent;
}

} // namespace

std::optional<std::size_t> find_observed_target(
    const entities::EntityPoolView& owner_pool,
    const entities::EntityPoolView& target_pool,
    const ObservedTargetQuery& query) noexcept {
    const auto owner = owner_pool.record_view(query.owner_index);
    if (!owner) {
        return std::nullopt;
    }

    const auto owner_x = read_i16(*owner, entities::kEntityFieldPositionX);
    const auto owner_y = read_i16(*owner, entities::kEntityFieldPositionY) +
                         read_i16(*owner, entities::kEntityFieldWord14);
    const auto owner_z = read_i16(*owner, entities::kEntityFieldWord10) +
                         read_i16(*owner, entities::kEntityFieldWord14);
    const auto query_x_min = owner_x + query.relative_x_min;
    const auto query_x_max = owner_x + query.relative_x_max;
    const auto query_y_min = owner_y + query.relative_y_min;
    const auto query_y_max = owner_y + query.relative_y_max;
    const auto query_z_min = owner_z + query.relative_z_min;
    const auto query_z_max = owner_z + query.relative_z_max;

    for (std::size_t index = 0; index < target_pool.spec().record_count; ++index) {
        // The original owner is in FF2D8C and targets are in FF19E8, so an
        // equal numeric index is not the owner-pointer exclusion from B922.
        if (!target_pool.active(index)) {
            continue;
        }
        const auto record = target_pool.record_view(index);
        if (!record) {
            continue;
        }

        const auto target_x = read_i16(*record, entities::kEntityFieldPositionX);
        const auto target_y =
            read_i16(*record, entities::kEntityFieldPositionY) +
            read_i16(*record, entities::kEntityFieldWord14);
        const auto target_z = read_i16(*record, entities::kEntityFieldWord10) +
                              read_i16(*record, entities::kEntityFieldWord14);
        if (!horizontal_overlap(
                query_x_min, query_x_max, target_x,
                read_i16(*record, entities::kEntityFieldWord42)) ||
            !horizontal_overlap(
                query_y_min, query_y_max, target_y,
                read_i16(*record, entities::kEntityFieldWord44)) ||
            query_z_min < target_z ||
            query_z_max >= target_z +
                               read_i16(*record, entities::kEntityFieldWord4A)) {
            continue;
        }

        const auto type = record->read_u16(entities::kEntityFieldType);
        if (type && *type == query.required_type) {
            return index;
        }
        return std::nullopt;
    }
    return std::nullopt;
}

SpiritDispatchTrace trace_observed_dispatch(
    const SpiritDispatchInput& input) noexcept {
    SpiritDispatchTrace trace{};
    trace.guard_flags = input.guard_flags;

    // 0x31B80 reaches the slot-gated branch only when both observed bits in
    // entity +0x41 are set. This preserves the raw contract without naming
    // the buttons represented by those bits.
    constexpr std::uint8_t kRequiredInputBits = 0x0AU;
    if ((input.input_41 & kRequiredInputBits) != kRequiredInputBits) {
        return trace;
    }

    trace.entered_observed_path = true;
    trace.slot_available =
        (input.slot_flags &
         static_cast<std::uint8_t>(1U << kObservedDispatchSlot)) != 0;

    if (trace.slot_available &&
        (input.guard_flags &
         static_cast<std::uint8_t>(1U << kObservedDispatchGuardBit)) == 0) {
        trace.selectors[trace.selector_count++] = kObservedResourceSelector;
        trace.guard_flags |= static_cast<std::uint8_t>(
            1U << kObservedDispatchGuardBit);
    }

    // The observed path falls through to 0x31BF2 and queues selector 0x15
    // even when the slot-gated resource branch is skipped.
    trace.selectors[trace.selector_count++] = kObservedEffectSelector;
    return trace;
}

std::optional<ObservedSummonSeed> initialize_observed_summon(
    const ObservedSummonCaller& caller) noexcept {
    // 0x7A10 tests bit 1 of +0x37 and only enters 0x846C for state +0x30=0x18.
    if ((caller.flags_37 & 0x02U) != 0 || caller.state_30 != 0x18U) {
        return std::nullopt;
    }

    // These fields are the direct writes performed by 0x846C. The routine's
    // later table-derived velocity calculation is deliberately left raw.
    return ObservedSummonSeed{
        .raw_type = kObservedSummonType,
        .position_x = caller.position_x,
        .position_y = caller.position_y,
        .raw_depth = kObservedSummonDepth,
        .raw_66 = caller.raw_17,
        .raw_14 = caller.raw_14,
        .raw_resource_18 = kObservedSummonResource,
        .raw_resource_5a = kObservedSummonResource,
        .raw_a6 = 0,
        .raw_aa = 0,
    };
}

bool try_summon_spirit(
    ActiveSpirit& spirit,
    const SpiritSlots& slots,
    std::uint8_t slot_id,
    std::int32_t px,
    std::int32_t py,
    std::int32_t pz,
    player::PlayerState& player,
    std::span<const std::uint8_t> rom) noexcept {
    if (!slots.acquired(slot_id)) {
        return false;
    }
    if (spirit.active) {
        return false; // NO_DUPLICATE_SLOT: slot already occupied
    }
    if (player.sp <= 0) {
        return false; // RESOURCE_GATE
    }

    spirit.active = true;
    spirit.slot = slot_id;
    spirit.type_id = (slot_id == 0) ? kSpiritTypeDytto :
                     ((slot_id == 1) ? kSpiritTypeEfreet :
                     ((slot_id == 2) ? kSpiritTypeBow : kSpiritTypeShade));
    spirit.state = kSpiritStateAppear;
    spirit.anim_selector = kSpiritAnimAppear;
    spirit.world_x = px;
    spirit.world_y = py;
    spirit.world_z = pz + 16; // elevated 16 px above ground (ROM 0x018D08)
    spirit.target_x = px;
    spirit.target_y = py;
    spirit.direction = 2; // facing Down initially (ROM 0x014674)
    spirit.state_timer = 20; // duration of appear sequence
    spirit.sp_timer = kSpiritSpCostInterval;
    spirit.halted = false;
    spirit.pieces.clear();
    spirit.tile_base = 0;

    if (!rom.empty() && spirit.type_id == kSpiritTypeDytto) {
        // Dytto frame 0 at ROM 0x11F506 (from 0x11F360 + rel 0x01A6)
        spirit.pieces = entities::decode_sprite_frame_from_rom(rom, 0x11F506U);
        if (!spirit.pieces.empty()) {
            spirit.tile_base = spirit.pieces.front().tile_index;
        }
    }
    return true;
}

void update_active_spirit(
    ActiveSpirit& spirit,
    const player::PlayerState& player,
    player::PlayerState& mutable_player) noexcept {
    if (!spirit.active) {
        return;
    }

    // Resource consumption (ROM 0x011B64..0x011B86)
    if (spirit.sp_timer > 0) {
        --spirit.sp_timer;
    }
    if (spirit.sp_timer == 0) {
        spirit.sp_timer = kSpiritSpCostInterval;
        if (mutable_player.sp > 0) {
            --mutable_player.sp;
        }
        if (mutable_player.sp == 0) {
            despawn_spirit(spirit);
            return;
        }
    }

    // State machine
    if (spirit.state == kSpiritStateAppear) {
        if (spirit.state_timer > 0) {
            --spirit.state_timer;
        }
        if (spirit.state_timer == 0) {
            spirit.state = kSpiritStateFollow;
            spirit.anim_selector = kSpiritAnimHover;
        }
    } else if (spirit.state == kSpiritStateFollow) {
        // Target calculation (ROM 0x01215E & 0x0123B4)
        const auto p_world_x = static_cast<std::int32_t>(player.x_fixed / 0x10000);
        const auto p_world_y = static_cast<std::int32_t>(player.y_fixed / 0x10000);
        spirit.target_x = p_world_x;
        spirit.target_y = p_world_y - 16;

        const auto dx = spirit.target_x - spirit.world_x;
        const auto dy = spirit.target_y - spirit.world_y;

        if (std::abs(dx) > std::abs(dy)) {
            spirit.direction = (dx > 0) ? 1 : 3;
        } else if (dy != 0) {
            spirit.direction = (dy > 0) ? 2 : 0;
        }

        const auto dist = std::abs(dx) + std::abs(dy);
        if (dist > 8) {
            constexpr std::int32_t kSpeed = 2;
            spirit.world_x += std::clamp(dx, -kSpeed, kSpeed);
            spirit.world_y += std::clamp(dy, -kSpeed, kSpeed);
        }
    } else if (spirit.state == kSpiritStateDespawn) {
        despawn_spirit(spirit);
    } else if (spirit.state == kSpiritStateBubbleCast) {
        // Bubble cast handled by update_dytto_ability (ROM 0x011DB6)
    } else {
        spirit.halted = true;
    }
}

std::uint8_t decode_dytto_command_input(
    ActiveSpirit& spirit,
    bool button_a_pressed) noexcept {
    if (!spirit.active) {
        spirit.button_a_hold_timer = 0;
        spirit.double_tap_timer = 0;
        spirit.prev_button_a = false;
        return 0;
    }

    std::uint8_t cmd = 0;
    // Double tap timer countdown (ROM 0x011A8E..0x011A9E)
    if (spirit.double_tap_timer > 0) {
        --spirit.double_tap_timer;
        if (spirit.double_tap_timer == 0) {
            // Window expired without second press: emits Command 1 (Single Tap)
            cmd = 1;
        }
    }

    if (button_a_pressed) {
        // Button A is held (ROM 0x011AA4..0x011AD4)
        ++spirit.button_a_hold_timer;
        if (!spirit.prev_button_a) {
            // Fresh press edge: check double-tap window
            if (spirit.double_tap_timer > 0) {
                spirit.double_tap_timer = 0;
                spirit.button_a_hold_timer = 0;
                cmd = 2; // Command 2: Double Tap (ROM 0x011B08)
            }
        }
    } else {
        // Button A is not held (ROM 0x011AD8..0x011B02)
        if (spirit.prev_button_a) {
            // Button was just released!
            if (spirit.button_a_hold_timer >= kSpiritHoldThreshold) {
                spirit.button_a_hold_timer = 0;
                spirit.double_tap_timer = 0;
                cmd = 3; // Command 3: Hold release (ROM 0x011B06)
            } else {
                // Released before 20 frames: start 6-frame double tap window (ROM 0x011AFA)
                spirit.button_a_hold_timer = 0;
                spirit.double_tap_timer = kSpiritDoubleTapWindow;
            }
        }
    }

    spirit.prev_button_a = button_a_pressed;
    return cmd;
}

bool try_execute_dytto_command(
    ActiveSpirit& spirit,
    std::uint8_t command_code,
    player::PlayerState& player,
    std::span<const std::uint8_t> rom) noexcept {
    if (!spirit.active || command_code == 0) return false;
    // Cannot start a new command while already casting (ROM 0x0121B0)
    if (spirit.state == kSpiritStateBubbleCast) return false;

    spirit.last_command_code = command_code;

    // Command 1: Water Bubble (ROM 0x011D8A..0x011DAE)
    if (command_code == 1) {
        // SP Cost check & deduction: 5 SP (ROM 0x011B28 & 0x011D8A)
        if (player.sp < kSpiritBubbleSpCost) {
            return false; // RESOURCE_GATE
        }
        player.sp -= kSpiritBubbleSpCost;
        spirit.last_command_sp_cost = kSpiritBubbleSpCost;

        // Transition spirit to State 14 (Bubble Cast), anim 6 (ROM 0x011DA8..0x011DAE)
        spirit.state = kSpiritStateBubbleCast;
        spirit.anim_selector = kSpiritAnimBubbleCast;
        spirit.command_state = kSpiritStateBubbleCast;
        spirit.command_timer = 24; // cast startup duration
        spirit.command_direction = player.direction_code;
        spirit.command_active = true;
        spirit.target_entity_hit = 0;
        return true;
    }

    return false;
}

void update_dytto_ability(
    ActiveSpirit& spirit,
    std::span<const std::uint8_t> rom,
    std::vector<entities::ScreenEntity>* entities) noexcept {
    if (!spirit.active) {
        cleanup_spirit_ability(spirit);
        return;
    }

    // 1. Dytto Cast State (State 14: ROM 0x011DB6..0x011ED0)
    if (spirit.state == kSpiritStateBubbleCast) {
        if (spirit.command_timer > 0) {
            --spirit.command_timer;
        }
        if (spirit.command_timer == 0) {
            // Spawn Water Bubble projectile at frame 24 (ROM 0x011E04..0x011EAC)
            spirit.projectile_active = true;
            spirit.projectile_state = kSpiritStateBubbleFly;
            spirit.command_state = kSpiritStateBubbleFly;
            spirit.projectile_x = spirit.world_x;
            spirit.projectile_y = spirit.world_y;
            spirit.projectile_z = spirit.world_z + 10; // elevated 10 px (ROM 0x011E72)
            spirit.projectile_lifetime = kSpiritBubbleLifetime;

            // Velocity along command direction (4 px/frame cardinal) (ROM 0x011E7C..0x011EAC)
            spirit.projectile_vx = (spirit.command_direction == 1) ? kSpiritBubbleSpeed :
                                   ((spirit.command_direction == 3) ? -kSpiritBubbleSpeed : 0);
            spirit.projectile_vy = (spirit.command_direction == 2) ? kSpiritBubbleSpeed :
                                   ((spirit.command_direction == 0) ? -kSpiritBubbleSpeed : 0);

            // Decode bubble piece from ROM 0x141412 (1 piece, 2x2 cells, tile 248, pal 3)
            if (!rom.empty()) {
                spirit.projectile_pieces = entities::decode_sprite_frame_from_rom(rom, kSpiritBubbleFrame0);
                if (!spirit.projectile_pieces.empty()) {
                    spirit.projectile_tile_base = spirit.projectile_pieces.front().tile_index;
                }
            }

            // Dytto returns to Follow/Idle State 2 with hover anim (ROM 0x011DE4)
            spirit.state = kSpiritStateFollow;
            spirit.anim_selector = kSpiritAnimHover;
            return;
        }
    }

    // 2. Water Bubble Projectile Update (State 16 & State 22: ROM 0x011ED2..0x011FA4)
    if (spirit.projectile_active) {
        if (spirit.projectile_state == kSpiritStateBubbleFly) {
            spirit.projectile_x += spirit.projectile_vx;
            spirit.projectile_y += spirit.projectile_vy;

            if (spirit.projectile_lifetime > 0) {
                --spirit.projectile_lifetime;
            }

            // Entity collision check (ROM 0x011EE6..0x011F46: box [-9, 9] x [-9, 9])
            bool hit = false;
            if (entities) {
                const entities::EntityHitbox bubble_box{
                    spirit.projectile_x - 9, spirit.projectile_y - 9,
                    spirit.projectile_x + 9, spirit.projectile_y + 9,
                    spirit.projectile_z - 8, spirit.projectile_z + 8
                };
                for (auto& ent : *entities) {
                    if (!ent.active()) continue;
                    if (ent.is_flame() && ent.flame_substate() == 0) {
                        if (bubble_box.overlaps(ent.hitbox())) {
                            ent.extinguish();
                            spirit.target_entity_hit = ent.type_id();
                            hit = true;
                            break;
                        }
                    } else if (ent.is_actor()) {
                        if (bubble_box.overlaps(ent.hitbox())) {
                            ent.apply_damage(5, spirit.command_direction);
                            spirit.target_entity_hit = ent.type_id();
                            hit = true;
                            break;
                        }
                    }
                }
            }

            if (hit || spirit.projectile_lifetime == 0) {
                // Transition to State 22 (Bubble Pop: ROM 0x011F8A..0x011F90)
                spirit.projectile_state = kSpiritStateBubblePop;
                spirit.command_state = kSpiritStateBubblePop;
                spirit.command_timer = 4; // pop animation duration
            }
        } else if (spirit.projectile_state == kSpiritStateBubblePop) {
            if (spirit.command_timer > 0) {
                --spirit.command_timer;
            }
            if (spirit.command_timer == 0) {
                // Bubble removed (ROM 0x011FA0: clr.w $0(a6))
                spirit.projectile_active = false;
                spirit.command_state = 0;
                spirit.command_active = false;
                spirit.projectile_pieces.clear();
            }
        }
    }
}

void cleanup_spirit_ability(ActiveSpirit& spirit) noexcept {
    spirit.command_state = 0;
    spirit.command_timer = 0;
    spirit.command_active = false;
    spirit.projectile_active = false;
    spirit.projectile_state = 0;
    spirit.projectile_pieces.clear();
    spirit.button_a_hold_timer = 0;
    spirit.double_tap_timer = 0;
    spirit.prev_button_a = false;
    if (spirit.active && spirit.state == kSpiritStateBubbleCast) {
        spirit.state = kSpiritStateFollow;
        spirit.anim_selector = kSpiritAnimHover;
    }
}

void reposition_spirit_on_screen_transition(
    ActiveSpirit& spirit,
    std::int32_t px,
    std::int32_t py,
    std::int32_t pz) noexcept {
    if (!spirit.active) {
        return;
    }
    // ROM 0x03A464..0x03A47E: teleports spirit coordinates to player destination coordinates
    cleanup_spirit_ability(spirit);
    spirit.world_x = px;
    spirit.world_y = py;
    spirit.world_z = pz + 16;
    spirit.target_x = px;
    spirit.target_y = py;
    spirit.state = kSpiritStateFollow;
    spirit.anim_selector = kSpiritAnimHover;
}

void despawn_spirit(ActiveSpirit& spirit) noexcept {
    cleanup_spirit_ability(spirit);
    spirit.active = false;
    spirit.state = kSpiritStateDespawn;
    spirit.pieces.clear();
}

} // namespace oasis::game::spirits
