#include "game/combat/kill_counter_vm_tick.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace oasis::game::combat;

static void test_constants_and_rom_addresses() {
    assert(KillCounterVmTickConfig::kReclaimPickupSlotAddr == 0x0000F64CU);
    assert(KillCounterVmTickConfig::kUpdateKillBonusAddr == 0x0000F740U);
    assert(KillCounterVmTickConfig::kStepEventVmTickAddr == 0x0000F788U);

    assert(KillCounterVmTickConfig::kPickupPoolRam == 0x00FF2ABCU);
    assert(KillCounterVmTickConfig::kPickupPoolSlots == 8U);
    assert(KillCounterVmTickConfig::kPickupSlotStride == 0x5AU);
    assert(KillCounterVmTickConfig::kPickupVramBaseTile == 0x0694U);
    assert(KillCounterVmTickConfig::kPickupVramTileStride == 9U);

    assert(KillCounterVmTickConfig::kGuaranteedDropFlagRam == 0x00FF0DBBU);
    assert(KillCounterVmTickConfig::kGuaranteedDropItem == 0x1101U);
    assert(KillCounterVmTickConfig::kAliEntityRamAddr == 0x00FF19E8U);
    assert(KillCounterVmTickConfig::kKillCounterRam == 0x00FF1A7CU);

    assert(KillCounterVmTickConfig::kEventVmPcRam == 0x00FF17B2U);
    assert(KillCounterVmTickConfig::kEventVmWaitTimerRam == 0x00FF17B6U);
    assert(KillCounterVmTickConfig::kEventVmFlagsRam == 0x00FF17B8U);
    assert(KillCounterVmTickConfig::kEventVmPauseBit == 0x02U);
}

static void test_python_reference_parity() {
    // Kill counter saturation: 10 + 1 = 11
    assert(increment_kill_counter_sat(10U, true, false) == 11U);
    // Saturated at 0xFFFF: 0xFFFF -> 0xFFFF
    assert(increment_kill_counter_sat(0xFFFFU, true, false) == 0xFFFFU);
    // Not killed by Ali: preserved
    assert(increment_kill_counter_sat(10U, false, false) == 10U);
    // Enemy exempt (no_count_bit = true): preserved
    assert(increment_kill_counter_sat(10U, true, true) == 10U);

    // Event VM wait timer parity
    auto tick_paused = step_event_vm_wait(true, 5U);
    assert(tick_paused.wait_timer == 5U && !tick_paused.should_dispatch);

    auto tick_counting = step_event_vm_wait(false, 5U);
    assert(tick_counting.wait_timer == 4U && !tick_counting.should_dispatch);

    auto tick_ready = step_event_vm_wait(false, 0U);
    assert(tick_ready.wait_timer == 0U && tick_ready.should_dispatch);
}

static void test_bonus_drop_and_kill_counter_state() {
    KillCounterState state{
        .kill_counter = 100U,
        .bonus_drop_flag = 1U
    };

    // Bonus drop active, fixed drop initially 0 -> sets fixed drop to 0x1101 and clears flag
    auto fixed_drop = update_kill_counter_and_bonus_drop(
        state, 0U, KillCounterVmTickConfig::kAliEntityRamAddr, 0U);
    assert(fixed_drop == 0x1101U);
    assert(state.bonus_drop_flag == 0U);
    assert(state.kill_counter == 101U);

    // Bonus drop now inactive, fixed drop already 0x0502 -> stays 0x0502
    fixed_drop = update_kill_counter_and_bonus_drop(
        state, 0x0502U, KillCounterVmTickConfig::kAliEntityRamAddr, 0U);
    assert(fixed_drop == 0x0502U);
    assert(state.kill_counter == 102U);

    // Non-Ali attacker payload -> kill counter does not increment
    fixed_drop = update_kill_counter_and_bonus_drop(
        state, 0x0502U, 0x00FF2000U, 0U);
    assert(state.kill_counter == 102U);

    // Enemy flags_3a bit 0 set (spirit/exempt) -> kill counter does not increment
    fixed_drop = update_kill_counter_and_bonus_drop(
        state, 0x0502U, KillCounterVmTickConfig::kAliEntityRamAddr, 1U);
    assert(state.kill_counter == 102U);
}

static void test_pickup_slot_reclaim_hierarchy() {
    // Pass 1: Find first slot where !bit7 && is_offscreen
    // Item 1 has bit 7 set (0xA1); Item 2 has bit 7 clear (0x01)
    std::vector<ReclaimCandidateSlot> slots(8);
    slots[0] = {.item_id = 1U, .is_offscreen = true};  // bit 7 set -> skipped
    slots[1] = {.item_id = 2U, .is_offscreen = false}; // on-screen -> skipped in Pass 1
    slots[2] = {.item_id = 2U, .is_offscreen = true};  // candidate!

    auto res1 = reclaim_pickup_slot(slots, 3U);
    assert(res1.outcome == ReclaimOutcome::SlotReclaimed);
    assert(res1.slot_index == 2U);
    assert(res1.ram_addr == KillCounterVmTickConfig::kPickupPoolRam + 2U * 90U);
    assert(res1.vram_tile == KillCounterVmTickConfig::kPickupVramBaseTile + 2U * 9U);

    // Pass 2: No offscreen items; reclaims first non-bit7 item even on-screen
    for (auto& s : slots) {
        s = {.item_id = 1U, .is_offscreen = false}; // all bit 7 items
    }
    slots[4] = {.item_id = 2U, .is_offscreen = false}; // item 2 on-screen

    auto res2 = reclaim_pickup_slot(slots, 3U);
    assert(res2.outcome == ReclaimOutcome::SlotReclaimed);
    assert(res2.slot_index == 4U);
    assert(res2.ram_addr == KillCounterVmTickConfig::kPickupPoolRam + 4U * 90U);
    assert(res2.vram_tile == KillCounterVmTickConfig::kPickupVramBaseTile + 4U * 9U);

    // Fallback: All 8 slots have bit 7 items.
    for (auto& s : slots) {
        s = {.item_id = 1U, .is_offscreen = true};
    }

    // Pending drop is regular food item 2 (0x01 in table: bit 5 == 0) -> DropSuppressed
    assert(!is_item_key_essential(2U));
    auto res3 = reclaim_pickup_slot(slots, 2U);
    assert(res3.outcome == ReclaimOutcome::DropSuppressed);

    // Pending drop is non-essential weapon item 22 (0x88 in table: bit 5 == 0) -> DropSuppressed
    assert(!is_item_key_essential(22U));
    auto res4 = reclaim_pickup_slot(slots, 22U);
    assert(res4.outcome == ReclaimOutcome::DropSuppressed);

    // Pending drop is essential key item 49 (0xA0 in table: bit 5 == 1) -> FatalOverflowError
    assert(is_item_key_essential(49U));
    auto res5 = reclaim_pickup_slot(slots, 49U);
    assert(res5.outcome == ReclaimOutcome::FatalOverflowError);
}

static void test_event_vm_tick_flags() {
    // Flag bit 1 set -> paused
    auto res1 = step_event_vm_tick(0x02U, 10U);
    assert(res1.wait_timer == 10U && !res1.should_dispatch);

    // Flag bit 1 clear -> decrements timer
    auto res2 = step_event_vm_tick(0x01U, 10U);
    assert(res2.wait_timer == 9U && !res2.should_dispatch);

    // Timer reached 0 -> ready to dispatch opcodes
    auto res3 = step_event_vm_tick(0x00U, 0U);
    assert(res3.wait_timer == 0U && res3.should_dispatch);
}

int main() {
    test_constants_and_rom_addresses();
    test_python_reference_parity();
    test_bonus_drop_and_kill_counter_state();
    test_pickup_slot_reclaim_hierarchy();
    test_event_vm_tick_flags();

    std::cout << "oasis_m16_133_kill_counter_vm_tick_test: ALL TESTS PASSED\n";
    return 0;
}
