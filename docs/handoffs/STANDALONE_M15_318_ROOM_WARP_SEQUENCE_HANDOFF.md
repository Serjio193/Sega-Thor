# Standalone Handoff — M15.318: Room Update Hooks & Warp Transition Sequence (`0x00C980..0x00CB0A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C980..0x00C996`: `room_update_early` (runs `header[+0]` relative hook if `$FF17B8 & 2 == 0`)
  - `0x00C998..0x00C9B0`: `room_update_late` (runs `header[+2]` relative hook if `$FF17B8 & 2 == 0`)
  - `0x00C9B2..0x00C9EA`: `room_warp_guard` (requires `$FF17B9`, `$FF1987`, `$FF1985`, `$FF17B8 & 4`, `$FF1983`, `$FF1982` all zero)
  - `0x00C9EC..0x00CB0A`: `warp_door_fade` / `warp_instant_cut`
- **Evidence JSON:** `data/thor-evidence/rom_room_warp_sequence_m15_318.json`
- **Python Extractor & Tests:** `tools/extract_rom_room_warp_sequence.py`, `tests/rom_room_warp_sequence_test.py`

## Behavior
1. Camera origin is centered on spawn `(d3, d4)` as `(d3 - 160, d4 - 112)` (`0x00A0`, `0x0070`).
2. Room `0x0004` redirects to `0x0611`.
3. If target room header byte `+0x19` bit 6 or `$FF16F1` bit 6 is set, saves checkpoint snapshot (`0x0017DA`).
4. Resets warp/lock flags (`$FF17BA = 0xFF`, `$FF17B8 = $FF17B9 = $FF1988 = $FF1858 = 0`, `$FF177E = $FF1780 = 0`), sends sound command `2` to `0x00060004`, flushes DMA, and runs fade (`$FF1896 = 4`).
