#include "game/ui/food_menu.hpp"

namespace oasis::game::ui {

std::uint8_t step_cursor_4x4(
    std::uint8_t cursor,
    int dcol,
    int drow) noexcept {
    const int col = ((cursor & 0x03) + dcol) & 0x03;
    const int row = (((cursor >> 2) & 0x03) + drow) & 0x03;
    return static_cast<std::uint8_t>((row << 2) | col);
}

InventoryDropResult try_drop_food_item(
    std::span<InventorySlotPair, kFoodInventorySlotCount> inventory,
    std::uint8_t cursor,
    std::span<const std::uint8_t> item_flags_table,
    bool terrain_blocked) noexcept {
    if (cursor >= kFoodInventorySlotCount || terrain_blocked) {
        return InventoryDropResult{};
    }

    InventorySlotPair& slot = inventory[cursor];
    if (slot.item_id == 0U) {
        return InventoryDropResult{};
    }
    if (slot.item_id < item_flags_table.size() &&
        (item_flags_table[slot.item_id] & kFoodUndroppableMask) != 0U) {
        return InventoryDropResult{};
    }

    const std::uint8_t item_id = slot.item_id;
    const std::uint8_t qty = slot.qty;
    slot.item_id = 0U;
    slot.qty = 0U;
    return InventoryDropResult{
        true,
        static_cast<std::uint8_t>(item_id | kDropFlagMask),
        qty,
    };
}

} // namespace oasis::game::ui
