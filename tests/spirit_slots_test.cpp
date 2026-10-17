#include "game/spirits/spirit_slots.hpp"
#include "game/player/player.hpp"

#include <array>
#include <cassert>
#include <span>
#include <vector>

namespace {

void write_word(std::span<std::uint8_t> bytes,
                std::size_t offset,
                std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

} // namespace

int main() {
    using namespace oasis::game::spirits;
    using oasis::game::entities::EntityPoolView;
    using oasis::game::entities::kEntityPoolAtFf19e8;
    using oasis::game::entities::kEntityPoolAtFf2d8c;

    SpiritSlots slots;
    slots.apply_event(0x15);
    assert(slots.acquired_mask() == 0);
    slots.apply_event(0x16);
    slots.apply_event(0x19);
    assert(slots.acquired(0));
    assert(!slots.acquired(1));
    assert(!slots.acquired(2));
    assert(slots.acquired(3));

    const auto active = trace_observed_dispatch(
        {.input_40 = 0, .input_41 = 0x0A, .slot_flags = 0x02});
    assert(active.entered_observed_path);
    assert(active.slot_available);
    assert(active.selector_count == 2);
    assert(active.selectors[0] == kObservedResourceSelector);
    assert(active.selectors[1] == kObservedEffectSelector);
    assert((active.guard_flags & 0x01U) != 0);

    const auto guarded = trace_observed_dispatch(
        {.input_41 = 0x0A, .slot_flags = 0x02, .guard_flags = 0x01});
    assert(guarded.selector_count == 1);
    assert(guarded.selectors[0] == kObservedEffectSelector);

    const auto unavailable = trace_observed_dispatch(
        {.input_41 = 0x0A, .slot_flags = 0x01});
    assert(unavailable.entered_observed_path);
    assert(!unavailable.slot_available);
    assert(unavailable.selector_count == 1);
    assert(unavailable.selectors[0] == kObservedEffectSelector);

    const auto unrelated_input = trace_observed_dispatch(
        {.input_41 = 0x08, .slot_flags = 0x02});
    assert(!unrelated_input.entered_observed_path);
    assert(unrelated_input.selector_count == 0);

    const auto summon = initialize_observed_summon({
        .state_30 = 0x18,
        .flags_37 = 0,
        .position_x = 0x0123,
        .position_y = 0x0456,
        .raw_17 = 0x07,
        .raw_14 = 0x0011,
    });
    assert(summon);
    assert(summon->raw_type == kObservedSummonType);
    assert(summon->position_x == 0x0123);
    assert(summon->position_y == 0x0456);
    assert(summon->raw_depth == kObservedSummonDepth);
    assert(summon->raw_66 == 0x07);
    assert(summon->raw_14 == 0x0011);
    assert(summon->raw_resource_18 == kObservedSummonResource);
    assert(summon->raw_resource_5a == kObservedSummonResource);
    assert(summon->raw_a6 == 0);
    assert(summon->raw_aa == 0);
    assert(!initialize_observed_summon({.state_30 = 0x17}));
    assert(!initialize_observed_summon({.state_30 = 0x18, .flags_37 = 0x02}));

    std::array<std::uint8_t, 6U * 0x5AU> owner_storage{};
    write_word(owner_storage, 0x00, 0x2E);
    write_word(owner_storage, 0x08, 100);
    write_word(owner_storage, 0x0C, 100);
    write_word(owner_storage, 0x10, 0);
    write_word(owner_storage, 0x14, 0);
    std::array<std::uint8_t, 21U * 0xBCU> target_storage{};
    const auto target_offset = 0x00U;
    write_word(target_storage, target_offset + 0x00, 0x16);
    write_word(target_storage, target_offset + 0x08, 100);
    write_word(target_storage, target_offset + 0x0C, 100);
    write_word(target_storage, target_offset + 0x10, 0);
    write_word(target_storage, target_offset + 0x14, 0);
    write_word(target_storage, target_offset + 0x42, 10);
    write_word(target_storage, target_offset + 0x44, 6);
    write_word(target_storage, target_offset + 0x4A, 8);
    const EntityPoolView owner_pool(owner_storage, kEntityPoolAtFf2d8c);
    const EntityPoolView target_pool(target_storage, kEntityPoolAtFf19e8);
    assert(find_observed_target(owner_pool, target_pool, {.owner_index = 0}) == 0);

    write_word(target_storage, target_offset + 0x00, 0x17);
    assert(!find_observed_target(owner_pool, target_pool, {.owner_index = 0}));

    // -------------------------------------------------------------
    // Test 1: SUMMON_INPUT
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16); // Slot 0 (Dytto) acquired
        assert(s.acquired(0));
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        const bool summoned = try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {});
        assert(summoned);
        assert(sp.active);
    }

    // -------------------------------------------------------------
    // Test 2: RESOURCE_GATE
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 0; // Out of SP
        const bool summoned = try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {});
        assert(!summoned); // Gate closed
        assert(!sp.active);
    }

    // -------------------------------------------------------------
    // Test 3: SPIRIT_SLOT_ALLOCATED
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 50;
        try_summon_spirit(sp, s, 0, 100, 100, 0, pl, {});
        assert(sp.active);
        assert(sp.slot == 0);
        assert(sp.type_id == kSpiritTypeDytto);
    }

    // -------------------------------------------------------------
    // Test 4: SPIRIT_ENTITY_SPAWNED
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_summon_spirit(sp, s, 0, 140, 180, 5, pl, {});
        assert(sp.world_x == 140);
        assert(sp.world_y == 180);
        assert(sp.world_z == 5 + 16); // Z+16 elevation offset (ROM 0x018D08)
        assert(sp.direction == 2);    // Initial direction Down (ROM 0x014674)
    }

    // -------------------------------------------------------------
    // Test 5: SPIRIT_GRAPHICS_VALID
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        // Mock ROM buffer containing frame descriptor at 0x11F506 (3 pieces)
        std::vector<std::uint8_t> mock_rom(0x11F600, 0);
        // Frame 0 at 0x11F506: piece count = 3
        mock_rom[0x11F506] = 0x00; mock_rom[0x11F507] = 0x03;
        // Piece 0: size=2x3 (raw 0x09), y=-24 (raw 0xE8), x=7, tile=773, pal=3
        mock_rom[0x11F508] = 0x09; mock_rom[0x11F509] = 0xE8;
        mock_rom[0x11F50A] = 0x00; mock_rom[0x11F50B] = 0x07;
        mock_rom[0x11F50C] = 0x63; mock_rom[0x11F50D] = 0x05;
        try_summon_spirit(sp, s, 0, 160, 120, 0, pl, mock_rom);
        assert(!sp.pieces.empty());
        assert(sp.tile_base == 773);
        assert(sp.pieces.front().palette_line == 3);
    }

    // -------------------------------------------------------------
    // Test 6: INITIAL_STATE
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {});
        assert(sp.state == kSpiritStateAppear); // State 26
        assert(sp.anim_selector == kSpiritAnimAppear); // Anim 34
        assert(sp.state_timer == 20);
    }

    // -------------------------------------------------------------
    // Test 7: FOLLOW_IDLE_MOVEMENT
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        pl.x_fixed = 200 * 0x10000;
        pl.y_fixed = 200 * 0x10000;
        try_summon_spirit(sp, s, 0, 100, 100, 0, pl, {});
        // Advance through appear sequence (20 ticks)
        for (int i = 0; i < 20; ++i) {
            update_active_spirit(sp, pl, pl);
        }
        assert(sp.state == kSpiritStateFollow); // State 2
        assert(sp.anim_selector == kSpiritAnimHover); // Anim 4
        // In State 2, spirit steps towards player (target 200, 184)
        const auto init_x = sp.world_x;
        const auto init_y = sp.world_y;
        for (int i = 0; i < 10; ++i) {
            update_active_spirit(sp, pl, pl);
        }
        assert(sp.world_x > init_x); // Moved rightwards towards player
        assert(sp.world_y > init_y); // Moved downwards towards player
    }

    // -------------------------------------------------------------
    // Test 8: NO_DUPLICATE_SLOT
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        assert(try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {}));
        assert(sp.active);
        // Second summon attempt rejected while slot is occupied
        assert(!try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {}));
    }

    // -------------------------------------------------------------
    // Test 9: SCREEN_TRANSITION_BEHAVIOR
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_summon_spirit(sp, s, 0, 100, 100, 0, pl, {});
        // Screen transition teleports spirit to player destination
        reposition_spirit_on_screen_transition(sp, 250, 300, 0);
        assert(sp.active);
        assert(sp.world_x == 250);
        assert(sp.world_y == 300);
        assert(sp.world_z == 16);
        assert(sp.target_x == 250);
        assert(sp.target_y == 300);
    }

    // -------------------------------------------------------------
    // Test 10: DESPAWN_OR_FAILURE_PATH
    // -------------------------------------------------------------
    {
        SpiritSlots s;
        s.apply_event(0x16);
        ActiveSpirit sp{};
        oasis::game::player::PlayerState pl{};
        pl.sp = 1; // Exactly 1 SP remaining
        try_summon_spirit(sp, s, 0, 160, 120, 0, pl, {});
        assert(sp.active);
        // Advance 45 frames (kSpiritSpCostInterval)
        for (int i = 0; i < 45; ++i) {
            update_active_spirit(sp, pl, pl);
        }
        assert(pl.sp == 0); // SP consumed
        assert(!sp.active); // Despawned on resource exhaustion
        assert(sp.state == kSpiritStateDespawn);
    }

    // -------------------------------------------------------------
    // Test 11: COMMAND_INPUT_DETECTED (ROM 0x011A4C..0x011B26)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        // Press Button::A for 3 frames (< 20 frame threshold)
        assert(decode_dytto_command_input(sp, true) == 0);
        assert(decode_dytto_command_input(sp, true) == 0);
        assert(decode_dytto_command_input(sp, true) == 0);
        // Release Button::A: starts 6-frame double tap window (ROM 0x011AFA)
        assert(decode_dytto_command_input(sp, false) == 0);
        assert(sp.double_tap_timer == 6);
        // Advance 5 frames (timer 5..1): still waiting
        for (int i = 0; i < 5; ++i) {
            assert(decode_dytto_command_input(sp, false) == 0);
        }
        // Frame 6: window expires, produces Command 1 (Single Tap) (ROM 0x011B0A)
        assert(decode_dytto_command_input(sp, false) == 1);
    }

    // -------------------------------------------------------------
    // Test 12: COMMAND_REJECTED_WITHOUT_SP (ROM 0x011B28 & 0x011D90)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        sp.state = kSpiritStateFollow;
        oasis::game::player::PlayerState pl{};
        pl.sp = 4; // Less than 5 SP cost
        const bool executed = try_execute_dytto_command(sp, 1, pl, {});
        assert(!executed);
        assert(pl.sp == 4); // SP not deducted
        assert(sp.state == kSpiritStateFollow); // Remains in follow
    }

    // -------------------------------------------------------------
    // Test 13: STATE_TRANSITION (ROM 0x011DA8)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        sp.state = kSpiritStateFollow;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        pl.direction_code = 1; // Facing Right
        const bool executed = try_execute_dytto_command(sp, 1, pl, {});
        assert(executed);
        assert(sp.state == kSpiritStateBubbleCast); // State 14
        assert(sp.command_state == kSpiritStateBubbleCast);
        assert(sp.command_timer == 24);
        assert(sp.command_direction == 1);
        assert(sp.command_active);
    }

    // -------------------------------------------------------------
    // Test 14: ABILITY_ENTITY_SPAWNED (ROM 0x011E04..0x011EAC)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        sp.world_x = 100;
        sp.world_y = 120;
        sp.world_z = 16;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        pl.direction_code = 2; // Facing Down
        try_execute_dytto_command(sp, 1, pl, {});
        // Advance 24 frames through cast duration
        for (int i = 0; i < 24; ++i) {
            update_dytto_ability(sp, {}, nullptr);
        }
        assert(sp.projectile_active);
        assert(sp.projectile_state == kSpiritStateBubbleFly); // State 16
        assert(sp.projectile_x == 100);
        assert(sp.projectile_y == 120);
        assert(sp.projectile_z == 26); // elevated +10 px (ROM 0x011E72)
        assert(sp.projectile_vx == 0);
        assert(sp.projectile_vy == 4); // 4 px/frame downward
        assert(sp.projectile_lifetime == 24);
    }

    // -------------------------------------------------------------
    // Test 15: ROM_ANIMATION_SELECTED (ROM 0x011DAE & 0x011E5A)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_execute_dytto_command(sp, 1, pl, {});
        assert(sp.anim_selector == kSpiritAnimBubbleCast); // Anim 6
        for (int i = 0; i < 24; ++i) {
            update_dytto_ability(sp, {}, nullptr);
        }
        // After cast, Dytto returns to hover animation
        assert(sp.anim_selector == kSpiritAnimHover); // Anim 4
    }

    // -------------------------------------------------------------
    // Test 16: SP_COST_APPLIED (ROM 0x011D8A)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        oasis::game::player::PlayerState pl{};
        pl.sp = 50;
        try_execute_dytto_command(sp, 1, pl, {});
        assert(pl.sp == 45); // Exactly 5 SP deducted
        assert(sp.last_command_sp_cost == 5);
    }

    // -------------------------------------------------------------
    // Test 17: ABILITY_COMPLETES (ROM 0x011ED2..0x011FA4)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        sp.world_x = 100;
        sp.world_y = 100;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        pl.direction_code = 1; // Right
        try_execute_dytto_command(sp, 1, pl, {});
        for (int i = 0; i < 24; ++i) update_dytto_ability(sp, {}, nullptr);
        assert(sp.projectile_active);
        const auto start_x = sp.projectile_x;
        // Fly 10 frames
        for (int i = 0; i < 10; ++i) update_dytto_ability(sp, {}, nullptr);
        assert(sp.projectile_x == start_x + 40); // 4 px/frame * 10
        // Exhaust remaining lifetime (14 frames)
        for (int i = 0; i < 14; ++i) update_dytto_ability(sp, {}, nullptr);
        // Enters Pop state (State 22)
        assert(sp.projectile_state == kSpiritStateBubblePop);
        // Advance pop timer (4 frames)
        for (int i = 0; i < 4; ++i) update_dytto_ability(sp, {}, nullptr);
        assert(!sp.projectile_active); // Cleared after pop
    }

    // -------------------------------------------------------------
    // Test 18: RETURN_TO_FOLLOW (ROM 0x011DE4)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_execute_dytto_command(sp, 1, pl, {});
        assert(sp.state == kSpiritStateBubbleCast);
        for (int i = 0; i < 24; ++i) update_dytto_ability(sp, {}, nullptr);
        assert(sp.state == kSpiritStateFollow); // Resumes follow
        assert(sp.anim_selector == kSpiritAnimHover);
    }

    // -------------------------------------------------------------
    // Test 19: NO_DUPLICATE_EFFECT (ROM 0x0121B0)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        assert(try_execute_dytto_command(sp, 1, pl, {}));
        // Cannot execute another command while casting
        assert(!try_execute_dytto_command(sp, 1, pl, {}));
        assert(pl.sp == 95); // SP not deducted a second time
    }

    // -------------------------------------------------------------
    // Test 20: SCREEN_TRANSITION_SAFE (ROM 0x03A464)
    // -------------------------------------------------------------
    {
        ActiveSpirit sp{};
        sp.active = true;
        oasis::game::player::PlayerState pl{};
        pl.sp = 100;
        try_execute_dytto_command(sp, 1, pl, {});
        for (int i = 0; i < 24; ++i) update_dytto_ability(sp, {}, nullptr);
        assert(sp.projectile_active);
        // Room transition occurs while projectile is flying
        reposition_spirit_on_screen_transition(sp, 200, 200, 0);
        assert(!sp.projectile_active); // Projectile cleaned up
        assert(sp.state == kSpiritStateFollow); // Spirit safely in follow
        assert(sp.world_x == 200);
        assert(sp.world_y == 200);
    }

    return 0;
}
