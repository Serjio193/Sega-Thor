# Standalone Handoff — M15.377: ROM Ali Cartwheel Evade Hitbox, Ledge-Catch Probe & Standup Sync (`0x006738..0x00693A`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x006738..0x0067DC`: `ali_cartwheel_evade_hitbox` (damage code `14`, boosted to `16` at keyframe `24`; spin SFX `0x1B400344` frames `8..20` on odd frame counter; contact check before keyframe `28` resets to keyframe `24`; linear friction `0x8000` towards zero).
  - `0x0067DE..0x0067FE`: `ali_ledge_hang_pullup_jump` (anim complete `$37 bit 1` sets `$FF17BB = 0xFF`, target position `0x00CA24`).
  - `0x006800..0x006874`: `ali_pit_edge_catch_hold` (`-0xC000` gravity, foot height probe at offset `+14` via `0x009C40`, on edge bit 7 transitions to state `0x32`).
  - `0x006876..0x00693A`: `ali_ledge_standup_spirit_sync` (clears `$FF1995`, sets state `0x34` with 20f recovery timer, syncs Shade spirit `$FF1AA4` follow flag, anim asset setup from `0x0003F4A6`, standup recovery SFX `0x0080037A`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_cartwheel_ledge_m15_377.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_cartwheel_ledge.py`, `tests/rom_ali_cartwheel_ledge_test.py`
