#!/usr/bin/env python3
"""Extract and verify ROM enemy AI steering, fixed-point aim & Z-gravity physics (0x00E398..0x00E520)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ai_steering_gravity_m15_336.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00E39A) == 0x00FF19F0  # player X
    assert u32(rom, 0x00E3A0) == 0x00FF19F4  # player Y
    assert u16(rom, 0x00E44E) == 0x0010      # min chase timer (16 frames)
    assert u16(rom, 0x00E4DE) == 0x0300      # launch X speed
    assert u16(rom, 0x00E4E2) == 0x02E0      # launch Y speed
    assert u32(rom, 0x00E4F6) == 0x00060000  # launch Z vel (+6.0 px/frame)
    assert u32(rom, 0x00E50C) == 0x0000C000  # Z gravity (0.75 px/frame^2)

    return {
        "milestone": "M15.336",
        "title": "ROM Enemy AI Steering, Fixed-Point Aim & Z-Gravity Physics",
        "routines": {
            "facing_and_manhattan_to_player": {"start": "0x00E398", "end": "0x00E3E6"},
            "steer_4way_toward_player": {"start": "0x00E3E8", "end": "0x00E45C"},
            "aim_fixed_point_vector": {"start": "0x00E45E", "end": "0x00E4CE"},
            "launch_knockback_arc": {"start": "0x00E4D0", "end": "0x00E500"},
            "step_z_gravity": {"start": "0x00E502", "end": "0x00E520"},
        },
        "constants": {
            "facing_up": 0,
            "facing_right": 1,
            "facing_down": 2,
            "facing_left": 3,
            "min_chase_timer_frames": 16,
            "launch_speed_x": "0x0300",
            "launch_speed_y": "0x02E0",
            "launch_z_vel_16_16": "0x00060000",
            "gravity_z_16_16": "0x0000C000",
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
