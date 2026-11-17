# Standalone Handoff — M15.345: Projectile Hit Knockback, Enemy Gauge Timer & Gravity (`0x00EB40..0x00EC58`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00EB40..0x00EC58`
- **Evidence JSON:** `data/thor-evidence/rom_projectile_hit_knockback_m15_345.json`
- **Python Extractor & Tests:** `tools/extract_rom_projectile_hit_knockback.py`, `tests/rom_projectile_hit_knockback_test.py`

## Constants
- Target stun: `10` frames; knockback Z velocity: `0x00020000` (`+2.0 px/frame`); XY speed `(0x0400, 0x03C0)` or `(0x0200, 0x01E0)` when light flag `0xA7 != 0`.
- Enemy HP gauge HUD timer (`$FF184F`): `90` frames (`0x5A`) when attacker is Ali (`0xFF19E8`).
- Projectile Z gravity: `0x00006000` (`0.375 px/frame^2`), viewport cull margin `10` px (`0x0A`), hit SFX `0x00800210`.
