# STANDALONE HANDOFF: M15.438 — ROM Background Tile Pattern Decompressor & 4KB VRAM DMA Streamer

## 1. Routine Overview
- **Addresses:** `0x00D3B2..0x00D406` (84 bytes)
- **Primary Entrypoint:**
  - `0x00D3B2`: `decompress_and_dma_bg_tiles` (Decompresses a background tile pattern from ROM pointer table `0x05CE96` into RAM buffer `$FF2FA8` via `0x003820`, enqueues 4KB DMA transfer to VRAM address `d1`, and awaits V-Int completion)

## 2. Calling Convention & Register Contract
- **Inputs:**
  - `d0.w`: Background tile pattern index (`0..19+`)
  - `d1.w`: Target VRAM base word address (e.g. `0x0000`, `0x2000`, `0x4000`)
- **Outputs:**
  - Decompressed pattern in RAM buffer `$FF2FA8`
  - 4KB transferred to target VRAM via hardware DMA
- **Preserved Registers:**
  - `d0`, `a0`, `a1` (saved via `movem.l d0/a0-a1, -(a7)` and restored before `rts`)

## 3. Tile Pattern Pointer Table (`0x05CE96`)
- Table of 32-bit big-endian ROM addresses pointing to compressed tile data blocks.
- Resolved via `address = read_be32(rom_data + 0x05CE96 + (index << 2))`.
- Sample ROM addresses:
  - Index 1: `0x1AD000`
  - Index 2: `0x1AD9D4`
  - Index 3: `0x1AE1A8`
  - Index 4: `0x1AE8AA`
  - Index 5: `0x1AF034`

## 4. VDP DMA Queue Packaging
- **Source Address:** `#$007F97D4` (`$FF2FA8 >> 1` in Mega Drive bus word addressing).
- **Destination:** `d1.w` (target VRAM address).
- **Length:** `0x0800` words (2048 words = 4096 bytes = 128 tiles of 32 bytes each).
- **Queue Pointer:** `$FF1892` advanced by 8 bytes.
- **Sync Handshake:** sets bit 1 on `$FF164E` (`bset.b #1, $FF164E`) and spin-waits until bit 1 clears during V-Int.

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::render {

constexpr uint32_t kBgTilePatternTableAddr = 0x05CE96;
constexpr uint32_t kBgTileDecompressBufferRam = 0x00FF2FA8;
constexpr uint32_t kBgTileDmaBusSource = 0x007F97D4;
constexpr uint16_t kBgTileDmaLengthWords = 0x0800; // 4096 bytes (128 tiles)

struct BgTileDmaCommand {
    uint32_t source_bus_addr{kBgTileDmaBusSource};
    uint16_t dest_vram{0};
    uint16_t length_words{kBgTileDmaLengthWords};
};

uint32_t resolve_bg_tile_pattern_address(
    std::span<const uint8_t> rom_bytes,
    uint16_t pattern_index);

void queue_bg_tile_pattern_dma(
    std::vector<BgTileDmaCommand>& dma_queue,
    uint16_t vram_dest);

} // namespace thor::render
```
