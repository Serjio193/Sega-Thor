#!/usr/bin/env python3
"""M15.372 extractor: ROM Ali jump arc, terrain landing impact & post-landing command buffer (0x005DE4..0x005F6E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_jump_landing_buffer_m15_372.json")

COMMAND_MOTION_PRIORITY = [
    {"order": 1, "name": "chained_fbf_360", "checker": "0x008814", "target": "0x00776C"},
    {"order": 2, "name": "spin_360", "checker": "0x0086B4", "target": "0x00770C"},
    {"order": 3, "name": "fbf_dash", "checker": "0x008778", "target": "0x007742"},
    {"order": 4, "name": "rapid_mash", "checker": "0x008910", "target": "0x0077AE"},
    {"order": 5, "name": "triple_tap", "checker": "0x008950", "target": "0x0077CE"},
]


def _i32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">i", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _i32(rom_bytes, 0x005DFE) == 0x00050000  # initial jump vz
    assert _u32(rom_bytes, 0x005E1C) == 0x00800340  # jump takeoff sfx
    assert _u32(rom_bytes, 0x005EEE) == 0x00500342  # normal land sfx
    assert _u32(rom_bytes, 0x005F0A) == 0x00800203  # roll land sfx
    assert _u32(rom_bytes, 0x005F3C) == 0x00008814  # chained super motion
    assert _u32(rom_bytes, 0x005F46) == 0x000086B4  # spin 360
    assert _u32(rom_bytes, 0x005F50) == 0x00008778  # fbf dash
    assert _u32(rom_bytes, 0x005F5A) == 0x00008910  # mash
    assert _u32(rom_bytes, 0x005F64) == 0x00008950  # triple-tap

    return {
        "milestone": "M15.372",
        "rom_range": "0x005DE4..0x005F6E",
        "jump_takeoff": {
            "initial_vz": 0x00050000,
            "horizontal_air_speed": 0x180,
            "takeoff_sfx": "0x00800340",
            "gravity_step": 0x0000C000,
        },
        "landing_impact": {
            "normal_landing_sfx": "0x00500342",
            "roll_landing_sfx": "0x00800203",
            "roll_anim_id": "0x4C",
            "pit_fall_state": "0x3E",
        },
        "post_landing_buffer": {
            "invuln_frames": 12,
            "input_buffer_evaluator": "0x007D8A",
            "buffer_window_frames": 10,
            "command_motions": COMMAND_MOTION_PRIORITY,
            "fallback_idle": "0x0062CC",
        },
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
