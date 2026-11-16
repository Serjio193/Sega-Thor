# Standalone Handoff: M15.285 — Ali Player 20-State Action & Combat Machine (`0x005580`..`0x0059E2`)

## Summary
Ali's per-frame controller (`0x005580`, `a6 = $FF19E8`) updates low-HP/low-SP heartbeat/chime timers, tracks safe respawn coordinates (`$FF1972`), applies hazard damage (`$FF0DBA & 1`) and passive SP regeneration (`$FF1AA4 == 0` in water or with SP ring), and dispatches `state = +4(a6)` (`0x00..0x26`) via the 20-entry signed self-relative jump table at `0x0059BA`.

## Pre-Dispatch Tick Rules (`0x0055B0`..`0x0057AE`)
- **Low HP Warning (`0x0055B0`):** When `0 < HP <= 0x40`, plays `0xA0038C` every `(HP >> 1) + 10` frames (`$FF1993`).
- **Low SP Warning (`0x0055EE`):** When active spirit `$FF1AA4` is non-zero (and `!= 0x16`) and `SP <= 0x18`, plays `0xA004DD` every `SP + 12` frames (`$FF1994`).
- **Safe Respawn (`0x005638`):** Saves `(x, y)` to `$FF1972` whenever `state in {0x00, 0x02, 0x04}` and `!(+0x6F & 0x80)`.
- **Hazard Damage (`0x005718`):** When `state == 0`, `$FF0DBA & 1`, and `$FF16F0 & 0x20`, ticks every 16 frames (or 8 if `$FF0DBA & 8`).
- **Passive SP Regen (`0x005762`):** When `$FF1AA4 == 0` and (`$FF16F0 & 0x20` or `$FF0DBA & 4`), `+1 SP` every 8 frames (or 4 if `$FF0DBA & 8`).

## 20-State Jump Table (`0x0059BA`)
`0x00` idle (`0x0061F6`), `0x02` walk (`0x0062E4`), `0x04` run/dash (`0x006516`), `0x06` jump (`0x006DD2`), `0x08` hurt (`0x007852`), `0x0A` climb (`0x007656`), `0x0C` swim (`0x00663E`), `0x0E` fall/land (`0x0071DA`), `0x10` crouch (`0x006C46`), `0x12` crouch rise (`0x006C3A`), `0x14` spirit summon (`0x007A10`), `0x16` jump attack (`0x007112`), `0x18` stun (`0x005CC8`), `0x1A` primary attack (`0x005CF8`), `0x1C` attack hold (`0x005DC8`), `0x1E` follow-through (`0x005DE4`), `0x20` secondary attack (`0x005CF8`), `0x22` secondary hold (`0x005DC8`), `0x24` recovery (`0x005F70`), `0x26` aerial special (`0x006EDA`).

## C++20 Standalone Target
- Align `Player` state machine in `src/game/player/` with all 20 state IDs and pre-dispatch HP/SP warning & regen timers.
- Evidence artifact: `data/thor-evidence/rom_player_state_machine_m15_285.json`.
