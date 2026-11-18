#include "game/render/bg_tile_dma.hpp"

#include "game/graphics_decompress.hpp"

namespace thor::render {

namespace {

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept {
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

} // namespace

std::uint32_t resolve_bg_tile_pattern_address(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index) noexcept {
    // 0x00D3BC: lea $5ce96.l, a0; lsl.w #2, d0; movea.l (a0, d0.w), a0
    const std::size_t entry_offset =
        static_cast<std::size_t>(kBgTilePatternTableAddr) +
        (static_cast<std::size_t>(pattern_index) << 2U);
    if (entry_offset > rom.size() || rom.size() - entry_offset < 4U) {
        return 0U;
    }
    return read_be32(rom, entry_offset);
}

std::size_t decompress_bg_tile_pattern(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index,
    std::span<std::uint8_t> dest_buffer) noexcept {
    const std::uint32_t rom_addr = resolve_bg_tile_pattern_address(rom, pattern_index);
    if (rom_addr == 0U || static_cast<std::size_t>(rom_addr) >= rom.size() || dest_buffer.empty()) {
        return 0U;
    }

    try {
        // 0x00D3C8: jsr $3820.l (a0 = ROM stream, a1 = dest RAM $FF2FA8)
        const auto result = oasis::game::decompress_graphics(
            rom.subspan(static_cast<std::size_t>(rom_addr)),
            dest_buffer);
        return result.output_size;
    } catch (...) {
        return 0U;
    }
}

void queue_bg_tile_pattern_dma(
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint16_t vram_dest,
    std::uint8_t& vint_sync_flags) {
    // 0x00D3D2..0x00D3E8: append (0x007F97D4, d1.w, 0x0800) at ($FF1892) and advance by 8
    dma_queue.push_back(BgTileDmaCommand{
        kBgTileDmaBusSource,
        vram_dest,
        kBgTileDmaLengthWords,
    });
    // 0x00D3EE: bset.b #1, $ff164e.l
    vint_sync_flags = static_cast<std::uint8_t>(vint_sync_flags | kBgTileDmaSyncBit);
}

std::size_t decompress_and_dma_bg_tiles(
    std::span<const std::uint8_t> rom,
    std::uint16_t pattern_index,
    std::uint16_t vram_dest,
    std::span<std::uint8_t> decomp_buffer,
    std::vector<BgTileDmaCommand>& dma_queue,
    std::uint8_t& vint_sync_flags,
    const VintSyncCallback& vint_callback) {
    const std::size_t output_bytes = decompress_bg_tile_pattern(rom, pattern_index, decomp_buffer);
    if (output_bytes == 0U) {
        return 0U;
    }

    queue_bg_tile_pattern_dma(dma_queue, vram_dest, vint_sync_flags);

    // 0x00D3F6..0x00D3FE: spin-wait btst.b #1, $ff164e.l until V-Int handler clears bit 1
    if (vint_callback) {
        vint_callback(vint_sync_flags);
    }

    return output_bytes;
}

std::size_t decompress_and_dma_bg_tiles_from_ram(
    std::span<const std::uint8_t> rom,
    std::span<std::uint8_t, 0x10000> ram,
    std::uint16_t pattern_index,
    std::uint16_t vram_dest,
    bool clear_vint_sync) noexcept {
    // 0x00D3D2: movea.l $ff1892.l, a0; guard 8-byte write within 64KB RAM window
    const std::uint32_t queue_ptr = read_be32(ram, kDmaQueuePtrRamOffset);
    const std::size_t queue_offset = static_cast<std::size_t>(queue_ptr & 0xFFFFU);
    if (queue_offset > ram.size() - kDmaQueueEntrySizeBytes) {
        return 0U;
    }

    // 0x00D3B6..0x00D3CC: decompress ROM pattern into $FF2FA8 (4096-byte staging buffer)
    const std::size_t output_bytes = decompress_bg_tile_pattern(
        rom,
        pattern_index,
        ram.subspan(kBgTileDecompressBufferRamOffset, kBgTileDmaByteSize));
    if (output_bytes == 0U) {
        return 0U;
    }

    // 0x00D3D8..0x00D3E8: write 8-byte BE DMA descriptor and advance $FF1892 by 8
    write_be32(ram, queue_offset + 0U, kBgTileDmaBusSource);
    write_be16(ram, queue_offset + 4U, vram_dest);
    write_be16(ram, queue_offset + 6U, kBgTileDmaLengthWords);
    write_be32(ram, kDmaQueuePtrRamOffset, queue_ptr + static_cast<std::uint32_t>(kDmaQueueEntrySizeBytes));

    // 0x00D3EE..0x00D3FE: bset.b #1, $ff164e.l and optional V-Int clear
    ram[kVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(ram[kVintSyncFlagsRamOffset] | kBgTileDmaSyncBit);
    if (clear_vint_sync) {
        ram[kVintSyncFlagsRamOffset] = static_cast<std::uint8_t>(
            ram[kVintSyncFlagsRamOffset] & static_cast<std::uint8_t>(~kBgTileDmaSyncBit));
    }

    return output_bytes;
}

} // namespace thor::render
