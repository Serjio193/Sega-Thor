# Standalone Handoff: M15.308 — 16-Bit LFSR PRNG (`0x00E268`..`0x00E2D3`, Seed `0x7F28`)

## Summary
Beyond Oasis uses a deterministic 16-bit LFSR stored in `$FF1928` (initialized to `0x7F28` at `0x000424` and `0x008A62`):
- **`0x00E268` (`next_rng16`):** Runs 4 iterations of `rol16 = ((d0 << 2) | (d0 >> 14)) & 0xFFFF; x = (rol16 ^ d0) & 1; d0 = ((d0 << 1) | x) & 0xFFFF;`.
- **`0x00E2A2` (`rand_u16(bound)`):** Returns `0` if `bound == 0`, else `((uint32_t)next_rng16() * bound) >> 16`.
- **`0x00E2BA` (`rand_u8(bound)`):** Same with `bound & 0xFF`.

## C++20 Standalone Target
- Implement `PrngLfsr16` in `src/game/math/` (or `src/game/`) matching the exact `0x7F28` seed and 4-iteration LFSR step.
- Evidence artifact: `data/thor-evidence/rom_prng_lfsr_m15_308.json`.
