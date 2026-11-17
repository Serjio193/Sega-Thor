#include "game/ui/inventory_grid_ui.hpp"

namespace oasis::game::ui {

namespace {

constexpr std::uint32_t kBytesPerTile4bpp = 32U;

[[nodiscard]] InventoryGridIconEntry make_icon_entry(
    std::uint8_t slot,
    std::int16_t x,
    std::int16_t y,
    std::uint16_t item_id,
    std::uint16_t buffer_slot) noexcept {
    const std::uint16_t tile_index = static_cast<std::uint16_t>(item_id * kIconTileStride);
    return InventoryGridIconEntry{
        slot,
        x,
        y,
        item_id,
        tile_index,
        kIconRomBase + static_cast<std::uint32_t>(tile_index) * kBytesPerTile4bpp,
        buffer_slot,
    };
}

} // namespace

GridSlotCoord grid_slot_to_xy(std::uint8_t slot) noexcept {
    const std::int16_t col = static_cast<std::int16_t>(slot & 0x03U);
    const std::int16_t row = static_cast<std::int16_t>((slot & 0x0CU) >> 2U);
    return GridSlotCoord{
        static_cast<std::int16_t>(kGridOriginX + col * kGridCellPitchPx),
        static_cast<std::int16_t>(kGridOriginY + row * kGridCellPitchPx),
    };
}

std::vector<InventoryGridIconEntry> build_inventory_grid_icon_plan(
    const std::array<std::uint16_t, kInventoryGridSlots>& inventory_slots,
    std::uint16_t spirit_slot,
    std::uint16_t special_slot) {
    std::vector<InventoryGridIconEntry> plan;
    plan.reserve(kInventoryGridSlots + 2U);

    for (std::size_t i = 0U; i < kInventoryGridSlots; ++i) {
        const std::uint16_t item_id = inventory_slots[i];
        if (item_id == 0U) {
            continue;
        }
        const auto slot = static_cast<std::uint8_t>(i);
        const GridSlotCoord coord = grid_slot_to_xy(slot);
        plan.push_back(make_icon_entry(
            slot,
            coord.x,
            static_cast<std::int16_t>(coord.y + kIconYBias),
            item_id,
            static_cast<std::uint16_t>(i * kIconTileStride)));
    }

    if (spirit_slot != 0U) {
        const GridSlotCoord origin = grid_slot_to_xy(0U);
        plan.push_back(make_icon_entry(
            0U,
            origin.x,
            origin.y,
            spirit_slot,
            kSpiritBufferSlot));
    }

    if (special_slot != 0U) {
        const GridSlotCoord origin = grid_slot_to_xy(0U);
        plan.push_back(make_icon_entry(
            0U,
            origin.x,
            origin.y,
            special_slot,
            kSpecialBufferSlot));
    }

    return plan;
}

} // namespace oasis::game::ui
