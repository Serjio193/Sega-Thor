# Standalone Handoff — M15.362: ROM CRAM Palette Animator & 3-Pool AI Dispatcher (`0x008E22..0x008F10`)

## ROM Source & Evidence
- ROM routines:
  - `0x008E22..0x008E8E`: Linked-list CRAM palette animator
  - `0x008E90..0x008EF6`: 3-pool AI/script dispatcher
  - `0x008EF8..0x008F10`: Entity script/anim state setter
- Evidence JSON: `data/thor-evidence/rom_palette_anim_pool_dispatch_m15_362.json`
- Extractor: `tools/extract_rom_palette_anim_pool_dispatch.py`
- Python tests: `tests/rom_palette_anim_pool_dispatch_test.py`

## Exact Behavioral Contract
1. **CRAM Palette Animator (`0x008E22..0x008E8E`)**:
   - Returns immediately if `$FF164F == 0`.
   - Walks linked list starting at `*(u32*)$FF17A2`; each node has `+0x00 next_delta (i16)`, `+0x02 cram_offset (u16)`, `+0x04 cursor (u16)`, `+0x06 delay_timer (u16)`, and `4B` `[delay, color]` keyframes starting at `+0x08` terminated by `delay == 0`.
   - Decrements `delay_timer` (triggers advance when `1` or `2`, otherwise stores `delay_timer - 2`). On advance, steps `cursor += 4` (wrapping to `8` at sentinel), writes `color` to `$FF134C + cram_offset`, and sets dirty bit 0 of `$FF164D`.
2. **3-Pool AI/Script Dispatcher (`0x008E90..0x008EF6`)**:
   - Iterates `$FF2954` (4x `0x5A` -> `0x008F12`), `$FF19E8` (21x `0xBC` -> `0x008F22`), and `$FF2D8C` (6x `0x5A` -> `0x008F12`) for active slots (`type > 0`), storing loop-continue PC in `$FF193C`.
3. **Anim State Setter (`0x008EF8`)**:
   - Sets `+0x2A = id`, `+0x2C = -1`, clears bits 1..2 of `+0x37`, and clears `+0x2E` and `+0x30`.
