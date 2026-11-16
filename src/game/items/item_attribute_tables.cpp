#include "game/items/item_attribute_tables.hpp"

namespace oasis::game::items {

std::uint8_t get_item_attribute_byte(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept {
    if (item_id >= ItemAttrConfig::kTableSize) {
        return 0U;
    }
    const std::size_t off = static_cast<std::size_t>(ItemAttrConfig::kAttrTableRom) + item_id;
    return (off < rom.size()) ? rom[off] : 0U;
}

bool is_item_non_discardable(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept {
    return (get_item_attribute_byte(rom, item_id) & ItemAttrConfig::kNonDiscardableBit) != 0U;
}

PickupOutcome resolve_pickup_outcome(
    std::span<const std::uint8_t> rom,
    std::uint8_t item_id) noexcept {
    PickupOutcome out{};
    if (item_id < ItemAttrConfig::kTableSize) {
        const std::size_t off = static_cast<std::size_t>(ItemAttrConfig::kPickupTierTableRom) + item_id;
        if (off < rom.size()) {
            out.tier = rom[off];
        }
    }

    // ROM 0x007A6C tier dispatch: 0..2 SFX, 3..4 fanfare + Player State 0x44 / Action 0x4A
    switch (out.tier) {
    case 0U:
        out.sound_cmd = 0x00C0035FU;
        break;
    case 1U:
        out.sound_cmd = 0x00C00360U;
        break;
    case 2U:
        out.sound_cmd = 0x00C00361U;
        break;
    case 3U:
        out.sound_cmd = 0x00000508U;
        out.is_fanfare = true;
        out.player_state = 0x44U;
        out.player_action = 0x4AU;
        break;
    case 4U:
        out.sound_cmd = 0x0000050DU;
        out.is_fanfare = true;
        out.player_state = 0x44U;
        out.player_action = 0x4AU;
        break;
    default:
        out.sound_cmd = 0x00C00360U;
        break;
    }
    return out;
}

bool apply_pickup_persistent_flag(
    event::EventVm& vm,
    std::uint8_t bank,
    std::uint16_t bit_index) noexcept {
    if (bank < 1U || bank > 3U) {
        return false;
    }
    vm.set_event_flag(bit_index, true, bank);
    return true;
}

} // namespace oasis::game::items
