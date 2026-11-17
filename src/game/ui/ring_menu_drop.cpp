#include "game/ui/ring_menu_drop.hpp"

namespace oasis::game::ui {

std::uint8_t step_cursor_4x2(
    std::uint8_t cursor,
    int dcol,
    int drow) noexcept {
    const int col = ((cursor & 0x03) + dcol) & 0x03;
    const int row = (((cursor >> 2) & 0x01) + drow) & 0x01;
    return static_cast<std::uint8_t>((row << 2) | col);
}

InventoryDropResult try_drop_inventory_item(
    std::span<InventorySlotPair, kRingInventorySlotCount> inventory,
    std::uint8_t cursor,
    std::uint8_t protected_slot,
    bool terrain_blocked) noexcept {
    if (cursor >= kRingInventorySlotCount ||
        cursor == protected_slot ||
        terrain_blocked) {
        return InventoryDropResult{};
    }

    InventorySlotPair& slot = inventory[cursor];
    if (slot.item_id < kMinDroppableItemId) {
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
