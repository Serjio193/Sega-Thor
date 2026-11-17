# Standalone Handoff — M15.376: ROM Ali Crawl Movement, Multi-Surface Footstep SFX & Skid Deceleration (`0x006516..0x006700`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x006516..0x006616`: `ali_crawl_walk_and_surface_sfx` (perpendicular D-pad crawl nudges `±0x2A000` X / `±0x25800` Y, crawl timer `$FF197E` increment, water footstep SFX `0x02400357`/`0x02400358`, dust `0x0040038E`/`0x0040038F`, creep `0x01500218`).
  - `0x006618..0x0066A8`: `ali_skid_stop_friction` (`$FF197E >= 6` gate, state `0x0C`, anim `0x0C`, 21-frame dust timer `0x9A=0x15`, scrape SFX `0x1B400344`, friction decel `0x0000ACCC` towards zero on X and Y).
  - `0x0066AA..0x006700`: `ali_combat_slide_decel` (tackle hitbox damage `0x00BCBA(4)`, slide SFX `0x1B400344`, impact SFX `0x00600343` at frame 8 / timer 1, friction decel `0x00008000` towards zero).
- **Evidence JSON:** `data/thor-evidence/rom_ali_crawl_skid_m15_376.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_crawl_skid.py`, `tests/rom_ali_crawl_skid_test.py`
