#include "game/combat/kill_counter_vm_tick.hpp"

#include <algorithm>

namespace oasis::game::combat {

ReclaimResult reclaim_pickup_slot(
    std::span<const ReclaimCandidateSlot> slots,
    std::uint8_t pending_drop_item_id) noexcept {
    const std::size_t num_slots = std::min<std::size_t>(slots.size(), KillCounterVmTickConfig::kPickupPoolSlots);

    // Pass 1 (0x00F64C..0x00F688): First slot where !bit7 && is_offscreen
    for (std::size_t i = 0U; i < num_slots; ++i) {
        const auto& slot = slots[i];
        if (!item_has_flag_bit7(slot.item_id) && slot.is_offscreen) {
            return ReclaimResult{
                .outcome = ReclaimOutcome::SlotReclaimed,
                .slot_index = i,
                .ram_addr = KillCounterVmTickConfig::kPickupPoolRam + static_cast<std::uint32_t>(i * KillCounterVmTickConfig::kPickupSlotStride),
                .vram_tile = static_cast<std::uint16_t>(KillCounterVmTickConfig::kPickupVramBaseTile + i * KillCounterVmTickConfig::kPickupVramTileStride)
            };
        }
    }

    // Pass 2 (0x00F68A..0x00F6BA): First slot where !bit7 (even on-screen)
    for (std::size_t i = 0U; i < num_slots; ++i) {
        const auto& slot = slots[i];
        if (!item_has_flag_bit7(slot.item_id)) {
            return ReclaimResult{
                .outcome = ReclaimOutcome::SlotReclaimed,
                .slot_index = i,
                .ram_addr = KillCounterVmTickConfig::kPickupPoolRam + static_cast<std::uint32_t>(i * KillCounterVmTickConfig::kPickupSlotStride),
                .vram_tile = static_cast<std::uint16_t>(KillCounterVmTickConfig::kPickupVramBaseTile + i * KillCounterVmTickConfig::kPickupVramTileStride)
            };
        }
    }

    // Fallback (0x00F6BE..0x00F6E0): All slots have bit 7 items.
    // Check if pending drop is a non-discardable key item (bit 5).
    if (is_item_key_essential(pending_drop_item_id)) {
        return ReclaimResult{
            .outcome = ReclaimOutcome::FatalOverflowError,
            .slot_index = 0U,
            .ram_addr = 0U,
            .vram_tile = 0U
        };
    }

    return ReclaimResult{
        .outcome = ReclaimOutcome::DropSuppressed,
        .slot_index = 0U,
        .ram_addr = 0U,
        .vram_tile = 0U
    };
}

std::uint16_t update_kill_counter_and_bonus_drop(
    KillCounterState& state,
    std::uint16_t current_fixed_drop,
    std::uint32_t attacker_payload,
    std::uint8_t enemy_flags_3a) noexcept {
    std::uint16_t updated_fixed_drop = current_fixed_drop;

    // ROM 0x00F740..0x00F758: If no fixed drop set and bonus drop flag active -> 0x1101
    if (current_fixed_drop == 0U && state.bonus_drop_flag != 0U) {
        state.bonus_drop_flag = 0U;
        updated_fixed_drop = KillCounterVmTickConfig::kGuaranteedDropItem;
    }

    // ROM 0x00F75E..0x00F786: Check attacker is Ali ($FF19E8) and enemy flags_3a bit 0 is clear
    const bool killed_by_ali = (attacker_payload == KillCounterVmTickConfig::kAliEntityRamAddr);
    const bool no_count_bit = (enemy_flags_3a & 0x01U) != 0U;

    state.kill_counter = increment_kill_counter_sat(state.kill_counter, killed_by_ali, no_count_bit);

    return updated_fixed_drop;
}

} // namespace oasis::game::combat
