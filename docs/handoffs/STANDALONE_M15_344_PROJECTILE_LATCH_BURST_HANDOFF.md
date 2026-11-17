# Standalone Handoff — M15.344: Death Explosion Burst, Latch Follow & Projectile Probe (`0x00EA04..0x00EB40`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00EA04..0x00EA74`: `death_explosion_burst` (decrements `0x9E`, jitters `(x, y)` by `(prng & 0x1F) - 16`, plays `0x0090021B` + `0x00600382`, clears `0(a6)` at `0`)
  - `0x00EA76..0x00EABE`: `carried_latch_follow` (decrements `0xB8`, sets `(x, y, z) = parent + (0xAE, 0xB0, 0xB2)`)
  - `0x00EAC0..0x00EB40`: `projectile_wall_and_hit_probe` (terrain wall bounce SFX `0x00700378`, `±6`px hitbox probe via `0x00B922`, writes `0xAA(a6) -> 0x7E(a0)`)
- **Evidence JSON:** `data/thor-evidence/rom_projectile_latch_burst_m15_344.json`
- **Python Extractor & Tests:** `tools/extract_rom_projectile_latch_burst.py`, `tests/rom_projectile_latch_burst_test.py`
