#include "game/player/horizontal_entity_sweeps.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::collision;

static void test_rom_constants() {
    static_assert(HorizontalEntitySweepsConfig::kGroup2EntityPoolAddr == 0x00FF19E8U);
    static_assert(HorizontalEntitySweepsConfig::kGroup2EntitySlots == 21U);
    static_assert(HorizontalEntitySweepsConfig::kEntitySlotStride == 0xBCU);
    static_assert(HorizontalEntitySweepsConfig::kTypeOffset == 0x00U);
    static_assert(HorizontalEntitySweepsConfig::kXOffset == 0x08U);
    static_assert(HorizontalEntitySweepsConfig::kYOffset == 0x0CU);
    static_assert(HorizontalEntitySweepsConfig::kZOffset == 0x10U);
    static_assert(HorizontalEntitySweepsConfig::kGroundHOffset == 0x14U);
    static_assert(HorizontalEntitySweepsConfig::kFlags37Offset == 0x37U);
    static_assert(HorizontalEntitySweepsConfig::kFlags38Offset == 0x38U);
    static_assert(HorizontalEntitySweepsConfig::kFlags3AOffset == 0x3AU);
    static_assert(HorizontalEntitySweepsConfig::kCollisionFlagsXOffset == 0x40U);
    static_assert(HorizontalEntitySweepsConfig::kRadXOffset == 0x42U);
    static_assert(HorizontalEntitySweepsConfig::kRadYOffset == 0x44U);
    static_assert(HorizontalEntitySweepsConfig::kHeightOffset == 0x4AU);
    static_assert(HorizontalEntitySweepsConfig::kCollidedPtrOffset == 0x68U);
    static_assert(HorizontalEntitySweepsConfig::kReactionCodeOffset == 0x6CU);
    static_assert(HorizontalEntitySweepsConfig::kVelXOffset == 0x72U);
    static_assert(HorizontalEntitySweepsConfig::kFlagCannotPushMask == 0x20U);
    static_assert(HorizontalEntitySweepsConfig::kFlagCannotBePushedMask == 0x40U);
    static_assert(HorizontalEntitySweepsConfig::kFlagIntangibleMask == 0x04U);
    static_assert(HorizontalEntitySweepsConfig::kFlagIgnoreCollisionMask == 0x02U);
    static_assert(HorizontalEntitySweepsConfig::kCollisionFlagEntityHit == 0x80U);
    static_assert(HorizontalEntitySweepsConfig::kReactionCodeSolidEntity == 3U);
}

static void test_intangible_self_skips_sweep() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.flags_38 = 0x04U; // intangible

    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 110;
    target.y = 100;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_pos_x(10, self, pool);
    assert(step == 10);
    assert(self.collision_flags_x == 0U);
}

static void test_pos_x_sweep_clear_when_target_far() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // self right = 100 + 8 = 108. target left = 130 - 8 = 122. dist = 14. dx = 10 < 14
    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 130;
    target.y = 100;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_pos_x(10, self, pool);
    assert(step == 10);
    assert(self.collision_flags_x == 0U);
}

static void test_pos_x_sweep_clamps_step_on_overlap() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target left = 120 - 8 = 112. self right = 108. dist = 4. dx = 10 > 4 -> clamped to 4
    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 120;
    target.y = 100;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_pos_x(10, self, pool);
    assert(step == 4);
    assert(self.collision_flags_x == 0x80U);
    assert(self.collided_entity_id == 0x00FF1AA4U);
    assert(self.reaction_code == 3U);
    assert(pool[0].vel_x == 0);
}

static void test_pos_x_sweep_flush_contact_pushes_target() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target left = 116 - 8 = 108. self right = 108. dist = 0 -> clamped to 0 and push!
    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 116;
    target.y = 100;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_pos_x(5, self, pool);
    assert(step == 0);
    assert(self.collision_flags_x == 0x80U);
    assert(pool[0].vel_x == 1); // pushed +1
}

static void test_pos_x_sweep_push_guards() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;
    self.flags_37 = 0x20U; // cannot push

    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 116;
    target.y = 100;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_pos_x(5, self, pool);
    assert(step == 0);
    assert(pool[0].vel_x == 0); // push prevented

    // Target cannot be pushed
    self.flags_37 = 0x00U;
    pool[0].flags_37 = 0x40U; // cannot be pushed
    step = sweep_entities_pos_x(5, self, pool);
    assert(step == 0);
    assert(pool[0].vel_x == 0); // push prevented
}

static void test_pos_x_sweep_disjoint_misses() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // Y disjoint: target Y = 130 (top = 122 > self bot = 108)
    DynamicEntityHeader tgt_y{};
    tgt_y.type = 2;
    tgt_y.x = 116;
    tgt_y.y = 130;
    tgt_y.rad_x = 8;
    tgt_y.rad_y = 8;
    tgt_y.height = 16;

    std::vector<DynamicEntityHeader> pool = {tgt_y};
    auto step = sweep_entities_pos_x(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_x == 0U);

    // Z disjoint: target Z = 30 (elev = 30 >= self top = 16)
    DynamicEntityHeader tgt_z{};
    tgt_z.type = 2;
    tgt_z.x = 116;
    tgt_z.y = 100;
    tgt_z.z = 30;
    tgt_z.rad_x = 8;
    tgt_z.rad_y = 8;
    tgt_z.height = 16;

    pool = {tgt_z};
    step = sweep_entities_pos_x(5, self, pool);
    assert(step == 5);
    assert(self.collision_flags_x == 0U);
}

static void test_neg_x_sweep_clamps_and_pushes_left() {
    DynamicEntityHeader self{};
    self.slot_id = 0x00FF19E8U;
    self.type = 1;
    self.x = 100;
    self.y = 100;
    self.rad_x = 8;
    self.rad_y = 8;
    self.height = 16;

    // target right = 84 + 8 = 92. self left = 92. dist = 0 -> push left!
    DynamicEntityHeader target{};
    target.slot_id = 0x00FF1AA4U;
    target.type = 2;
    target.x = 84;
    target.y = 100;
    target.rad_x = 8;
    target.rad_y = 8;
    target.height = 16;

    std::vector<DynamicEntityHeader> pool = {target};
    auto step = sweep_entities_neg_x(5, self, pool);
    assert(step == 0);
    assert(self.collision_flags_x == 0x80U);
    assert(self.collided_entity_id == 0x00FF1AA4U);
    assert(self.reaction_code == 3U);
    assert(pool[0].vel_x == -1); // pushed -1 in X
}

static void test_slot_bytes_integration() {
    // 188-byte self slot
    std::vector<std::uint8_t> self_slot(0xBC, 0);
    self_slot[0x00] = 0x00; self_slot[0x01] = 0x01; // type = 1
    self_slot[0x08] = 0x00; self_slot[0x09] = 0x64; // X = 100
    self_slot[0x0C] = 0x00; self_slot[0x0D] = 0x64; // Y = 100
    self_slot[0x42] = 0x00; self_slot[0x43] = 0x08; // rad_x = 8
    self_slot[0x44] = 0x00; self_slot[0x45] = 0x08; // rad_y = 8
    self_slot[0x4A] = 0x00; self_slot[0x4B] = 0x10; // height = 16

    // 21 slots of 188 bytes each
    std::vector<std::uint8_t> pool_bytes(21 * 0xBC, 0);
    // Target in slot 1 (offset 0xBC): X = 116 (dist = 0)
    const std::size_t slot1_off = 0xBC;
    pool_bytes[slot1_off + 0x00] = 0x00; pool_bytes[slot1_off + 0x01] = 0x02; // type = 2
    pool_bytes[slot1_off + 0x08] = 0x00; pool_bytes[slot1_off + 0x09] = 0x74; // X = 116
    pool_bytes[slot1_off + 0x0C] = 0x00; pool_bytes[slot1_off + 0x0D] = 0x64; // Y = 100
    pool_bytes[slot1_off + 0x42] = 0x00; pool_bytes[slot1_off + 0x43] = 0x08; // rad_x = 8
    pool_bytes[slot1_off + 0x44] = 0x00; pool_bytes[slot1_off + 0x45] = 0x08; // rad_y = 8
    pool_bytes[slot1_off + 0x4A] = 0x00; pool_bytes[slot1_off + 0x4B] = 0x10; // height = 16

    auto step = sweep_slot_entities_pos_x(5, self_slot, pool_bytes);
    assert(step == 0);
    assert(self_slot[0x40] == 0x80);
    assert(self_slot[0x6C] == 3);
    // collided_ptr at 0x68 should match 0x00FF19E8 + 0xBC = 0x00FF1AA4
    std::uint32_t ptr = (static_cast<std::uint32_t>(self_slot[0x68]) << 24) |
                        (static_cast<std::uint32_t>(self_slot[0x69]) << 16) |
                        (static_cast<std::uint32_t>(self_slot[0x6A]) << 8)  |
                        (static_cast<std::uint32_t>(self_slot[0x6B]));
    assert(ptr == 0x00FF1AA4U);

    // Target vel_x in slot 1 should be +1
    std::int16_t vel_x = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(pool_bytes[slot1_off + 0x72]) << 8) |
         static_cast<std::uint16_t>(pool_bytes[slot1_off + 0x73]));
    assert(vel_x == 1);
}

int main() {
    std::cout << "Running M16.157 Horizontal Entity Sweeps Tests..." << std::endl;
    test_rom_constants();
    test_intangible_self_skips_sweep();
    test_pos_x_sweep_clear_when_target_far();
    test_pos_x_sweep_clamps_step_on_overlap();
    test_pos_x_sweep_flush_contact_pushes_target();
    test_pos_x_sweep_push_guards();
    test_pos_x_sweep_disjoint_misses();
    test_neg_x_sweep_clamps_and_pushes_left();
    test_slot_bytes_integration();
    std::cout << "All M16.157 tests PASSED!" << std::endl;
    return 0;
}
