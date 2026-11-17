#include "game/player/anim_palette_entity_stepper.hpp"

#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    assert(AnimPaletteEntityStepperConfig::kAnimPaletteSequencerAddr == 0x008E22U);
    assert(AnimPaletteEntityStepperConfig::kAnimPaletteSequencerEndAddr == 0x008E8EU);
    assert(AnimPaletteEntityStepperConfig::kEntityMasterStepperAddr == 0x008E90U);
    assert(AnimPaletteEntityStepperConfig::kEntityMasterStepperEndAddr == 0x008EF6U);
    assert(AnimPaletteEntityStepperConfig::kEntityStateResetAddr == 0x008EF8U);
    assert(AnimPaletteEntityStepperConfig::kEntityStateResetEndAddr == 0x008F10U);
    assert(AnimPaletteEntityStepperConfig::kSecondaryEntityDispatchAddr == 0x008F12U);
    assert(AnimPaletteEntityStepperConfig::kSecondaryEntityDispatchEndAddr == 0x008F22U);

    assert(AnimPaletteEntityStepperConfig::kAnimPaletteActiveRam == 0x00FF164FU);
    assert(AnimPaletteEntityStepperConfig::kAnimPaletteTablePtrRam == 0x00FF17A2U);
    assert(AnimPaletteEntityStepperConfig::kCramBufferRam == 0x00FF134CU);
    assert(AnimPaletteEntityStepperConfig::kCramSyncFlagRam == 0x00FF164DU);
    assert(AnimPaletteEntityStepperConfig::kCramSyncBit == 0U);
    assert(AnimPaletteEntityStepperConfig::kRecoveryVectorRam == 0x00FF193CU);

    assert(AnimPaletteEntityStepperConfig::kGroup1BaseRam == 0x00FF2954U);
    assert(AnimPaletteEntityStepperConfig::kGroup1Slots == 4U);
    assert(AnimPaletteEntityStepperConfig::kGroup1Stride == 90U);
    assert(AnimPaletteEntityStepperConfig::kGroup1RecoveryVector == 0x00008EAAU);
    assert(AnimPaletteEntityStepperConfig::kGroup1Handler == 0x00008F12U);

    assert(AnimPaletteEntityStepperConfig::kGroup2BaseRam == 0x00FF19E8U);
    assert(AnimPaletteEntityStepperConfig::kGroup2Slots == 21U);
    assert(AnimPaletteEntityStepperConfig::kGroup2Stride == 188U);
    assert(AnimPaletteEntityStepperConfig::kGroup2RecoveryVector == 0x00008ECCU);
    assert(AnimPaletteEntityStepperConfig::kGroup2Handler == 0x00008F22U);

    assert(AnimPaletteEntityStepperConfig::kGroup3BaseRam == 0x00FF2D8CU);
    assert(AnimPaletteEntityStepperConfig::kGroup3Slots == 6U);
    assert(AnimPaletteEntityStepperConfig::kGroup3Stride == 90U);
    assert(AnimPaletteEntityStepperConfig::kGroup3RecoveryVector == 0x00008EEEU);
    assert(AnimPaletteEntityStepperConfig::kGroup3Handler == 0x00008F12U);

    assert(AnimPaletteEntityStepperConfig::kStateOffset == 0x2AU);
    assert(AnimPaletteEntityStepperConfig::kTimerOffset == 0x2CU);
    assert(AnimPaletteEntityStepperConfig::kTimerInitVal == 0xFFFFU);
    assert(AnimPaletteEntityStepperConfig::kFlags37Offset == 0x37U);
    assert(AnimPaletteEntityStepperConfig::kFlags37Mask == 0xF9U);
    assert(AnimPaletteEntityStepperConfig::kSubtimer30Offset == 0x30U);
    assert(AnimPaletteEntityStepperConfig::kSubtimer2eOffset == 0x2EU);

    assert(AnimPaletteEntityStepperConfig::kSecondaryActiveBit == 4U);
    assert(AnimPaletteEntityStepperConfig::kSecondaryScriptEngineAddr == 0x0039CAU);
    assert(AnimPaletteEntityStepperConfig::kSecondaryBranchTarget == 0x008FA4U);
}

static void test_anim_palette_sequencer() {
    std::array<std::uint8_t, 128> cram{};
    cram.fill(0U);

    std::vector<AnimPaletteFrame> frames = {
        {4U, 0x0E00U}, // Frame 0: duration 4, blue
        {4U, 0x00E0U}, // Frame 1: duration 4, green
    };

    // Entry at offset 0, target CRAM offset 30 (color 15), initial frame 8 (frame 0), timer 4
    auto table = create_anim_palette_entry(30U, 8U, 4U, frames, 0U);

    // 1. Inactive test
    bool changed = step_anim_palette_raw(false, table, cram);
    assert(!changed);
    assert(cram[30] == 0U && cram[31] == 0U);

    // 2. Active step 1: timer 4 -> decrements to 2
    changed = step_anim_palette_raw(true, table, cram);
    assert(!changed);
    assert(cram[30] == 0U && cram[31] == 0U);

    // 3. Active step 2: timer 2 <= 2 -> expires! Advances to Frame 1 (offset 12), writes 0x00E0 to CRAM
    changed = step_anim_palette_raw(true, table, cram);
    assert(changed);
    assert(cram[30] == 0x00 && cram[31] == 0xE0);

    // 4. Active step 3: timer 4 -> decrements to 2
    changed = step_anim_palette_raw(true, table, cram);
    assert(!changed);

    // 5. Active step 4: timer 2 -> expires! Hits sentinel (0, 0), loops back to Frame 0 (offset 8), writes 0x0E00
    changed = step_anim_palette_raw(true, table, cram);
    assert(changed);
    assert(cram[30] == 0x0E && cram[31] == 0x00);
}

static void test_anim_palette_chained_entries() {
    std::array<std::uint8_t, 128> cram{};
    cram.fill(0U);

    std::vector<AnimPaletteFrame> frames1 = {{2U, 0x0EE0U}};
    std::vector<AnimPaletteFrame> frames2 = {{2U, 0x00EEU}};

    // Entry 1 stride = 8 + (1+1)*4 = 16 bytes
    auto e1 = create_anim_palette_entry(10U, 8U, 2U, frames1, 16U);
    auto e2 = create_anim_palette_entry(20U, 8U, 2U, frames2, 0U);

    std::vector<std::uint8_t> chained_table;
    chained_table.insert(chained_table.end(), e1.begin(), e1.end());
    chained_table.insert(chained_table.end(), e2.begin(), e2.end());

    bool changed = step_anim_palette_raw(true, chained_table, cram);
    assert(changed);
    assert(cram[10] == 0x0E && cram[11] == 0xE0);
    assert(cram[20] == 0x00 && cram[21] == 0xEE);
}

static void test_entity_state_reset() {
    std::array<std::uint8_t, 90> slot{};
    slot.fill(0xFFU);

    reset_entity_state(slot, 0x0007U);

    // +0x2A: new state 0x0007
    assert(slot[0x2A] == 0x00 && slot[0x2B] == 0x07);
    // +0x2C: timer 0xFFFF
    assert(slot[0x2C] == 0xFF && slot[0x2D] == 0xFF);
    // +0x37: masked with 0xF9
    assert(slot[0x37] == 0xF9);
    // +0x30: subtimer 1 cleared to 0
    assert(slot[0x30] == 0x00 && slot[0x31] == 0x00);
    // +0x2E: subtimer 2 cleared to 0
    assert(slot[0x2E] == 0x00 && slot[0x2F] == 0x00);

    // Other bytes untouched
    assert(slot[0x00] == 0xFF);
    assert(slot[0x38] == 0xFF);
}

static void test_secondary_entity_dispatch() {
    std::array<std::uint8_t, 90> slot{};
    slot.fill(0U);
    slot[0x37] = 0x02U;

    dispatch_secondary_entity(slot);
    // bit 4 set: 0x02 | 0x10 = 0x12
    assert(slot[0x37] == 0x12U);

    // Short buffer safety
    std::array<std::uint8_t, 10> short_slot{};
    dispatch_secondary_entity(short_slot);
}

static void test_entity_master_stepper() {
    std::vector<std::uint8_t> g1(AnimPaletteEntityStepperConfig::kGroup1Slots * AnimPaletteEntityStepperConfig::kGroup1Stride, 0U);
    std::vector<std::uint8_t> g2(AnimPaletteEntityStepperConfig::kGroup2Slots * AnimPaletteEntityStepperConfig::kGroup2Stride, 0U);
    std::vector<std::uint8_t> g3(AnimPaletteEntityStepperConfig::kGroup3Slots * AnimPaletteEntityStepperConfig::kGroup3Stride, 0U);

    auto write_u16 = [](std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t val) {
        buf[off] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    };

    // Group 1: slot 0 type 2, slot 2 type 4
    write_u16(g1, 0 * 90, 2U);
    write_u16(g1, 2 * 90, 4U);

    // Group 2: slot 1 type 6, slot 10 type 8
    write_u16(g2, 1 * 188, 6U);
    write_u16(g2, 10 * 188, 8U);

    // Group 3: slot 5 type 10
    write_u16(g3, 5 * 90, 10U);

    std::uint32_t recovery_ram = 0U;
    std::vector<std::pair<std::size_t, std::size_t>> secondary_invocations;
    std::vector<std::size_t> primary_invocations;

    auto on_sec = [&](std::size_t grp, std::size_t slot, std::span<std::uint8_t>) {
        secondary_invocations.emplace_back(grp, slot);
    };

    auto on_pri = [&](std::size_t slot, std::span<std::uint8_t>) {
        primary_invocations.push_back(slot);
    };

    auto stats = step_entity_groups_raw(g1, g2, g3, recovery_ram, on_sec, on_pri);

    assert(stats.group1_active_count == 2U);
    assert(stats.group2_active_count == 2U);
    assert(stats.group3_active_count == 1U);
    assert(stats.last_recovery_addr == AnimPaletteEntityStepperConfig::kGroup3RecoveryVector);
    assert(recovery_ram == AnimPaletteEntityStepperConfig::kGroup3RecoveryVector);

    // Verify secondary invocations
    assert(secondary_invocations.size() == 3U);
    assert(secondary_invocations[0] == std::make_pair(std::size_t(0), std::size_t(0)));
    assert(secondary_invocations[1] == std::make_pair(std::size_t(0), std::size_t(2)));
    assert(secondary_invocations[2] == std::make_pair(std::size_t(2), std::size_t(5)));

    // Verify bit 4 was set on active secondary slots
    assert((g1[0 * 90 + 0x37] & 0x10) != 0);
    assert((g1[2 * 90 + 0x37] & 0x10) != 0);
    assert((g3[5 * 90 + 0x37] & 0x10) != 0);

    // Inactive slots do not have bit 4 set
    assert((g1[1 * 90 + 0x37] & 0x10) == 0);
    assert((g3[0 * 90 + 0x37] & 0x10) == 0);

    // Verify primary invocations
    assert(primary_invocations.size() == 2U);
    assert(primary_invocations[0] == 1U);
    assert(primary_invocations[1] == 10U);
}

int main() {
    std::cout << "Running M16.146 Anim Palette & Entity Stepper Tests..." << std::endl;
    test_rom_constants();
    test_anim_palette_sequencer();
    test_anim_palette_chained_entries();
    test_entity_state_reset();
    test_secondary_entity_dispatch();
    test_entity_master_stepper();
    std::cout << "All M16.146 Anim Palette & Entity Stepper Tests Passed!" << std::endl;
    return 0;
}
