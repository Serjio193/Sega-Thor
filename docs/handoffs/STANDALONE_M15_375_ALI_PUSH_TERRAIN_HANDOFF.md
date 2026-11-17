# Standalone Handoff — M15.375: ROM Ali Push Physics, Jewel Socket Inserter & Isometric Velocity Scaler (`0x0062E4..0x006514`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0062E4..0x006336`: `ali_walk_step_and_footstep_sfx` (integrates velocity, triggers footstep SFX `0x01500218` on walk cycle frame 8 / timer 1).
  - `0x006338..0x00638A`: `ali_push_trigger_and_state` (if pushing against entity `$6C(a6) != 0`, dispatches block push or jewel socket, sets push state `0x02`, anim `0x02`).
  - `0x00638C..0x0063D8`: `ali_push_heavy_block` (moves pushable block `0x46` or stone pillar `0x7E` at half speed `d1>>1, d2>>1`, plays scraping SFX `0x00800390` every 4 frames).
  - `0x0063DA..0x0064C2`: `ali_jewel_socket_insert` (inserts jewel `0x5A/0x5C/0x5E/0x60`, sets state `0x6A`, anim `0x36`, 60-frame toast, decrements jewel count, clears entity, plays socket SFX `0x00B00385`).
  - `0x0064C4..0x006514`: `scale_isometric_terrain_velocity` (Megadrive isometric 2:1 Y halving `d2 >>= 1`, slowdown halving `d1 >>= 1, d2 >>= 1` on `$FF1984 != 0`, slope climbing full speed on `$FF1985 != 0` or tile slope flags).
- **Evidence JSON:** `data/thor-evidence/rom_ali_push_terrain_m15_375.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_push_terrain.py`, `tests/rom_ali_push_terrain_test.py`
