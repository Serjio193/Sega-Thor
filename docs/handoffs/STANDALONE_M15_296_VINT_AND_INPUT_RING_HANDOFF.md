# Standalone Handoff: M15.296 — V-Int DMA Queue & Input History Ring (`0x001F76`..`0x002218`)

## Summary
Beyond Oasis's V-Int (`0x001F76`, vector `0x000078`) executes four core per-frame services:
1. **DMA Queue (`0x002112`):** When `$FF164E` bit 1 is set, flushes 8-byte entries from `$FF1892` down to `$FF10B4` via `0x0027FC` (`src_words`, `vram_dst`, `word_len`).
2. **Controller Poll & Soft Reset (`0x00217C`):** Polls `$A10003` via `0x002992`; checks `$FF165E.L == 0xE0000202` for immediate soft reset (`0x00020E`).
3. **64-Frame Input Ring (`0x0021B2`):** Records `$FF165E.W` into `$FF1668` (`64` words, index `$FF16E8` stepped `-2 & 0x7E`, depth `$FF16EA` clamped to `0x3F`) for command-move detection.
4. **Frame Timers (`0x0021EC`):** Increments `$FF1656` and decrements `$FF1658` / `$FF1654` when non-zero.

## C++20 Standalone Target
- Implement `InputHistoryRing64` and `VdpDmaQueue` in `src/game/` matching the `64`-entry reverse-indexed ring and 8-byte DMA queue semantics.
- Evidence artifact: `data/thor-evidence/rom_vint_and_input_ring_m15_296.json`.
