#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::ui {

inline constexpr std::uint32_t kStepCursor4x2Addr = 0x00462EU;
inline constexpr std::uint32_t kTryDropInventoryItemAddr = 0x004674U;

inline constexpr std::uint32_t kCursorMoveSfxCmd = 0x006003A6U;
inline constexpr std::uint8_t kMinDroppableItemId = 2U;
inline constexpr std::uint8_t kDropFlagMask = 0x80U;
inline constexpr std::size_t kRingInventorySlotCount = 8U;

inline constexpr std::uint32_t kRingInventoryRamAddr = 0x00FF0D80U;
inline constexpr std::uint32_t kProtectedSlotRamAddr = 0x00FF0DB0U;
inline constexpr std::uint32_t kDropItemIdRamAddr = 0x00FF1864U;
inline constexpr std::uint32_t kDropItemQtyRamAddr = 0x00FF1865U;

struct InventorySlotPair {
    std::uint8_t item_id{0U};
    std::uint8_t qty{0U};
};

struct InventoryDropResult {
    bool dropped{false};
    std::uint8_t drop_item_id{0U};  // item_id | 0x80 ($FF1864)
    std::uint8_t drop_item_qty{0U}; // qty ($FF1865)
};

/// Models 0x00462E..0x004670: wraps 4x2 cursor ((row << 2) | col) with
/// col_mask=3 and row_mask=1 (caller emits SFX 0x006003A6 on move).
[[nodiscard]] std::uint8_t step_cursor_4x2(
    std::uint8_t cursor,
    int dcol,
    int drow) noexcept;

/// Models 0x004674..0x0046E2: guards against cursor == protected_slot,
/// terrain_blocked, and item_id <= 1; clears slot and emits (item_id | 0x80, qty).
[[nodiscard]] InventoryDropResult try_drop_inventory_item(
    std::span<InventorySlotPair, kRingInventorySlotCount> inventory,
    std::uint8_t cursor,
    std::uint8_t protected_slot,
    bool terrain_blocked) noexcept;

} // namespace oasis::game::ui
