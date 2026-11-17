#include "game/player/ali_jump_pickup_probe.hpp"

#include <array>
#include <cassert>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(AliJumpPickupConfig::kJumpTriggerAddr == 0x00822CU);
    static_assert(AliJumpPickupConfig::kPickupProbeAddr == 0x0082AEU);
    static_assert(AliJumpPickupConfig::kInteractProbeAddr == 0x0082F8U);

    static_assert(AliJumpPickupConfig::kCutsceneLockRam == 0x00FF1985U);
    static_assert(AliJumpPickupConfig::kRunningJumpFlagRam == 0x00FF197EU);
    static_assert(AliJumpPickupConfig::kStandingJumpFlagRam == 0x00FF1980U);
    static_assert(AliJumpPickupConfig::kInputRingCountRam == 0x00FF16EAU);
    static_assert(AliJumpPickupConfig::kCollectedItemRam == 0x00FF1976U);
    static_assert(AliJumpPickupConfig::kCollectedSlotRam == 0x00FF1978U);
    static_assert(AliJumpPickupConfig::kCollectedPayloadRam == 0x00FF197AU);

    static_assert(AliJumpPickupConfig::kJumpZVel16_16 == 0x00070000);
    static_assert(AliJumpPickupConfig::kRunningJumpState == 22U);
    static_assert(AliJumpPickupConfig::kStandingJumpState == 16U);
    static_assert(AliJumpPickupConfig::kRunningSubAction == 4U);

    static_assert(AliJumpPickupConfig::kPickupProbeRadiusPx == 6);
    static_assert(AliJumpPickupConfig::kPickupEntityType == 8U);
}

static void test_interact_boxes() {
    // Facing 0: Up [-8, -30, 8, -4]
    const auto box_up = get_interact_box(0U);
    assert(box_up.x1 == -8 && box_up.y1 == -30 && box_up.x2 == 8 && box_up.y2 == -4);
    assert(is_in_interact_box(0U, 0, -10));
    assert(is_in_interact_box(0U, -8, -4));
    assert(is_in_interact_box(0U, 8, -30));
    assert(!is_in_interact_box(0U, 0, 0));   // Behind/inside
    assert(!is_in_interact_box(0U, 10, -10)); // Outside X

    // Facing 1: Right [4, -8, 32, 8]
    const auto box_right = get_interact_box(1U);
    assert(box_right.x1 == 4 && box_right.y1 == -8 && box_right.x2 == 32 && box_right.y2 == 8);
    assert(is_in_interact_box(1U, 10, 0));
    assert(is_in_interact_box(1U, 4, -8));
    assert(is_in_interact_box(1U, 32, 8));
    assert(!is_in_interact_box(1U, 0, 0));
    assert(!is_in_interact_box(1U, 10, 10));

    // Facing 2: Down [-8, 4, 8, 30]
    const auto box_down = get_interact_box(2U);
    assert(box_down.x1 == -8 && box_down.y1 == 4 && box_down.x2 == 8 && box_down.y2 == 30);
    assert(is_in_interact_box(2U, 0, 10));
    assert(is_in_interact_box(2U, -8, 4));
    assert(is_in_interact_box(2U, 8, 30));
    assert(!is_in_interact_box(2U, 0, 0));
    assert(!is_in_interact_box(2U, -10, 10));

    // Facing 3: Left [-32, -8, -4, 8]
    const auto box_left = get_interact_box(3U);
    assert(box_left.x1 == -32 && box_left.y1 == -8 && box_left.x2 == -4 && box_left.y2 == 8);
    assert(is_in_interact_box(3U, -10, 0));
    assert(is_in_interact_box(3U, -32, -8));
    assert(is_in_interact_box(3U, -4, 8));
    assert(!is_in_interact_box(3U, 0, 0));
    assert(!is_in_interact_box(3U, -10, -10));
}

static void test_jump_trigger() {
    AliJumpTriggerContext ctx{};
    AliJumpEntityState ali{};

    // 1. Cutscene locked -> returns None
    ctx.cutscene_locked = true;
    ali.sub_action = 4U;
    ctx.dpad_held = true;
    auto res_locked = trigger_ali_jump(ctx, ali);
    assert(!res_locked.triggered);
    assert(res_locked.jump_type == AliJumpType::None);
    ctx.cutscene_locked = false;

    // 2. Running jump: dpad_held && sub_action == 4
    ali.sub_action = 4U;
    ali.state = 4U;
    ali.vel_x_16_16 = 0x00030000; // +3.0 px/frame
    ali.vel_y_16_16 = 0x00020000; // +2.0 px/frame
    ali.vel_z_16_16 = 0;
    ctx.dpad_held = true;
    ctx.input_ring_count = 5U;
    ctx.running_jump_flag = 0U;

    auto res_run = trigger_ali_jump(ctx, ali);
    assert(res_run.triggered);
    assert(res_run.jump_type == AliJumpType::RunningJump);
    assert(ali.state == 22U);
    assert(ali.sub_action == 22U);
    assert(ali.prev_sub_action == 4U);
    assert(ali.vel_z_16_16 == 0x00070000);
    assert(ali.vel_x_16_16 == 0x00030000); // Preserved
    assert(ali.vel_y_16_16 == 0x00020000); // Preserved
    assert(ctx.running_jump_flag == 0xFFU);
    assert(ctx.input_ring_count == 0U);

    // 3. Standing jump: dpad_held == false
    ali.sub_action = 4U;
    ali.vel_x_16_16 = 0x00030000;
    ali.vel_y_16_16 = 0x00020000;
    ctx.dpad_held = false;
    ctx.input_ring_count = 12U;
    ctx.standing_jump_flag = 0x1234U;

    auto res_stand1 = trigger_ali_jump(ctx, ali);
    assert(res_stand1.triggered);
    assert(res_stand1.jump_type == AliJumpType::StandingJump);
    assert(ali.state == 16U);
    assert(ali.sub_action == 16U);
    assert(ali.prev_sub_action == 4U);
    assert(ali.vel_z_16_16 == 0x00070000);
    assert(ali.vel_x_16_16 == 0); // Cleared to 0
    assert(ali.vel_y_16_16 == 0); // Cleared to 0
    assert(ctx.standing_jump_flag == 0U);
    assert(ctx.input_ring_count == 0U);

    // 4. Standing jump: sub_action != 4 (e.g. idle sub_action 0, but dpad_held)
    ali.sub_action = 0U;
    ali.vel_x_16_16 = 0x00010000;
    ctx.dpad_held = true;

    auto res_stand2 = trigger_ali_jump(ctx, ali);
    assert(res_stand2.triggered);
    assert(res_stand2.jump_type == AliJumpType::StandingJump);
    assert(ali.state == 16U);
    assert(ali.vel_x_16_16 == 0);
}

static void test_pickup_probe() {
    std::vector<PickupCandidateEntity> entities(3);

    // Entity 0: type 8 (pickup), dx = +4px, dy = -5px -> within ±6px
    entities[0] = {
        .entity_type = 8U,
        .sub_action = 2U,
        .pos_x_16_16 = (100 + 4) << 16,
        .pos_y_16_16 = (200 - 5) << 16,
        .item_id = 0x0012U,
        .world_payload = 0x12345678,
        .quantity = 3U
    };

    // Entity 1: type 8 (pickup), dx = +8px -> outside ±6px
    entities[1] = {
        .entity_type = 8U,
        .sub_action = 1U,
        .pos_x_16_16 = (100 + 8) << 16,
        .pos_y_16_16 = 200 << 16,
        .item_id = 0x0005U,
        .world_payload = 0,
        .quantity = 1U
    };

    // Entity 2: type 0x14 (enemy), inside ±6px
    entities[2] = {
        .entity_type = 0x14U,
        .sub_action = 0U,
        .pos_x_16_16 = 100 << 16,
        .pos_y_16_16 = 200 << 16,
        .item_id = 0U,
        .world_payload = 0,
        .quantity = 0U
    };

    const std::int32_t ali_x = 100 << 16;
    const std::int32_t ali_y = 200 << 16;

    auto res = probe_pickup_entities(ali_x, ali_y, entities);
    assert(res.collected);
    assert(res.entity_index == 0U);
    assert(res.collected_item_word == 0x1203U); // (0x12 << 8) | 0x03
    assert(res.collected_sub_action == 2U);
    assert(res.collected_payload == 0x12345678);
    // Verified entity 0 is despawned
    assert(entities[0].entity_type == 0U);

    // Second probe: entity 0 is now 0 (despawned), entity 1 is too far, entity 2 is not type 8
    auto res2 = probe_pickup_entities(ali_x, ali_y, entities);
    assert(!res2.collected);
    assert(entities[1].entity_type == 8U);
}

int main() {
    test_rom_constants();
    test_interact_boxes();
    test_jump_trigger();
    test_pickup_probe();
    return 0;
}
