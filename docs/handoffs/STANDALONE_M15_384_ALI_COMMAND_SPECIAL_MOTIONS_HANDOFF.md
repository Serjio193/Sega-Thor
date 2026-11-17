# Standalone Handoff — M15.384: ROM Ali Command Motions & Button-Mash Rapid Attack (`0x007656..0x00780E`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007656..0x0076C0`: `ali_motion_dispatcher` (checks recognizers in priority order: `0x8814` -> Dash Leap `state = 0x62, anim = 0x70, vz = 0x30000`, SFX `#$800220`; `0x86B4` -> Slide Attack `state = 0x40, anim = 0x46`, SFX `#$600343`; `0x8778` -> Somersault Flip `state = 0x56, anim = 0x64, vz = 0x84000`, SFX `#$800220`; `0x8910` -> Cartwheel `state = 0x72, anim = 0x7A`, SFX `#$80025B`; `0x8950` -> Grand Dash Strike `state = 0x74, anim = 0x80`, speed `0x600`, SFX `#$7004D3`; if none match, clears ring buffer `+$16EA = 0`, plays normal strike SFX `#$7004D3`, and triggers combo hit 1 `anim = 8` or hit 2 `anim = 0x3C`).
  - `0x0076C2..0x00770A`: `ali_button_mash_counter` (scans 8 frames in ring buffer `$FF1668`; when 8 or more Button A taps are detected, forces frame `+0x30 = 8` and clears `+0x2E = 0` to trigger rapid-fire combo flurry).
  - `0x00770C..0x007740`: `ali_motion_slide_attack` (slide state initialization: `state = 0x40, anim = 0x46`, 18-frame invulnerability timer `$FF198E = 0x12`, facing 2, SFX `#$600343`).
  - `0x007742..0x00776A`: `ali_motion_somersault_flip` (somersault state initialization: `state = 0x56, anim = 0x64`, vertical impulse `+0x56 = 0x00084000`, speed `(0x100, 0x100)`, SFX `#$800220`).
  - `0x00776C..0x0077AC`: `ali_motion_dash_leap` (dash leap state initialization: `state = 0x62, anim = 0x70`, vertical impulse `+0x56 = 0x00030000`, speed `(0x100, 0x100)`, SFX `#$800220`).
  - `0x0077AE..0x0077CC`: `ali_motion_cartwheel` (cartwheel state initialization: `state = 0x72, anim = 0x7A`, clears lock `$FF1991 = 0`, SFX `#$80025B`).
  - `0x0077CE..0x00780E`: `ali_motion_grand_dash_strike` (grand dash strike state initialization: `state = 0x74, anim = 0x80`, high speed `(0x600, 0x600)`, sets bit 2 of `+0x39`, attack property `+0x9A = 0x15`, SFX `#$7004D3`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_command_special_motions_m15_384.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_command_special_motions.py`, `tests/rom_ali_command_special_motions_test.py`
