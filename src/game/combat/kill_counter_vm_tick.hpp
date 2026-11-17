#pragma once

#include "game/combat/enemy_loot_drop.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::combat {

struct KillCounterVmTickConfig {
    static constexpr std::uint32_t kReclaimPickupSlotAddr = 0x0000F64CU;
    static constexpr std::uint32_t kUpdateKillBonusAddr = 0x0000F740U;
    static constexpr std::uint32_t kStepEventVmTickAddr = 0x0000F788U;

    static constexpr std::uint32_t kPickupPoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kPickupPoolSlots = 8U;
    static constexpr std::size_t kPickupSlotStride = 0x5AU; // 90 bytes
    static constexpr std::uint16_t kPickupVramBaseTile = 0x0694U;
    static constexpr std::uint16_t kPickupVramTileStride = 9U;

    static constexpr std::uint32_t kGuaranteedDropFlagRam = 0x00FF0DBBU;
    static constexpr std::uint16_t kGuaranteedDropItem = 0x1101U;
    static constexpr std::uint32_t kAliEntityRamAddr = 0x00FF19E8U;
    static constexpr std::uint32_t kKillCounterRam = 0x00FF1A7CU;

    static constexpr std::uint32_t kEventVmPcRam = 0x00FF17B2U;
    static constexpr std::uint32_t kEventVmWaitTimerRam = 0x00FF17B6U;
    static constexpr std::uint32_t kEventVmFlagsRam = 0x00FF17B8U;
    static constexpr std::uint8_t kEventVmPauseBit = 0x02U; // Bit 1 of $FF17B8
    static constexpr std::uint8_t kNonDiscardableBit = 0x20U; // Bit 5 of item attribute
};

struct ReclaimCandidateSlot {
    std::uint8_t item_id{0U};
    bool is_offscreen{false};
};

enum class ReclaimOutcome : std::uint8_t {
    SlotReclaimed,
    DropSuppressed,
    FatalOverflowError, // Key item cannot be reclaimed
};

struct ReclaimResult {
    ReclaimOutcome outcome{ReclaimOutcome::DropSuppressed};
    std::size_t slot_index{0U};
    std::uint32_t ram_addr{0U};
    std::uint16_t vram_tile{0U};
};

struct KillCounterState {
    std::uint16_t kill_counter{0U};   // $FF1A7C (saturating at 0xFFFF)
    std::uint8_t bonus_drop_flag{0U}; // $FF0DBB
};

struct VmWaitTickResult {
    std::uint8_t wait_timer{0U};
    bool should_dispatch{false};
};

// Checks bit 5 (0x20) of ROM 0x0005CE16 item table: true for essential non-discardable key items.
[[nodiscard]] constexpr bool is_item_key_essential(std::uint8_t item_id) noexcept {
    if (item_id < EnemyLootConfig::kItemFlagsTable.size()) {
        return (EnemyLootConfig::kItemFlagsTable[item_id] & KillCounterVmTickConfig::kNonDiscardableBit) != 0U;
    }
    return false;
}

// ROM 0x00F64C..0x00F6E0:
// Reclaims an allocated pickup slot from the 8-slot $FF2ABC pool:
// Pass 1: first slot where !item_has_flag_bit7(item_id) && is_offscreen.
// Pass 2: first slot where !item_has_flag_bit7(item_id) (even if on-screen).
// Fallback: checks if pending fixed drop is non-discardable key item (bit 5).
//   If key item: FatalOverflowError.
//   Else: DropSuppressed.
[[nodiscard]] ReclaimResult reclaim_pickup_slot(
    std::span<const ReclaimCandidateSlot> slots,
    std::uint8_t pending_drop_item_id) noexcept;

// ROM 0x00F740..0x00F786:
// Updates bonus drop flag ($FF0DBB) and increments saturating 16-bit kill counter ($FF1A7C).
// Returns updated fixed drop word for enemy.
std::uint16_t update_kill_counter_and_bonus_drop(
    KillCounterState& state,
    std::uint16_t current_fixed_drop,
    std::uint32_t attacker_payload,
    std::uint8_t enemy_flags_3a) noexcept;

// Pure calculation of kill counter increment with 16-bit saturation.
[[nodiscard]] constexpr std::uint16_t increment_kill_counter_sat(
    std::uint16_t kills,
    bool killed_by_ali,
    bool no_count_bit) noexcept {
    if (!killed_by_ali || no_count_bit) {
        return kills;
    }
    return (kills >= 0xFFFFU) ? 0xFFFFU : static_cast<std::uint16_t>(kills + 1U);
}

// ROM 0x00F788..0x00F7A8: Event VM wait-timer tick & pause guard.
[[nodiscard]] constexpr VmWaitTickResult step_event_vm_wait(
    bool paused,
    std::uint8_t wait_timer) noexcept {
    if (paused) {
        return {.wait_timer = wait_timer, .should_dispatch = false};
    }
    if (wait_timer > 0U) {
        return {.wait_timer = static_cast<std::uint8_t>(wait_timer - 1U), .should_dispatch = false};
    }
    return {.wait_timer = 0U, .should_dispatch = true};
}

// ROM 0x00F788: Overload testing bit 1 of $FF17B8 control flags.
[[nodiscard]] constexpr VmWaitTickResult step_event_vm_tick(
    std::uint8_t vm_flags,
    std::uint8_t wait_timer) noexcept {
    const bool paused = (vm_flags & KillCounterVmTickConfig::kEventVmPauseBit) != 0U;
    return step_event_vm_wait(paused, wait_timer);
}

} // namespace oasis::game::combat
