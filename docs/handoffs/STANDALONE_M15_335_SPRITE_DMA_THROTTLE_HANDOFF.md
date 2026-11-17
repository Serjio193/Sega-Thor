# Standalone Handoff — M15.335: Entity Sprite-DMA Budget Throttle & Frame Lookup (`0x00A9A0..0x00AAA5`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00A9A0..0x00AAA5`
- **Evidence JSON:** `data/thor-evidence/rom_sprite_dma_throttle_m15_335.json`
- **Python Extractor & Tests:** `tools/extract_rom_sprite_dma_throttle.py`, `tests/rom_sprite_dma_throttle_test.py`

## Behavior
1. **DMA Budget Throttle (`0x00A9AE..0x00A9F8`):**
   - Computes target frame `(facing == 3 ? 1 : facing) + anim_frame`.
   - If `target != cached_frame`: if `cached_frame >= 0 && dma_cost > dma_budget ($FF1652)`, throttles back to `cached_frame` and sets backlog flag `$FF1651 = 0xFF`; else updates `cached_frame` and queues DMA.
2. **Shadow Visibility (`0x00AA78..0x00AAA5`):**
   - Draws shadow when `!(flags_36 & 8)` and `(z > 0 || (z == 0 && (flags_36 & 4)))`.
