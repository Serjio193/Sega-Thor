# Standalone Handoff — M15.372: ROM Ali Jump Arc, Landing Impact & Command-Motion Buffer (`0x005DE4..0x005F6E`)

## ROM Source & Evidence
- ROM routine: `0x005DE4..0x005F6E`
- Evidence JSON: `data/thor-evidence/rom_ali_jump_landing_buffer_m15_372.json`
- Extractor: `tools/extract_rom_ali_jump_landing_buffer.py`
- Python tests: `tests/rom_ali_jump_landing_buffer_test.py`

## Exact Behavioral Contract
1. **Jump Launch & Air Physics (`0x005DE4..0x005E60`)**:
   - Initial jump impulse `vz = 0x50000` (`+0x56`), takeoff SFX `0x00800340`.
   - Gravity: `vz -= 0xC000`. When `vz <= 0`: sets `vz -= 1`, sets `cursor = 12`, `delay = 0` (apex animation transition).
   - Horizontal air velocity: `d1 = d2 = 0x180` projected along facing via `0x00E2D4` added to `+0x72/+0x76`.
2. **Terrain Landing Impact (`0x005E62..0x005F12`)**:
   - On ground impact (`z <= 0`): clamps `z = 0`, sets `cursor = 20`, `delay = 0`.
   - Checks pit vs platform (`0x009C40` / `0x009C7C`): if pit, triggers fall `state 0x3E` (`0x00B00213`/`0x00B00240`).
   - Normal landing: plays SFX `0x00500342` (or roll anim `0x4C` + SFX `0x00800203` if `+0x39` bit 1 set and `+0x9E != 4`).
3. **Landing Recovery & 5-Stage Special Command Buffer (`0x005F14..0x005F6E`)**:
   - When landing anim completes (`bit 1 of +0x37`):
     - Grants 12 frames invulnerability (`$FF198E = 12`), clears `bit 3 of +0x39`.
     - Evaluates 10-frame attack input buffer (`0x007D8A`).
     - Priority dispatch:
       1. `0x008814` (Chained F-B-F + 360 Spin) -> `0x00776C`
       2. `0x0086B4` (360 Spin) -> `0x00770C`
       3. `0x008778` (F-B-F Dash) -> `0x007742`
       4. `0x008910` (Rapid-Mash) -> `0x0077AE`
       5. `0x008950` (Triple-Tap) -> `0x0077CE`
       - Fallback: `0x0062CC` (idle / walk).
