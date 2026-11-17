# Standalone Handoff — M15.343: Explosion Spawner, 7-Frame SFX Cooldown & Shade Phase (`0x00E90E..0x00EA02`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00E90E..0x00E9DC`: `spawn_explosion_normal` / `spawn_explosion_hi` (entity type `0x34`, base ID `0x055E` / `0x855E`, 7-frame SFX cooldown between `$FF199C` and `$FF1944` for `0x0090021B` + `0x00600382`)
  - `0x00E9DE..0x00EA02`: `sync_shade_phase_flags` (sets/clears `0x36` bit 1 and `0x38` bit 2 when `$FF1AA4 == 0x1E`)
- **Evidence JSON:** `data/thor-evidence/rom_explosion_spawner_m15_343.json`
- **Python Extractor & Tests:** `tools/extract_rom_explosion_spawner_cooldown.py`, `tests/rom_explosion_spawner_cooldown_test.py`
