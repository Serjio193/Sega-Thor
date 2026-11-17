#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::world {

inline constexpr std::uint32_t kRoomLayerLoaderAddr = 0x00D500U;
inline constexpr std::uint32_t kRoomRowShiftHelperAddr = 0x00D7B0U;
inline constexpr std::uint16_t kRoomTilesetDmaWords = 0x2000U; // 8192 words
inline constexpr std::uint32_t kRoomDecompRamDest = 0xFF3FA8U;
inline constexpr std::uint32_t kPlaneADescRam = 0xFF1716U;
inline constexpr std::uint32_t kPlaneAColDescRam = 0xFF1766U;
inline constexpr std::uint16_t kPlaneAVramAddr = 0xC000U;
inline constexpr std::uint32_t kPlaneBDescRam = 0xFF173EU;
inline constexpr std::uint32_t kPlaneBColDescRam = 0xFF1770U;
inline constexpr std::uint16_t kPlaneBVramAddr = 0xE000U;

struct RoomLayerSpec {
    std::uint16_t tiles_w{0};
    std::uint16_t tiles_h{0};
    std::uint8_t row_shift{0};
    std::uint32_t tilemap_offset{0};
    std::uint32_t tilemap_bytes{0};
    bool has_collision{false};
    std::uint32_t collision_offset{0};
    std::uint32_t collision_bytes{0};
};

struct RoomLayerLayout {
    std::uint16_t plane_a_vram{kPlaneAVramAddr};
    std::uint16_t plane_b_vram{kPlaneBVramAddr};
    std::uint16_t tileset_dma_words{kRoomTilesetDmaWords};
    RoomLayerSpec plane_a{};
    RoomLayerSpec plane_b{};
};

/// Models 0x00D7B0: ctz(tiles_w * 2) for power-of-two tilemap widths (0 if tiles_w == 0).
[[nodiscard]] std::uint8_t compute_row_shift(std::uint16_t tiles_w) noexcept;

/// Models 0x00D560..0x00D620: unpacks 4-byte decompressed room header into
/// Plane A and Plane B tilemap/collision byte offsets and dimensions.
[[nodiscard]] RoomLayerLayout decode_room_layer_header(
    const std::array<std::uint8_t, 4>& header4) noexcept;

} // namespace oasis::game::world
