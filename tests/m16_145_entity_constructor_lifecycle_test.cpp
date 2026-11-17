#include "game/player/entity_constructor_lifecycle.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(EntityLifecycleConfig::kMasterEntityClearAddr == 0x008C5AU);
    assert(EntityLifecycleConfig::kMasterEntityClearEndAddr == 0x008D04U);
    assert(EntityLifecycleConfig::kUniversalConstructorAddr == 0x008D06U);
    assert(EntityLifecycleConfig::kUniversalConstructorEndAddr == 0x008DA2U);
    assert(EntityLifecycleConfig::kScriptPointersReinitAddr == 0x008DA4U);
    assert(EntityLifecycleConfig::kScriptPointersReinitEndAddr == 0x008DD2U);
    assert(EntityLifecycleConfig::kSpiritGlowPulseAddr == 0x008DD4U);
    assert(EntityLifecycleConfig::kSpiritGlowPulseEndAddr == 0x008E22U);

    assert(EntityLifecycleConfig::kDescriptorTableRomAddr == 0x0003F2FAU);
    assert(EntityLifecycleConfig::kDescriptorStrideBytes == 32U);
    assert(EntityLifecycleConfig::kSmallEntityThresholdRam == 0x00FF2954U);

    assert(EntityLifecycleConfig::kPool1BaseRam == 0x00FF2954U);
    assert(EntityLifecycleConfig::kPool1Slots == 4U);
    assert(EntityLifecycleConfig::kPool1Stride == 90U);

    assert(EntityLifecycleConfig::kPool2BaseRam == 0x00FF2D8CU);
    assert(EntityLifecycleConfig::kPool2Slots == 6U);
    assert(EntityLifecycleConfig::kPool2Stride == 90U);

    assert(EntityLifecycleConfig::kPool3BaseRam == 0x00FF2ABCU);
    assert(EntityLifecycleConfig::kPool3Slots == 8U);
    assert(EntityLifecycleConfig::kPool3Stride == 90U);

    assert(EntityLifecycleConfig::kPool4BaseRam == 0x00FF19C8U);
    assert(EntityLifecycleConfig::kPool4Slots == 4U);
    assert(EntityLifecycleConfig::kPool4Stride == 8U);

    assert(EntityLifecycleConfig::kPlayerSlotRam == 0x00FF19E8U);
    assert(EntityLifecycleConfig::kSpiritSlotRam == 0x00FF1AA4U);
    assert(EntityLifecycleConfig::kPool5BaseRam == 0x00FF1B60U);
    assert(EntityLifecycleConfig::kPool5Slots == 19U);
    assert(EntityLifecycleConfig::kPool5Stride == 188U);

    assert(EntityLifecycleConfig::kGlowLevelRam == 0x00FF183EU);
    assert(EntityLifecycleConfig::kActiveSpiritRam == 0x00FF1AA4U);
    assert(EntityLifecycleConfig::kTimerWordRam == 0x00FF199EU);
    assert(EntityLifecycleConfig::kTimerSubbyteRam == 0x00FF199FU);
}

static void test_clear_entity_slots() {
    // 90-byte slot
    std::array<std::uint8_t, 90> slot90{};
    slot90.fill(0xAAU);
    clear_entity_slot_90(slot90);

    assert(slot90[0x00] == 0x00 && slot90[0x01] == 0x00);
    assert(slot90[0x02] == 0xAA && slot90[0x03] == 0xAA);
    assert(slot90[0x04] == 0x00 && slot90[0x05] == 0x00);
    assert(slot90[0x06] == 0x00 && slot90[0x07] == 0x00);
    assert(slot90[0x2A] == 0x00 && slot90[0x2B] == 0x00);
    assert(slot90[0x2C] == 0xFF && slot90[0x2D] == 0xFF);
    assert(slot90[0x34] == 0xFF && slot90[0x35] == 0xFF);
    assert(slot90[89] == 0xAA);

    // 188-byte slot
    std::array<std::uint8_t, 188> slot188{};
    slot188.fill(0xBBU);
    clear_entity_slot_188(slot188);

    assert(slot188[0x00] == 0x00 && slot188[0x01] == 0x00);
    assert(slot188[0x04] == 0x00 && slot188[0x05] == 0x00);
    assert(slot188[0x06] == 0x00 && slot188[0x07] == 0x00);
    assert(slot188[0x2A] == 0x00 && slot188[0x2B] == 0x00);
    assert(slot188[0x2C] == 0xFF && slot188[0x2D] == 0xFF);
    assert(slot188[0x34] == 0xFF && slot188[0x35] == 0xFF);
    assert(slot188[0x5C] == 0xFF && slot188[0x5D] == 0xFF);
    assert(slot188[187] == 0xBB);

    // 8-byte special slot
    std::array<std::uint8_t, 8> slot8{};
    slot8.fill(0xCCU);
    clear_special_slot_8(slot8);
    assert(slot8[0x00] == 0x00 && slot8[0x01] == 0x00);
    assert(slot8[0x02] == 0xCC);

    // Short buffer safety
    std::array<std::uint8_t, 10> short_buf{};
    clear_entity_slot_90(short_buf);
    clear_entity_slot_188(short_buf);
}

static void test_entity_pool_set() {
    EntityPoolSet pools;
    pools.init_default();

    assert(pools.pool1_90b.size() == 4U);
    assert(pools.pool2_90b.size() == 6U);
    assert(pools.pool3_90b.size() == 8U);
    assert(pools.pool4_8b.size() == 4U);
    assert(pools.pool5_188b.size() == 19U);

    // Mark player and spirit slots
    pools.player_slot.fill(0x5AU);
    pools.spirit_slot.fill(0xA5U);

    // Mark slots with garbage
    for (auto& slot : pools.pool1_90b) slot.fill(0x11U);
    for (auto& slot : pools.pool2_90b) slot.fill(0x22U);
    for (auto& slot : pools.pool3_90b) slot.fill(0x33U);
    for (auto& slot : pools.pool4_8b) slot.fill(0x44U);
    for (auto& slot : pools.pool5_188b) slot.fill(0x55U);

    pools.clear_subsystems();

    // Verify pools 1-3 have timer 0xFFFF at 0x2C
    for (const auto& slot : pools.pool1_90b) {
        assert(slot[0x00] == 0x00 && slot[0x2C] == 0xFF && slot[0x2D] == 0xFF);
    }
    for (const auto& slot : pools.pool2_90b) {
        assert(slot[0x00] == 0x00 && slot[0x2C] == 0xFF && slot[0x2D] == 0xFF);
    }
    for (const auto& slot : pools.pool3_90b) {
        assert(slot[0x00] == 0x00 && slot[0x2C] == 0xFF && slot[0x2D] == 0xFF);
    }
    // Verify pool 4 has word 0 cleared
    for (const auto& slot : pools.pool4_8b) {
        assert(slot[0x00] == 0x00 && slot[0x01] == 0x00 && slot[0x02] == 0x44);
    }
    // Verify pool 5 has 0x5C timer cleared to 0xFFFF
    for (const auto& slot : pools.pool5_188b) {
        assert(slot[0x00] == 0x00 && slot[0x5C] == 0xFF && slot[0x5D] == 0xFF);
    }

    // Verify player and spirit were preserved
    for (std::uint8_t byte : pools.player_slot) {
        assert(byte == 0x5AU);
    }
    for (std::uint8_t byte : pools.spirit_slot) {
        assert(byte == 0xA5U);
    }
}

static void test_descriptor_reading() {
    // Descriptor table with 2 entries (each 32 bytes)
    std::vector<std::uint8_t> table(64, 0U);

    // Entry 0 (type 2):
    // sprite_id = 0x0102
    table[0] = 0x01; table[1] = 0x02;
    table[2] = 0x12; // radius
    table[3] = 0xFA; // offset_x (-6)
    table[4] = 0x0A; // offset_y (+10)
    table[5] = 0x82; // tile_attr
    table[6] = 0x24; // height
    table[7] = 0x01; // anim_flags
    table[8] = 0x03; // collision_flags
    table[9] = 0x05; // defense_flags
    table[10] = 0x07; // render_flags
    table[11] = 0x09; // ai_flags
    // fn_ptr_1a = 0x00010000
    table[12] = 0x00; table[13] = 0x01; table[14] = 0x00; table[15] = 0x00;
    // fn_ptr_1e = 0x00020000
    table[16] = 0x00; table[17] = 0x02; table[18] = 0x00; table[19] = 0x00;
    // fn_ptr_26 = 0x00030000
    table[20] = 0x00; table[21] = 0x03; table[22] = 0x00; table[23] = 0x00;
    // fn_ptr_22 = 0x00040000
    table[24] = 0x00; table[25] = 0x04; table[26] = 0x00; table[27] = 0x00;
    // fn_ptr_5e = 0x00050000
    table[28] = 0x00; table[29] = 0x05; table[30] = 0x00; table[31] = 0x00;

    // Entry 1 (type 4):
    table[32] = 0x04; table[33] = 0x08; // sprite_id = 0x0408

    // Read type < 2 -> nullopt
    auto desc_invalid = read_entity_descriptor(table, 0, false);
    assert(!desc_invalid.has_value());

    // Read type 2 (offset 0)
    auto desc2 = read_entity_descriptor(table, 2, false);
    assert(desc2.has_value());
    assert(desc2->sprite_id == 0x0102U);
    assert(desc2->radius == 0x12U);
    assert(desc2->offset_x == -6);
    assert(desc2->offset_y == 10);
    assert(desc2->tile_attribute == 0x82U);
    assert(desc2->height == 0x24U);
    assert(desc2->anim_flags == 0x01U);
    assert(desc2->collision_flags == 0x03U);
    assert(desc2->defense_flags == 0x05U);
    assert(desc2->render_flags == 0x07U);
    assert(desc2->ai_flags == 0x09U);
    assert(desc2->fn_ptr_1a == 0x00010000U);
    assert(desc2->fn_ptr_1e == 0x00020000U);
    assert(desc2->fn_ptr_26 == 0x00030000U);
    assert(desc2->fn_ptr_22 == 0x00040000U);
    assert(desc2->fn_ptr_5e == 0x00050000U);

    // Read type 4 (offset 32)
    auto desc4 = read_entity_descriptor(table, 4, false);
    assert(desc4.has_value());
    assert(desc4->sprite_id == 0x0408U);

    // Read type 6 (offset 64 -> out of range)
    auto desc6 = read_entity_descriptor(table, 6, false);
    assert(!desc6.has_value());

    // Test with full ROM mock
    std::vector<std::uint8_t> mock_rom(EntityLifecycleConfig::kDescriptorTableRomAddr + 64, 0U);
    std::copy(table.begin(), table.end(), mock_rom.begin() + EntityLifecycleConfig::kDescriptorTableRomAddr);

    auto desc_rom2 = read_entity_descriptor(mock_rom, 2, true);
    assert(desc_rom2.has_value());
    assert(desc_rom2->sprite_id == 0x0102U);
}

static void test_universal_constructor() {
    EntityDescriptor desc{};
    desc.sprite_id = 0x0345U;
    desc.radius = 0x18U;
    desc.offset_x = -4;
    desc.offset_y = 8;
    desc.tile_attribute = 0x40U;
    desc.height = 0x30U;
    desc.anim_flags = 0x11U;
    desc.collision_flags = 0x22U;
    desc.defense_flags = 0x33U;
    desc.render_flags = 0x44U;
    desc.ai_flags = 0x55U;
    desc.fn_ptr_1a = 0x00011111U;
    desc.fn_ptr_1e = 0x00022222U;
    desc.fn_ptr_26 = 0x00033333U;
    desc.fn_ptr_22 = 0x00044444U;
    desc.fn_ptr_5e = 0x00055555U;

    // Small slot constructor
    std::array<std::uint8_t, 90> small_slot{};
    small_slot.fill(0xEEU);
    construct_entity(small_slot, 4U, desc, true);

    // Type at 0x00
    assert(small_slot[0x00] == 0x00 && small_slot[0x01] == 0x04);
    // Zero-cleared regions
    assert(small_slot[0x02] == 0x00 && small_slot[0x03] == 0x00);
    // Fields
    assert(small_slot[0x4A] == 0x03 && small_slot[0x4B] == 0x45);
    assert(small_slot[0x46] == 0x00 && small_slot[0x47] == 0x18);
    assert(small_slot[0x42] == 0x00 && small_slot[0x43] == 0x18);
    assert(small_slot[0x44] == 0x00 && small_slot[0x45] == 0x18);
    assert(static_cast<std::int8_t>(small_slot[0x48]) == -4);
    assert(static_cast<std::int8_t>(small_slot[0x49]) == 8);
    assert(small_slot[0x3D] == 0x40);
    assert(small_slot[0x4C] == 0x00 && small_slot[0x4D] == 0x30);
    assert(small_slot[0x3A] == 0x11);
    assert(small_slot[0x36] == 0x22);
    assert(small_slot[0x37] == 0x33);
    assert(small_slot[0x38] == 0x44);
    assert(small_slot[0x3C] == 0x55);

    // Function pointers
    auto read_u32 = [](const auto& s, std::size_t off) -> std::uint32_t {
        return (static_cast<std::uint32_t>(s[off]) << 24U) |
               (static_cast<std::uint32_t>(s[off + 1]) << 16U) |
               (static_cast<std::uint32_t>(s[off + 2]) << 8U) |
               static_cast<std::uint32_t>(s[off + 3]);
    };
    assert(read_u32(small_slot, 0x1A) == 0x00011111U);
    assert(read_u32(small_slot, 0x1E) == 0x00022222U);
    assert(read_u32(small_slot, 0x26) == 0x00033333U);
    assert(read_u32(small_slot, 0x22) == 0x00044444U);

    // Timers
    assert(small_slot[0x2C] == 0xFF && small_slot[0x2D] == 0xFF);
    assert(small_slot[0x34] == 0xFF && small_slot[0x35] == 0xFF);

    // Large slot constructor
    std::array<std::uint8_t, 188> large_slot{};
    large_slot.fill(0xDDU);
    construct_entity(large_slot, 6U, desc, false);

    assert(large_slot[0x00] == 0x00 && large_slot[0x01] == 0x06);
    assert(read_u32(large_slot, 0x5E) == 0x00055555U);
    assert(large_slot[0x5C] == 0xFF && large_slot[0x5D] == 0xFF);
    assert(large_slot[0x2C] == 0xFF && large_slot[0x2D] == 0xFF);
    assert(large_slot[0x34] == 0xFF && large_slot[0x35] == 0xFF);
}

static void test_script_pointers_reinit() {
    EntityDescriptor desc{};
    desc.fn_ptr_1a = 0x11112222U;
    desc.fn_ptr_1e = 0x33334444U;
    desc.fn_ptr_26 = 0x55556666U;
    desc.fn_ptr_22 = 0x77778888U;
    desc.fn_ptr_5e = 0x9999AAAAU;

    std::array<std::uint8_t, 188> slot{};
    reinit_entity_script_pointers(slot, desc);

    auto read_u32 = [](const auto& s, std::size_t off) -> std::uint32_t {
        return (static_cast<std::uint32_t>(s[off]) << 24U) |
               (static_cast<std::uint32_t>(s[off + 1]) << 16U) |
               (static_cast<std::uint32_t>(s[off + 2]) << 8U) |
               static_cast<std::uint32_t>(s[off + 3]);
    };
    assert(read_u32(slot, 0x1A) == 0x11112222U);
    assert(read_u32(slot, 0x1E) == 0x33334444U);
    assert(read_u32(slot, 0x26) == 0x55556666U);
    assert(read_u32(slot, 0x22) == 0x77778888U);
    assert(read_u32(slot, 0x5E) == 0x9999AAAAU);
}

static void test_spirit_glow_pulse() {
    // Active spirit: ramp up
    assert(step_spirit_glow_pulse(0, true, 0) == 1);
    assert(step_spirit_glow_pulse(1, true, 0) == 2);
    assert(step_spirit_glow_pulse(2, true, 0) == 3);

    // Active spirit: steady at 3 when anim_timer < 20
    assert(step_spirit_glow_pulse(3, true, 0) == 3);
    assert(step_spirit_glow_pulse(3, true, 19) == 3);

    // Active spirit: pulse to 4 on even frames >= 20
    assert(step_spirit_glow_pulse(3, true, 20) == 4);
    assert(step_spirit_glow_pulse(3, true, 21) == 3);
    assert(step_spirit_glow_pulse(3, true, 22) == 4);
    assert(step_spirit_glow_pulse(3, true, 23) == 3);

    // Active spirit: clamping when already at 4
    assert(step_spirit_glow_pulse(4, true, 20) == 4);
    assert(step_spirit_glow_pulse(4, true, 21) == 3);

    // Dismissed spirit: ramp down
    assert(step_spirit_glow_pulse(4, false, 0) == 2); // 4 drops straight to 2
    assert(step_spirit_glow_pulse(3, false, 0) == 2);
    assert(step_spirit_glow_pulse(2, false, 0) == 1);
    assert(step_spirit_glow_pulse(1, false, 0) == 0);
    assert(step_spirit_glow_pulse(0, false, 0) == 0);
}

int main() {
    std::cout << "Running M16.145 Entity Constructor & Lifecycle Tests..." << std::endl;
    test_rom_constants();
    test_clear_entity_slots();
    test_entity_pool_set();
    test_descriptor_reading();
    test_universal_constructor();
    test_script_pointers_reinit();
    test_spirit_glow_pulse();
    std::cout << "All M16.145 Entity Constructor & Lifecycle Tests Passed!" << std::endl;
    return 0;
}
