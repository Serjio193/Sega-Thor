# Standalone Handoff: M15.286 — Ali Complete 59-State Table (`0x00..0x74`) & Death/Auto-Revive (`0x005F92`, `0x00611E`)

## Summary
Completes Ali's full `0x0059BA..0x005A2F` jump table (**59 states**, `0x00..0x74` in steps of 2), adding climbing (`0x28..0x2E`), push/pull (`0x32..0x34`), pit fall (`0x3E`), weapon/combo/bow states (`0x46..0x5C`), death (`0x5E` at `0x005F92`), game over (`0x60` at `0x005FEA`), item pickup (`0x62..0x68`), and auto-revive (`0x6C` at `0x00611E`).

## Death, Auto-Revive & Game Over (`0x005F92`, `0x005FEA`, `0x00611E`)
- **Death (`0x5E` -> `0x005F92`):** Once death anim finishes (`$37 & 2`), calls `0x0061CC` to check for an auto-revive item.
  - **If found (`CS`):** Consumes item slot (`clr.w (a0)`), sets active item `0x2A`, plays `0xD00355`, and enters **`0x6C` (`0x00611E`)**.
  - **Revive Refill (`0x6C` -> `0x00611E`):** Adds `+3` HP on even frames (`$FF1971 & 1 == 0`) and `+3` SP on odd frames (`$FF1971 & 1 != 0`), clamped to MaxHP/MaxSP, until full.
  - **If not found (`CC`):** Sets `$FF1992 = 0x28` (`40` frames) and advances to **`0x60` (`0x005FEA`)** Game Over fade.

## C++20 Standalone Target
- Extend `Player` state enum/table to all 59 states (`0x00..0x74`) and implement the `0x5E -> 0x6C` auto-revive `+3 HP / +3 SP` refill loop vs `0x60` game-over fade.
- Evidence artifact: `data/thor-evidence/rom_player_extended_states_m15_286.json`.
