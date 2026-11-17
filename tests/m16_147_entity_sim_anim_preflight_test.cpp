#include "game/player/entity_sim_anim_preflight.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(EntitySimConfig::kFreezeSlowmoAddr == 0x008F22U);
    assert(EntitySimConfig::kFreezeSlowmoEndAddr == 0x008F84U);
    assert(EntitySimConfig::kAnimFrameStepperAddr == 0x008F84U);
    assert(EntitySimConfig::kAnimFrameStepperEndAddr == 0x009040U);
    assert(EntitySimConfig::kTerrainPreflightAddr == 0x009040U);
    assert(EntitySimConfig::kTerrainPreflightEndAddr == 0x009084U);

    assert(EntitySimConfig::kGlobalFlagsRam == 0x00FF17B8U);
    assert(EntitySimConfig::kGlobalSlowmoBit == 0U);
    assert(EntitySimConfig::kGlobalFreezeBit == 1U);

    assert(EntitySimConfig::kLayerSelectRam == 0x00FF16F0U);
    assert(EntitySimConfig::kLayerSelectBit == 7U);
    assert(EntitySimConfig::kLayer1TilemapRam == 0x00FF1766U);
    assert(EntitySimConfig::kLayer2TilemapRam == 0x00FF1770U);

    assert(EntitySimConfig::kTypeOffset == 0x00U);
    assert(EntitySimConfig::kPosXOffset == 0x08U);
    assert(EntitySimConfig::kAnimTablePtrOffset == 0x26U);
    assert(EntitySimConfig::kStateOffset == 0x2AU);
    assert(EntitySimConfig::kPrevStateOffset == 0x2CU);
    assert(EntitySimConfig::kKeyframeTimerOffset == 0x2EU);
    assert(EntitySimConfig::kFrameOffsetOffset == 0x30U);
    assert(EntitySimConfig::kSpriteAssetOffset == 0x32U);
    assert(EntitySimConfig::kFlags37Offset == 0x37U);
    assert(EntitySimConfig::kFlags38Offset == 0x38U);
    assert(EntitySimConfig::kCollision40Offset == 0x40U);
    assert(EntitySimConfig::kCollision41Offset == 0x41U);
    assert(EntitySimConfig::kVelXOffset == 0x72U);
    assert(EntitySimConfig::kVelYOffset == 0x76U);
    assert(EntitySimConfig::kSubtimer9cOffset == 0x9CU);
    assert(EntitySimConfig::kSubtimer9dOffset == 0x9DU);

    assert(EntitySimConfig::kFlagAnimLockBit == 0U);
    assert(EntitySimConfig::kFlagLoopEndTriggerBit == 1U);
    assert(EntitySimConfig::kFlagLoopMarkBit == 2U);
    assert(EntitySimConfig::kFlagIntangibleBit == 4U);
    assert(EntitySimConfig::kFlagFlyingBit == 1U);

    assert(EntitySimConfig::kMoveCodePosX == 0x11U);
    assert(EntitySimConfig::kBypassTarget == 0x009382U);
    assert(EntitySimConfig::kBranchFlying == 0x009708U);
    assert(EntitySimConfig::kBranchSweepY == 0x0091F6U);
    assert(EntitySimConfig::kBranchSweepNegX == 0x009128U);
}

static void test_entity_freeze_slowmo() {
    std::array<std::uint8_t, 188> slot{};
    slot.fill(0U);

    auto write_u16 = [](std::span<std::uint8_t> s, std::size_t off, std::uint16_t v) {
        s[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
        s[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
    };
    auto write_i32 = [](std::span<std::uint8_t> s, std::size_t off, std::int32_t v) {
        auto u = static_cast<std::uint32_t>(v);
        s[off] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        s[off + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        s[off + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        s[off + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    };

    // 1. Subtimer 9D freeze
    write_u16(slot, 0x00, 4U); // enemy
    slot[0x9D] = 3U;
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);

    auto res = step_entity_freeze_slowmo(slot, false, false);
    assert(res.frozen);
    assert(res.skip_anim);
    assert(res.vel_x == 0 && res.vel_y == 0);
    assert(slot[0x9D] == 2U);

    // 2. Subtimer 9C decrement & local slow-motion halving
    slot[0x9D] = 0U;
    slot[0x9C] = 4U;
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);

    res = step_entity_freeze_slowmo(slot, false, false);
    assert(!res.frozen);
    assert(slot[0x9C] == 3U);
    assert(res.vel_x == 0x00010000);
    assert(res.vel_y == 0x00008000);

    // 3. Global freeze: enemy frozen vs player preserved
    slot[0x9C] = 0U;
    slot[0x9D] = 0U;

    // Enemy (type 4)
    write_u16(slot, 0x00, 4U);
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);
    res = step_entity_freeze_slowmo(slot, true, false);
    assert(res.frozen);
    assert(res.vel_x == 0 && res.vel_y == 0);

    // Player (type 2)
    write_u16(slot, 0x00, 2U);
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);
    res = step_entity_freeze_slowmo(slot, true, false);
    assert(!res.frozen);
    assert(res.vel_x == 0x00020000);
    assert(res.vel_y == 0x00010000);

    // 4. Global slow-motion: enemy halved vs player normal
    // Enemy (type 4)
    write_u16(slot, 0x00, 4U);
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);
    res = step_entity_freeze_slowmo(slot, false, true);
    assert(!res.frozen);
    assert(res.vel_x == 0x00010000);
    assert(res.vel_y == 0x00008000);

    // Player (type 2)
    write_u16(slot, 0x00, 2U);
    write_i32(slot, 0x72, 0x00020000);
    write_i32(slot, 0x76, 0x00010000);
    res = step_entity_freeze_slowmo(slot, false, true);
    assert(!res.frozen);
    assert(res.vel_x == 0x00020000);
    assert(res.vel_y == 0x00010000);

    // 5. Anim lock bit 0 on flags_37
    slot[0x37] = 0x01U;
    res = step_entity_freeze_slowmo(slot, false, false);
    assert(res.skip_anim);
}

static void test_entity_animation_stepper() {
    std::array<std::uint8_t, 90> slot{};
    slot.fill(0U);

    auto write_u16 = [](std::span<std::uint8_t> s, std::size_t off, std::uint16_t v) {
        s[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
        s[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
    };
    auto read_u16 = [](std::span<const std::uint8_t> s, std::size_t off) -> std::uint16_t {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(s[off]) << 8U) | s[off + 1U]);
    };

    // 2-frame animation:
    // Frame 0: sprite 0x0123, duration 8 (dur>>1 = 4, timer = 3)
    // Frame 1: sprite 0x0456, duration 8 | 0x8000 (loop marker, timer = 3)
    std::vector<AnimKeyframe> kfs = {
        {0x0123U, 8U},
        {0x0456U, static_cast<std::uint16_t>(8U | 0x8000U)},
    };
    auto anim_data = pack_anim_sequence(kfs);

    // Set state transition: cur_state = 2, prev_state = 0
    write_u16(slot, 0x2A, 2U);
    write_u16(slot, 0x2C, 0U);

    // Step 1: Transition initializes frame_offset = -4, timer = 0 -> advances to frame 0
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2C) == 2U); // prev_state updated
    assert(read_u16(slot, 0x32) == 0x0123U); // sprite 0
    assert(read_u16(slot, 0x30) == 0U); // frame offset 0
    assert(read_u16(slot, 0x2E) == 3U); // timer = (8>>1)-1 = 3
    assert((slot[0x37] & 0x02U) == 0U); // loop trigger bit clear

    // Steps 2..4: timer counts down 3 -> 2 -> 1 -> 0
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 2U);
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 1U);
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 0U);
    assert((slot[0x37] & 0x02U) == 0U); // not loop frame yet

    // Step 5: timer was 0 -> advances to Frame 1 (offset 4, loop marker)
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x32) == 0x0456U); // sprite 1
    assert(read_u16(slot, 0x30) == 4U); // frame offset 4
    assert(read_u16(slot, 0x2E) == 3U); // timer 3
    assert((slot[0x37] & 0x04U) != 0U); // bit 2 set (loop frame mark)

    // Steps 6..8: countdown 3 -> 2 -> 1 -> 0
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 2U);
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 1U);
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x2E) == 0U);
    // At timer == 0 on loop frame, bit 1 is set!
    assert((slot[0x37] & 0x02U) != 0U);

    // Step 9: timer was 0 on loop frame -> loops back to Frame 0
    step_entity_animation(slot, anim_data);
    assert(read_u16(slot, 0x32) == 0x0123U);
    assert(read_u16(slot, 0x30) == 0U);
    assert(read_u16(slot, 0x2E) == 3U);
}

static void test_terrain_preflight() {
    std::array<std::uint8_t, 188> slot{};
    slot.fill(0U);

    auto write_i32 = [](std::span<std::uint8_t> s, std::size_t off, std::int32_t v) {
        auto u = static_cast<std::uint32_t>(v);
        s[off] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        s[off + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        s[off + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        s[off + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    };

    // 1. Intangible check
    slot[0x37] = 0x10U; // bit 4
    auto res = preflight_terrain_collision(slot, false);
    assert(res.action == TerrainPreflightAction::BypassIntangible);
    assert(slot[0x40] == 0U && slot[0x41] == 0U);

    // 2. Layer select check
    slot[0x37] = 0x00U;
    res = preflight_terrain_collision(slot, false);
    assert(res.layer_tilemap_ram == EntitySimConfig::kLayer1TilemapRam);

    res = preflight_terrain_collision(slot, true);
    assert(res.layer_tilemap_ram == EntitySimConfig::kLayer2TilemapRam);

    // 3. Flying check
    slot[0x38] = 0x02U; // bit 1
    res = preflight_terrain_collision(slot, false);
    assert(res.action == TerrainPreflightAction::Flying);

    // 4. Grounded velocity branches
    slot[0x38] = 0x00U;

    // vel_x == 0
    write_i32(slot, 0x72, 0);
    res = preflight_terrain_collision(slot, false);
    assert(res.action == TerrainPreflightAction::SweepY);
    assert(slot[0x40] == 0U);

    // vel_x < 0
    write_i32(slot, 0x72, -0x00010000);
    res = preflight_terrain_collision(slot, false);
    assert(res.action == TerrainPreflightAction::SweepNegX);
    assert(slot[0x40] == 0U);

    // vel_x > 0
    write_i32(slot, 0x72, 0x00010000);
    res = preflight_terrain_collision(slot, false);
    assert(res.action == TerrainPreflightAction::SweepPosX);
    assert(res.move_code == EntitySimConfig::kMoveCodePosX);
    assert(slot[0x40] == EntitySimConfig::kMoveCodePosX);
}

int main() {
    std::cout << "Running M16.147 Entity Sim, Anim & Preflight Tests..." << std::endl;
    test_rom_constants();
    test_entity_freeze_slowmo();
    test_entity_animation_stepper();
    test_terrain_preflight();
    std::cout << "All M16.147 Entity Sim, Anim & Preflight Tests Passed!" << std::endl;
    return 0;
}
