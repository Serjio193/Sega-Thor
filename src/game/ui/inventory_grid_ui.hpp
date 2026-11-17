#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace oasis::game::ui {

inline constexpr std::uint32_t kGridSlotToXyAddr = 0x0042A6U;
inline constexpr std::uint32_t kRenderInventoryGridAddr = 0x0042E8U;

inline constexpr std::int16_t kGridOriginX = 0x0070; // 112
inline constexpr std::int16_t kGridOriginY = 0x0040; // 64
inline constexpr std::int16_t kGridCellPitchPx = 24;
inline constexpr std::uint8_t kGridCols = 4U;
inline constexpr std::size_t kInventoryGridSlots = 8U;
inline constexpr std::uint16_t kIconTileStride = 9U;
inline constexpr std::int16_t kIconYBias = 0x18; // 24

inline constexpr std::uint32_t kIconRomBase = 0x00155D76U;
inline constexpr std::uint32_t kInventoryRamAddr = 0x00FF0D80U;
inline constexpr std::uint32_t kSpiritSlotRamAddr = 0x00FF0E50U;
inline constexpr std::uint32_t kSpecialSlotRamAddr = 0x00FF0DB0U;
inline constexpr std::uint32_t kGridBufferRamAddr = 0x00FF2FA8U;

inline constexpr std::uint16_t kSpiritBufferSlot = 0x0020U;
inline constexpr std::uint16_t kSpecialBufferSlot = 0x0029U;

struct GridSlotCoord {
    std::int16_t x{0};
    std::int16_t y{0};
};

struct InventoryGridIconEntry {
    std::uint8_t slot{0};
    std::int16_t x{0};
    std::int16_t y{0};
    std::uint16_t item_id{0};
    std::uint16_t tile_index{0};
    std::uint32_t rom_byte_addr{0};
    std::uint16_t buffer_slot{0};
};

/// Models 0x0042A6..0x0042D0: converts 4x4 grid slot index (0..15) to screen
/// coordinates (0x70 + (slot & 3) * 24, 0x40 + ((slot & 0xC) >> 2) * 24).
[[nodiscard]] GridSlotCoord grid_slot_to_xy(std::uint8_t slot) noexcept;

/// Models 0x0042E8..0x004360: builds icon layout entries for the 8 inventory
/// slots ($FF0D80, +0x18 Y bias, 9-tile stride) plus spirit ($FF0E50 -> 0x20)
/// and special ($FF0DB0 -> 0x29) slots in $FF2FA8.
[[nodiscard]] std::vector<InventoryGridIconEntry> build_inventory_grid_icon_plan(
    const std::array<std::uint16_t, kInventoryGridSlots>& inventory_slots,
    std::uint16_t spirit_slot,
    std::uint16_t special_slot);

} // namespace oasis::game::ui
