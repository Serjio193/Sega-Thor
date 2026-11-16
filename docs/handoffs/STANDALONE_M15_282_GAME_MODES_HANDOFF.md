# Standalone Handoff: M15.282 — Top-Level Game Mode State Machine (`0x00034E`..`0x000471`)

## Summary
After ROM checksum verification (`0x00034E`), WRAM clear (`$FF0BFE`, `0x11D5` words at `0x0003E6`), and Z80 sound init (`0x060008` at `0x000412`), Beyond Oasis enters its top-level mode loop at `0x00042C`. Pending transitions in `$FF10AC` (when `!= 0xFFFF`) latch into `$FF10AE` and dispatch through the 5-entry `u32` pointer table at `0x00045E` (`mode & 0x7C`).

## Game Modes (`0x00045E`)
| Mode | Handler | Role |
|---:|---|---|
| `0x00` | `0x00307A` | Sega logo, `$A10001` region check, `$FF10B0` timer / Start+A skip -> `0x04` |
| `0x04` | `0x0032F8` | Title screen & save-file select (loads font `0x16943C` to VRAM `0xF020`) -> `0x08`/`0x0C` |
| `0x08` | `0x0089B2` | Main gameplay loop (new/continue) |
| `0x0C` | `0x0089B2` | Main gameplay loop (alternate entry) |
| `0x10` | `0x03A748` | Ending & credits sequence (fade to `0x0EEE`, clear Plane A/B, epilogue text) |

## C++20 Standalone Target
- Wire `GameModeStateMachine` (`0x00, 0x04, 0x08, 0x0C, 0x10`) in `src/game/` matching `$FF10AC` / `$FF10AE` latch semantics.
- Evidence artifact: `data/thor-evidence/rom_game_modes_m15_282.json`.
