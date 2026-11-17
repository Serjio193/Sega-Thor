#pragma once

#include "game/ui/ring_menu_drop.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::ui {

inline constexpr std::uint32_t kFoodMenuLoopAddr = 0x00474EU;
inline constexpr std::uint32_t kUseFoodItemAddr = 0x00482AU;
inline constexpr std::uint32_t kDropFoodItemAddr = 0x004896U;
inline constexpr std::uint32_t kStepCursor4x4Addr = 0x004906U;

inline constexpr std::uint32_t kFoodCursorRamAddr = 0x00FF0E51U;
inline constexpr std::uint32_t kFoodInventoryRamAddr = 0x00FF0D90U;
inline constexpr std::uint32_t kItemFlagsTableRomAddr = 0x0005CE16U;
inline constexpr std::uint32_t kConsumeCheckRoutineAddr = 0x00005236U;
inline constexpr std::uint32_t kFoodConsumeTimerRamAddr = 0x00FF1856U;

inline constexpr std::uint32_t kFoodConsumeSfxCmd = 0x00A00256U;
inline constexpr std::uint16_t kFoodConsumeTimerFrames = 90U; // 0x005A
inline constexpr std::uint8_t kFoodUndroppableMask = 0x20U; // bit 5
inline constexpr std::size_t kFoodInventorySlotCount = 16U;

/// Models 0x004906..0x00494A: wraps 4x4 cursor ((row << 2) | col) with
/// col_mask=3 and row_mask=3.
[[nodiscard]] std::uint8_t step_cursor_4x4(
    std::uint8_t cursor,
    int dcol,
    int drow) noexcept;

/// Models 0x004896..0x004904: checks terrain_blocked, item_id != 0, and
/// bit 5 undroppable flag from 0x05CE16; clears slot and emits (item_id | 0x80, qty).
[[nodiscard]] InventoryDropResult try_drop_food_item(
    std::span<InventorySlotPair, kFoodInventorySlotCount> inventory,
    std::uint8_t cursor,
    std::span<const std::uint8_t> item_flags_table,
    bool terrain_blocked) noexcept;

} // namespace oasis::game::ui
