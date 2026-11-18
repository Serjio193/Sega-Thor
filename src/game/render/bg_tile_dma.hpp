#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace thor::render {

inline constexpr std::uint32_t kDecompressAndDmaBgTilesAddr = 0x00D3B2U;
inline constexpr std::uint32_t kDecompressAndDmaBgTilesEndAddr = 0x00D406U;
inline constexpr std::uint32_t kBgTilePatternTableAddr = 0x05CE96U;
inline constexpr std::uint32_t kBgTileDecompressBufferRamAddr = 0x00FF2FA8U;
inline constexpr std::uint32_t kBgTileDecompressBufferRamOffset = 0x2FA8U;
inline constexpr std::uint32_t kBgTileDmaBusSource = 0x007F97D4U;
inline constexpr std::uint16_t kBgTileDmaLengthWords = 0x0800U; // 2048 words = 4096 bytes
inline constexpr std::size_t kBgTileDmaByteSize = 0x1000U;      // 4096 bytes = 128 8x8 4bpp tiles
inline constexpr std::uint32_t kDmaQueuePtrRamOffset = 0x1892U;
inline constexpr std::uint32_t kVintSyncFlagsRamOffset = 0x164EU;
inline constexpr std::uint8_t kBgTileDmaSyncBit = 0x02U; // bit 1 ($FF164E)
inline constexpr std::size_t kDmaQueueEntrySizeBytes = 8U;

/// 8-byte VDP DMA queue command enqueued by 0x00D3B2..0x00D406 at ($FF1892).
struct BgTileDmaCommand {
    std::uint32_t src_bus_addr{kBgTileDmaBusSource};
    std::uint16_t vram_dest{0U};
    std::uint16_t length_words{kBgTileDmaLengthWords};

    [[nodiscard]] constexpr bool operator==(const BgTileDmaCommand&) const noexcept = default;
};

using VintSyncCallback = std::function<void(std::uint8_t& vint_sync_flags)>;

/// Resolves big-endian 32-bit ROM stream address at kBgTilePatternTableAddr + (pattern_index << 2).
/// Returns 0 if the table entry is out of bounds or contains a null pointer.
[[nodiscard]] std::uint32_t resolve_bg_tile_pattern_address(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index) noexcept;

/// Resolves pattern_index in ROM and decompresses via 0x003820 into dest_buffer.
/// Returns decompressed byte count, or 0 if the address is null/out of bounds or decoding fails.
[[nodiscard]] std::size_t decompress_bg_tile_pattern(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index,
    std::span<std::uint8_t> dest_buffer) noexcept;

/// Appends BgTileDmaCommand{kBgTileDmaBusSource, vram_dest, kBgTileDmaLengthWords}
/// and sets kBgTileDmaSyncBit (bit 1) on vint_sync_flags.
void queue_bg_tile_pattern_dma(
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint16_t vram_dest,
    std::uint8_t& vint_sync_flags);

/// Models 0x00D3B2..0x00D406: decompresses pattern_index into decomp_buffer,
/// queues the 4KB VRAM DMA command, sets kBgTileDmaSyncBit on vint_sync_flags,
/// and optionally invokes vint_callback(vint_sync_flags) to simulate V-Int completion.
/// Returns decompressed byte count (0 if decompression fails; DMA is not queued on failure).
std::size_t decompress_and_dma_bg_tiles(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index,
    std::uint16_t vram_dest,
    std::span<std::uint8_t> decomp_buffer,
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback = {});

/// 64KB RAM window helper modeling 0x00D3B2..0x00D406 against Mega Drive Work RAM ($FF0000..$FFFFFF):
/// decompresses into ram[0x2FA8..0x3FA7], reads queue pointer at $FF1892 (low 16 bits, bounded to 0x10000 - 8),
/// writes 8-byte BE entry (0x007F97D4, vram_dest, 0x0800), advances $FF1892 by 8,
/// sets bit 1 on $FF164E, and clears bit 1 if clear_vint_sync is true.
/// Returns decompressed byte count, or 0 if decompression fails or queue pointer is out of bounds.
std::size_t decompress_and_dma_bg_tiles_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::uint16_t pattern_index,
    std::uint16_t vram_dest,
    bool clear_vint_sync = true) noexcept;

} // namespace thor::render

namespace oasis::game::render {
using namespace thor::render;
} // namespace oasis::game::render
