#pragma once

#include "game/event/event_vm.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::items {

// ROM 0x05CE16 (64-byte Item Attribute Table) & 0x05CE56 (64-byte Pickup Tier Table).
struct ItemAttrConfig {
    static constexpr std::uint32_t kAttrTableRom = 0x0005CE16U;
    static constexpr std::uint32_t kPickupTierTableRom = 0x0005CE56U;
    static constexpr std::uint32_t kPickupHandlerRom = 0x00007A6CU;
    static constexpr std::uint32_t kPickupFlagSetterRom = 0x00007AF0U;
    static constexpr std::size_t kTableSize = 64U; // IDs 0x00..0x3F
    static constexpr std::uint8_t kNonDiscardableBit = 0x20U; // Bit 5
};

struct PickupOutcome {
    std::uint8_t tier{0U};
    std::uint32_t sound_cmd{0U};
    bool is_fanfare{false};
    std::uint8_t player_state{0U};  // 0x44 for fanfare tiers 3 & 4
    std::uint8_t player_action{0U}; // 0x4A for fanfare tiers 3 & 4
};

// Reads 1-byte attribute for `item_id` (0x00..0x3F) from ROM 0x05CE16.
[[nodiscard]] std::uint8_t get_item_attribute_byte(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept;

// Tests bit 5 (0x20) of ROM 0x05CE16 attribute byte: true for key/non-discardable items.
[[nodiscard]] bool is_item_non_discardable(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept;

// ROM 0x007A6C: resolves pickup tier (0..4) from 0x05CE56 and returns SFX/fanfare/state contract.
[[nodiscard]] PickupOutcome resolve_pickup_outcome(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept;

// ROM 0x007AF0: sets persistent pickup flag in bank 1 ($FF0DBC), 2 ($FF0E1C), or 3 ($FF0E2C).
bool apply_pickup_persistent_flag(
    event::EventVm& vm,
    std::uint8_t bank,
    std::uint16_t bit_index) noexcept;

} // namespace oasis::game::items
