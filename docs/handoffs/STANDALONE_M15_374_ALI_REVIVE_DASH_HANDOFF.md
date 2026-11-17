# Standalone Handoff — M15.374: ROM Ali Auto-Revive Refill, Item 0x2A Scanner & 1.5x Dash Dispatcher (`0x00611E..0x0062CA`)

## ROM Source & Evidence
- ROM routine: `0x00611E..0x0062CA`
- Evidence JSON: `data/thor-evidence/rom_ali_revive_dash_m15_374.json`
- Extractor: `tools/extract_rom_ali_revive_dash.py`
- Python tests: `tests/rom_ali_revive_dash_test.py`

## Exact Behavioral Contract
1. **Revive Item 0x2A Scanner (`0x0061CC..0x0061F4`)**:
   - If `$FF0E40 == 0x10`, returns `false`.
   - Scans 16 entries in special inventory `$FF0D90` for item `0x2A`. If found, returns `true`.
2. **Auto-Revive Refill Sequence (`0x00611E..0x0061CA`)**:
   - Zeroes `vx/vy`.
   - Even frames: adds `3` to `HP`, clamped to `MaxHP`.
   - Odd frames: adds `3` to `SP`, clamped to `MaxSP`.
   - Plays refill chime `0x00C0034B` every other frame, toggles `$FF1857`.
   - Once `HP == MaxHP && SP == MaxSP`: clears `$FF1987 = 0`, sets standing state (`state 0x24 / anim 0x30`, or `0x1E / 0x2A` if anim was `0x6A`), clears `$FF1857`, plays revive jingle `0x00D00355`.
3. **Walk vs 1.5x Dash Dispatcher (`0x0061F6..0x0062CA`)**:
   - D-pad evaluation via `0x0085E2`. If none, drops to idle `0x0062CC`.
   - If double-tap dash active (`0x0083F8`):
     - `vx = vx + (vx >> 1)`, `vy = vy + (vy >> 1)` (exact 1.5x speed).
     - `state = 4`, `anim = 4`.
     - Plays water splash `0x02400357` (if `$FF16F0` bit 5 set) or dust dash `0x0040038E` (if dry land).
   - If normal walk: standard speed, `state = 2`, `anim = 2`.
   - If spirit shadow active: plays `0x01500218`.
