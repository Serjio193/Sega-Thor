# Standalone Handoff: M15.310 — Entity Animation Frame Sequencer (`0x008FE0`..`0x0090FC`)

## Summary
- **4-Byte Keyframe Sequencer (`0x008FF0..0x009041`):** Each keyframe is `(frame_id: u16, flags_dur: u16)`. Bit 15 (`0x8000`) marks the last frame (`$37` bit 2), `timer_2e = max(0, ((flags_dur & 0x7FFF) >> 1) - 1)`, clears `$37` bit 3 (per-frame hit flag), and sets `$37` bit 1 (`anim_completed`) on last-frame wrap.
- **Physics Stepper (`0x009042`):** Clears `$40/$41`, integrates `16.16` velocity, runs room clamp (`0x009D4E`), solid sweep (`0x009DD6`), and 8px-step tile collision (`0x00938E`).

## C++20 Standalone Target
- Implement `AnimKeyframe4` & `step_anim_keyframe()` in `src/game/entities/`.
- Evidence artifact: `data/thor-evidence/rom_entity_anim_sequencer_m15_310.json`.
