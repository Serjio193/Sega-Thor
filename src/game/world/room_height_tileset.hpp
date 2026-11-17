#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game::world {

inline constexpr std::uint32_t kFlushVScrollToVdpBufferAddr = 0x00D344U;
inline constexpr std::uint32_t kSyncHvDirtyToVintAddr = 0x00D398U;
inline constexpr std::uint32_t kLoadSingleChunkDmaAddr = 0x00D3B2U;
inline constexpr std::uint32_t kInitRoomBandsAndChunksAddr = 0x00D406U;

inline constexpr std::uint16_t kHeightLutSentinel = 0xF000U;
inline constexpr std::size_t kHeightBandCount = 4U;
inline constexpr std::size_t kHeightLutEntries = 8U;
inline constexpr std::size_t kPlaneStateWords = 20U;
inline constexpr std::uint32_t kChunkTableRomAddr = 0x0005CE96U;
inline constexpr std::uint32_t kChunkRamBaseAddr = 0x00FF3FA8U;
inline constexpr std::uint32_t kChunkRamStrideBytes = 0x1000U;

struct HeightBandTables {
    std::array<std::int16_t, kHeightBandCount> band_steps_4w{};  // $FF16FE
    std::array<std::int16_t, kHeightLutEntries> height_lut_8w{}; // $FF1706
};

struct QuantizedCameraPos {
    std::int32_t x{0};
    std::int32_t y{0};
};

/// Models 0x00D43A..0x00D46A: builds 4-word band steps (band[i] << 3) and
/// 8-word cumulative/midpoint height LUT with 0xF000 sentinel at index 7.
[[nodiscard]] HeightBandTables build_height_band_tables(
    const std::array<std::int8_t, kHeightBandCount>& band_bytes) noexcept;

/// Models 0x00D48A..0x00D4C4: quantizes camera position to 8px tile grid
/// clamped >= 0: max(0, coord & ~7).
[[nodiscard]] QuantizedCameraPos quantize_camera_pos(
    std::int32_t x,
    std::int32_t y) noexcept;

/// Models 0x00D344..0x00D396: flushes vdp_vscroll to $FFFF5C (20-col pairs)
/// or $FF1348 (full-screen 2 words), offset +0 (Plane A) or +2 (Plane B).
void flush_vscroll_to_vdp_buffer(
    std::uint16_t vdp_vscroll,
    bool is_plane_b,
    bool per_col_mode,
    std::span<std::uint16_t, 40> vscroll_col_buf,
    std::array<std::uint16_t, 2>& vscroll_full_buf) noexcept;

} // namespace oasis::game::world
