# Standalone Handoff — M15.398: Universal Entity Simulation Machine, Animation Frame Stepper & Terrain Pre-flight (`0x008F22..0x009084`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x008F22..0x008F84`: `entity_sim_freeze_slowmo` (decrements sub-timers `+0x9C`/`+0x9D`; checks global freeze flag `$FF17B8` bit 1 [zeros velocities `+0x72`/`+0x76` for entity types > 2, skipping to `0x9382`]; checks slow-motion flag `$FF17B8` bit 0 [arithmetic shifts right `+0x72` and `+0x76` by 1 for entity types > 2])
  - `0x008F84..0x009040`: `anim_frame_stepper` (detects state changes `+0x2A` vs `+0x2C`; resets frame offset `+0x30 = -4` and keyframe timer `+0x2E = 0`; reads anim sequence pointer `+0x26(a6)`; updates keyframe timer; decodes sprite asset ID to `+0x32(a6)` and duration `(dur >> 1) - 1`; updates loop bit 15 into `+0x37(a6)`)
  - `0x009040..0x009084`: `terrain_collision_preflight` (clears collision flags `+0x40`/`+0x41`; checks intangible bit 4 on `+0x37`; selects terrain collision layer map from `$FF16F0` bit 7 [`$FF1766` Layer 1 vs `$FF1770` Layer 2]; checks flying bit 1 on `+0x38`; branches on X velocity: `vel_x == 0` -> `0x91F6`, `vel_x < 0` -> `0x9128`, `vel_x > 0` -> sets `+0x40 = 0x11` and steps +X boundary check)
- **Evidence JSON:** `data/thor-evidence/rom_entity_sim_anim_preflight_m15_398.json`
- **Python Extractor & Tests:** `tools/extract_rom_entity_sim_anim_preflight.py`, `tests/rom_entity_sim_anim_preflight_test.py`
