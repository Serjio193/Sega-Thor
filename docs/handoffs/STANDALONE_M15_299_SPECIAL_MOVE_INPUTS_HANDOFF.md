# Standalone Handoff: M15.299 — Ali Command-Input Special Moves (`0x0086B4`, `0x008778`)

## Summary
Using the 64-frame reverse-indexed input history ring (`$FF1668`, M15.296), Ali detects two signature command-input special moves:
1. **360° Spin Slash (`0x0086B4`):** Scans up to `30` frames backwards, maps D-pad nibble (`& 0x0F`) through `0x008768` (`1->0, 8->2, 2->4, 4->6, diagonals odd/ignored, neutral 0xFF`), matches 4 consecutive cardinal steps in CW (`+2 mod 8`) or CCW (`-2 mod 8`) order, and clears `$FF16EA` on match.
2. **Forward-Back-Forward Lunge (`0x008778`):** Scans `$FF1668` against the facing's 4-byte sequence & mask (`0x0087F4`: Up `1,2,1,0`, Right `8,4,8,0`, Down `2,1,2,0`, Left `4,8,4,0`), aborting if a perpendicular D-pad input occurs, and clearing `$FF16EA` on match.

## C++20 Standalone Target
- Implement `detect_spin_360(InputHistoryRing64&)` and `detect_fbf_lunge(InputHistoryRing64&, uint8_t facing)` in `src/game/player/`.
- Evidence artifact: `data/thor-evidence/rom_special_move_inputs_m15_299.json`.
