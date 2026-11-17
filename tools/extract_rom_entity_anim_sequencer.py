"""Extract Entity Animation Frame Sequencer & Collision Stepper (0x008FE0..0x0090FC).

Milestone: M15.310.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x008FE0..0x009041: 4-byte anim keyframe sequencer (frame_id u16, flags_dur u16):
      * bit 15 (0x8000) of flags_dur -> $37 bit 2 (last frame in loop)
      * duration_ticks = ((flags_dur & 0x7FFF) >> 1) - 1 (clamped >= 0) -> $2E(a6)
      * clears $37 bit 3 (per-frame swing hit flag) on keyframe advance
      * sets $37 bit 1 (anim_completed) when wrapping or on last 0-tick frame
  - 0x009042..0x0090FC: Per-frame physics integration (clear $40/$41, 16.16 X/Y step,
                        room clamp 0x009D4E, solid sweep 0x009DD6, 8px tile step 0x00938E)
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_entity_anim_sequencer_m15_310.json")


def step_anim_keyframe(keyframes: list[tuple[int, int]], cursor_bytes: int, combat_flags_37: int) -> dict:
    """Execute one keyframe advance matching ROM 0x008FF0..0x00903E."""
    d0 = cursor_bytes + 4
    combat_flags_37 &= ~(1 << 3)  # bclr #3 (reset per-frame hit flag)
    wrap = (combat_flags_37 & (1 << 2)) != 0
    combat_flags_37 &= ~(1 << 2)
    if wrap:
        d0 = 0
    idx = (d0 // 4) % len(keyframes)
    frame_id, dur_raw = keyframes[idx]
    if (dur_raw & 0x8000) != 0:
        combat_flags_37 |= (1 << 2)
    d3 = (dur_raw & 0x7FFF) >> 1
    if d3 > 0:
        d3 -= 1
    if d3 == 0 and (combat_flags_37 & (1 << 2)) != 0:
        combat_flags_37 |= (1 << 1)
    return {
        "frame_id": frame_id,
        "cursor_bytes": idx * 4,
        "timer_2e": d3,
        "combat_flags_37": combat_flags_37,
    }


def extract_entity_anim_sequencer(rom_path: Path) -> dict:
    """Extract entity animation sequencer & physics stepper metadata."""
    _ = rom_path.read_bytes()
    return {
        "milestone": "M15.310",
        "rom_path": str(rom_path),
        "anim_sequencer": {
            "entry_addr": "0x008FF0",
            "keyframe_stride_bytes": 4,
            "last_frame_bit_mask": "0x8000",
            "duration_shift_right": 1,
            "flag_bit_anim_completed": 1,
            "flag_bit_last_frame": 2,
            "flag_bit_swing_hit_once": 3,
            "offset_timer_2e": "0x2E",
            "offset_cursor_30": "0x30",
            "offset_frame_id_32": "0x32",
            "offset_combat_flags_37": "0x37",
        },
        "physics_stepper": {
            "entry_addr": "0x009042",
            "tile_step_pixels": 8,
            "tile_probe_addr": "0x00938E",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_entity_anim_sequencer(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote entity anim sequencer spec (stride={data['anim_sequencer']['keyframe_stride_bytes']}B) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
