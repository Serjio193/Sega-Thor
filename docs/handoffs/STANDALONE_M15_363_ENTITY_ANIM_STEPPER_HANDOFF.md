# Standalone Handoff — M15.363: ROM Entity Animation Keyframe Stepper & Velocity Damper (`0x008F12..0x00904A`)

## ROM Source & Evidence
- ROM routine: `0x008F12..0x00904A`
- Evidence JSON: `data/thor-evidence/rom_entity_anim_stepper_m15_363.json`
- Extractor: `tools/extract_rom_entity_anim_stepper.py`
- Python tests: `tests/rom_entity_anim_stepper_test.py`

## Exact Behavioral Contract
1. **Hitstop & Time-Stop / Slow-Mo Velocity Damper (`0x008F22..0x008FA2`)**:
   - Reads `u16(entity+0x9C)`: if low byte nonzero, decrements it into `+0x9D` and zeroes `vx/vy` (`+0x72/+0x76`).
   - If `$FF17B8` bit 1 (time-stop) is set and `type > 2`, zeroes `vx/vy` and exits movement/anim.
   - If `d3 != 0` or (`type > 2` and `$FF17B8` bit 0 slow-mo set), halves `vx` and `vy` (`>> 1`).
2. **Animation Keyframe Sequence Stepper (`0x008FA4..0x009040`)**:
   - Skips if `bit 0` of `+0x37` is set. Clears `bit 1` (end-of-sequence pulse) each tick.
   - When `anim_id (+0x2A) != cached (+0x2C)`: resets `cached = anim_id`, `cursor = -4`, `delay = 0`, clears `bit 2`.
   - Reads `4B` `[u16 frame_id, u16 raw_dur]` from `*(u32*)(entity+0x26) + anim_id + *(i16*)(table + anim_id)`.
   - `bit 15` of `raw_dur` sets `bit 2` of `+0x37` (last frame, wraps `cursor` to `0` on next advance).
   - `delay = (raw_dur & 0x7FFF) >> 1`; if `> 0`, `delay -= 1`; if `delay == 0` and `bit 2` set, pulses `bit 1` of `+0x37`.
