# Standalone Handoff: Milestone M15.469 — ROM Script Virtual Machine Opcodes $09, $0C, $0D, $1A..$1D: Media, Audio, Palette & Screen Shake Subsystem

**ROM Range:** `0x010034..0x010140` (268 bytes)  
**Status:** Reverse-engineered, verified with Capstone M68K disassembly, JSON evidence generated, and unit tests passing.

---

## 1. System Overview

This subsystem implements presentation, audio triggers, CRAM palette updates, screen shake effects, and channel transformations in the Cutscene & Event Script Virtual Machine:
1. **Opcode `$09` (`0x010034..0x010086`): Spawn Ground Item Drop:**
   - Allocates Pool 2 item slot via `$D9BE` (yields frame via `$F91C` on exhaustion).
   - Instantiates Type 8 item entity via constructor `$8D06`.
   - Reads `d1.w = (a0)+`: unpacks category byte `+$32 = d1 >> 8` and item ID `+$52 = d1 & $FF`.
   - Sets world coordinates `(x, y)` at `+$08/+$0C` and calls sprite helper `$9AD6` (saving VRAM tile to `+$14`).
   - Sets launch flag `+$39 |= 0x01`.
   - Computes random lifetime `0..63` frames (`$E268 & 0x3F`) stored to `+$02(a6)`.
2. **Opcode `$0C` (`0x010086..0x0100AA`): Load Palette Line to RAM Buffer:**
   - Reads palette index `d0 = (a0)+`, saves index to `$FF16F8`.
   - Copies 8 longwords (32 bytes) from ROM palette table `$5D046 + d0 * 32` into RAM palette buffer `$FF136C`.
3. **Opcode `$0D` (`0x0100AA..0x0100D6`): Load and Apply Palette Line to CRAM:**
   - Copies 32 bytes from `$5D046 + d0 * 32` into `$FF136C`.
   - Sets CRAM transfer request flag: `bset.b #0, $FF164D`.
4. **Opcode `$1A` (`0x0100D6..0x0100E4`): Play Sound Effect (SFX):**
   - Reads parameters `d1.w`, `d2.w`, and 32-bit sound ID `d0.l`.
   - Jumps directly to sound effect dispatcher `jmp $E78E.l`.
5. **Opcode `$1B` (`0x0100E4..0x0100EE`): Play Music Track / Command (BGM):**
   - Reads 32-bit sound driver command `d0.l`.
   - Jumps directly to Z80 communication port `jmp $60004.l`.
6. **Opcode `$1C` (`0x0100EE..0x0100F8`): Trigger Screen Shake / Quake:**
   - Reads 16-bit duration word and writes to screen shake counter: `move.w (a0)+, $FF17C4.l`.
7. **Opcode `$1D` (`0x0100F8..0x010140`): Dispatch Channel Plane Effect:**
   - Reads channel selector `d0.b = (a0)+`.
   - Selects channel structure: `$FF1716` for channels 0..2, `$FF173E` for channels 3..5.
   - Dispatches effect command to `$00DB64`, `$00DB68`, or `$00DBCA`.
   - Advances script PC by 12 bytes (`lea.l $C(a0), a0; rts`).

---

## 2. Memory & Architecture Reference

| Symbol | Address | Description |
|---|---|---|
| `ROM_PALETTE_TABLE` | `0x05D046` | Base table of 16-color 32-byte Mega Drive palettes |
| `RAM_PALETTE_BUFFER` | `$FF136C` | Active RAM palette staging line |
| `CRAM_TRANSFER_FLAG` | `$FF164D` | Bit 0: V-Int CRAM DMA transfer request |
| `SFX_DISPATCHER` | `0x00E78E` | Sound effect dispatcher |
| `Z80_DRIVER_PORT` | `0x060004` | Z80 sound driver command communication port |
| `SCREEN_SHAKE_TIMER` | `$FF17C4` | Screen shake frame countdown timer |
| `CHANNEL_EFFECT_A` | `$FF1716` | Background effect channel structure 0..2 |
| `CHANNEL_EFFECT_B` | `$FF173E` | Background effect channel structure 3..5 |

---

## 3. Cleanroom C++20 Interface

```cpp
namespace thor::script {

bool execute_opcode_09_spawn_item_drop(
    const uint8_t*& pc,
    uint8_t category,
    uint8_t item_id,
    int16_t x, int16_t y,
    uint8_t lifetime);

void execute_opcode_0c_load_palette(
    const uint8_t*& pc,
    uint16_t pal_id,
    std::span<const uint8_t> rom_pal_data,
    std::span<uint8_t, 32> ram_pal_buffer);

void execute_opcode_0d_load_and_apply_palette(
    const uint8_t*& pc,
    uint16_t pal_id,
    std::span<const uint8_t> rom_pal_data,
    std::span<uint8_t, 32> ram_pal_buffer,
    bool& cram_request_flag);

void execute_opcode_1a_play_sfx(
    const uint8_t*& pc,
    std::function<void(uint16_t d1, uint16_t d2, uint32_t sfx_id)> sfx_fn);

void execute_opcode_1b_play_bgm(
    const uint8_t*& pc,
    std::function<void(uint32_t bgm_cmd)> bgm_fn);

void execute_opcode_1c_screen_shake(
    const uint8_t*& pc,
    uint16_t& screen_shake_counter);

void execute_opcode_1d_channel_effect(
    const uint8_t*& pc,
    uint8_t channel_id,
    std::function<void(uint8_t ch_idx, uint8_t effect_mode)> dispatch_fn);

} // namespace thor::script
```
