# STANDALONE HANDOFF: Milestone M15.502 — ROM Companion Spirit VRAM Graphics Decompressor & DMA Uploader (`0x014522..0x014628`), Special Lock Reset & 4-Spirit Summon Dispatcher (`0x014628..0x0147E0`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x014522..0x0147E0` (702 bytes, 148 M68K instructions; contiguous executable code handling spirit VRAM tile loading, overlay state reset, and summoning setup for all 4 spirits).
- **Core Responsibilities:**
  1. `0x014522..0x014628` (`Spirits_DecompressAndQueueVramGfx`, 262 bytes, 58 insns):
     - Reads spirit type word at `$FF1AA4` and matches against the 4 companion spirit types:
       - `0x0018` (Dytto): Source `0x00141580`, Dest VRAM tile `0x04D0`.
       - `0x001A` (Efreet): Source `0x001425AE`, Dest VRAM tile `0x0450`.
       - `0x001C` (Shade): Source `0x00143262`, Dest VRAM tile `0x0280`.
       - `0x001E` (Bow): Source `0x00143A42`, Dest VRAM tile `0x0360`.
     - Invokes Nemesis decompressor `0x003820` (`a0 = src_rom, a1 = $FF2FA8`).
     - Queues an 8-byte VDP DMA descriptor under `ORI.W #$0700, SR`:
       - `move.l #$007F97D4, (a0)+` (DMA command / src word)
       - `move.w #$F500, (a0)+` (DMA length / control word)
       - `move.w #vram_tile, (a0)+` (destination VRAM tile index)
     - Updates queue pointer `$FF1892` and restores `SR` via `ANDI.W #$F9FF, SR`.
  2. `0x014628..0x014658` (`Spirits_ClearOverlayStateAndDispatch`, 48 bytes, 12 insns):
     - Resets hold counter `clr.w $FF199E` and double-tap timer `clr.b $FF19A1`.
     - Sets button A suppression sentinel `move.b #$FF, $FF19A0`.
     - Reads summon index `$FF19A6`:
       - Index `0` -> branches to `0x014658` (Dytto).
       - Index `1` -> branches to `0x0146C0` (Efreet).
       - Index `2` -> branches to `0x014732` (Shade).
       - Index `3` -> branches to `0x014798` (Bow).
       - Index `>= 4` -> returns via `rts` at `0x014656`.
  3. `0x014658..0x0146C0` (`Spirits_SpawnDytto`, 104 bytes, 22 insns):
     - Preserves coordinates from `+$08(a6)`, `+$0C(a6)`, `+$10(a6)`, `+$14(a6)`.
     - Sets `+$00(a6) = 0x0018` and calls `0x008D06` (entity template init).
     - Sets `+$16(a6) = 2`, `+$18(a6) = 0x04F8`, `+$88(a6) = 100`, `+$8A(a6) = 100`, `+$8C(a6) = 10`, `+$8E(a6) = 10`.
     - Restores preserved coordinates.
     - Sets State `$02` (`+$04(a6) = 2`, `+$2A(a6) = 4`).
     - Calls `0x014522` (VRAM art decompression and DMA queueing).
     - Clears spawn lock `sf.b $FF19A7`.
  4. `0x0146C0..0x014732` (`Spirits_SpawnEfreet`, 114 bytes, 24 insns):
     - Preserves coordinates, adding elevation offset `+$00080000` (+8.0 px fixed point) to Z (`+$10(a6)`).
     - Sets `+$00(a6) = 0x001A` and calls `0x008D06`.
     - Sets `+$18(a6) = 0x04F8`, `+$88(a6) = 100`, `+$8A(a6) = 100`, `+$8C(a6) = 10`, `+$8E(a6) = 10`.
     - Restores coordinates with elevated Z.
     - Sets State `$04` (`+$04(a6) = 4`, `+$2A(a6) = 4`, `+$16(a6) = 2`, `+$A6(a6) = a6`).
     - Calls `0x014522` and clears spawn lock `sf.b $FF19A7`.
  5. `0x014732..0x014798` (`Spirits_SpawnShade`, 102 bytes, 21 insns):
     - Preserves coordinates, sets `+$00(a6) = 0x001C`, and calls `0x008D06`.
     - Sets `+$18(a6) = 0x04F8`, `+$88(a6) = 100`, `+$8A(a6) = 100`, `+$8C(a6) = 10`, `+$8E(a6) = 10`.
     - Restores coordinates.
     - Sets State `$02` (`+$04(a6) = 2`, `+$2A(a6) = 0`, `+$16(a6) = 0`).
     - Calls `0x014522` and clears spawn lock `sf.b $FF19A7`.
  6. `0x014798..0x0147E0` (`Spirits_SpawnBow`, 72 bytes, 15 insns):
     - Sets `+$00(a6) = 0x001E` and immediately calls `0x014522` (art decompress & DMA).
     - Loads X (`+$08`), Y (`+$0C`), ground_z (`+$14`) into `d0..d2` and invokes bow root emerge subroutine at `0x0147E0`.
     - Sets State `$0E` (`+$04(a6) = $0E`, `+$2A(a6) = $1E`, `+$16(a6) = 0`, `+$18(a6) = 0x07A8`).
     - Clears status bit `bclr.b #0, +$36(a6)`.
     - Plays SFX `#$0080039F` via `0x00E7FC`.
     - Clears spawn lock `sf.b $FF19A7`.

## 2. ROM Evidence Reference
- Evidence JSON: `data/thor-evidence/rom_spirit_summon_vram_m15_502.json`
- Extractor Tool: `tools/extract_rom_spirit_summon_vram.py`
- Test Suite: `tests/rom_spirit_summon_vram_test.py`

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace Thor::Spirits {

inline constexpr uint16_t kSpiritTypeDytto       = 0x0018u;
inline constexpr uint16_t kSpiritTypeEfreet      = 0x001Au;
inline constexpr uint16_t kSpiritTypeShade       = 0x001Cu;
inline constexpr uint16_t kSpiritTypeBow         = 0x001Eu;

inline constexpr uint32_t kDmaCmdSrcWord         = 0x007F97D4u;
inline constexpr uint16_t kDmaLengthWord         = 0xF500u;

inline constexpr uint16_t kSpiritBaseVramAttr    = 0x04F8u;
inline constexpr uint16_t kBowSummonVramAttr     = 0x07A8u;
inline constexpr int32_t  kEfreetSpawnZOffset    = 0x00080000; // +8.0 px fixed point
inline constexpr uint32_t kBowSummonSfxId        = 0x0080039Fu;

struct SpiritVramDescriptor {
    uint16_t    spirit_type{0};
    uint32_t    rom_art_addr{0};
    uint16_t    vram_tile{0};
    uint16_t    dma_length{kDmaLengthWord};
    uint32_t    dma_cmd{kDmaCmdSrcWord};
};

inline constexpr std::array<SpiritVramDescriptor, 4> kSpiritVramDescriptors{{
    { kSpiritTypeDytto,  0x00141580u, 0x04D0u, kDmaLengthWord, kDmaCmdSrcWord },
    { kSpiritTypeEfreet, 0x001425AEu, 0x0450u, kDmaLengthWord, kDmaCmdSrcWord },
    { kSpiritTypeShade,  0x00143262u, 0x0280u, kDmaLengthWord, kDmaCmdSrcWord },
    { kSpiritTypeBow,    0x00143A42u, 0x0360u, kDmaLengthWord, kDmaCmdSrcWord },
}};

inline std::optional<SpiritVramDescriptor> LookupSpiritVramDescriptor(uint16_t type) noexcept {
    for (const auto& desc : kSpiritVramDescriptors) {
        if (desc.spirit_type == type) {
            return desc;
        }
    }
    return std::nullopt;
}

enum class SummonTarget : uint8_t {
    Dytto = 0,
    Efreet = 1,
    Shade = 2,
    Bow = 3,
    Invalid = 0xFF,
};

struct OverlayResetState {
    uint16_t hold_counter{0};
    uint8_t  dtap_timer{0};
    uint8_t  prev_button_a{0xFF};
};

inline SummonTarget ResetOverlayAndDispatchSummon(
    uint8_t summon_index_19a6,
    OverlayResetState& overlay) noexcept {
    overlay.hold_counter = 0;
    overlay.dtap_timer = 0;
    overlay.prev_button_a = 0xFF;

    switch (summon_index_19a6) {
        case 0: return SummonTarget::Dytto;
        case 1: return SummonTarget::Efreet;
        case 2: return SummonTarget::Shade;
        case 3: return SummonTarget::Bow;
        default: return SummonTarget::Invalid;
    }
}

} // namespace Thor::Spirits
```
