#!/usr/bin/env python3
"""Extract and verify ROM camera deadzone follow, screen shake & HUD slide trigger (0x00CEEA..0x00D114)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_camera_follow_m15_319.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00CEEC) == 0x00FF19F0  # player_x
    assert u32(rom, 0x00CEF2) == 0x00FF19F4  # player_y
    assert u32(rom, 0x00CF02) == 0x00FF19F8  # player_z
    assert u32(rom, 0x00CF10) == 0x00FF17BA  # instant_snap_flag
    assert u32(rom, 0x00CF2A) == 0x00FF1859  # hud_slide_dir
    assert u16(rom, 0x00CF34) == 0x0050      # hud_slide_left_threshold (80)
    assert u16(rom, 0x00CF48) == 0x0138      # hud_slide_rev_snap
    assert u16(rom, 0x00CF54) == 0x00F0      # hud_slide_right_threshold (240)
    assert u16(rom, 0x00CF68) == 0x0020      # hud_slide_fwd_snap
    assert u16(rom, 0x00CF70) == 0x0078      # deadzone_x_min (120)
    assert u16(rom, 0x00CFAA) == 0x00C8      # deadzone_x_max (200)
    assert u16(rom, 0x00CFEC) == 0x0068      # deadzone_y_min (104)
    assert u16(rom, 0x00D02E) == 0x0080      # deadzone_y_max (128)
    assert u32(rom, 0x00CFDC) == 0x00FF177A  # cam_max_x
    assert u32(rom, 0x00D05E) == 0x00FF177C  # cam_max_y
    assert u32(rom, 0x00D06A) == 0x00FF17C4  # shake_timer
    assert u16(rom, 0x00D0D4) == 0x00A0      # locked_center_x (160)
    assert u16(rom, 0x00D0DC) == 0x0078      # locked_center_y (120)

    return {
        "milestone": "M15.319",
        "title": "ROM Camera Deadzone Follow, Screen Shake & HUD Slide Trigger",
        "routine": {"start": "0x00CEEA", "end": "0x00D114"},
        "constants": {
            "deadzone_x_min": 0x78,
            "deadzone_x_max": 0xC8,
            "deadzone_y_min": 0x68,
            "deadzone_y_max": 0x80,
            "hud_slide_left_threshold": 0x50,
            "hud_slide_right_threshold": 0xF0,
            "hud_slide_rev_snap": "0x0138",
            "hud_slide_fwd_snap": "0x0020",
            "smooth_step_px": 3,
            "catchup_threshold_px": 0x20,
            "catchup_bias_px": 0x1E,
            "catchup_up_min_delta": -6,
            "locked_center_x": 0xA0,
            "locked_center_y": 0x78,
        },
        "ram_state": {
            "player_x": "0x00FF19F0",
            "player_y": "0x00FF19F4",
            "player_z": "0x00FF19F8",
            "cam_max_x": "0x00FF177A",
            "cam_max_y": "0x00FF177C",
            "instant_snap_flag": "0x00FF17BA",
            "shake_timer": "0x00FF17C4",
            "hud_slide_dir": "0x00FF1859",
            "hud_slide_offset": "0x00FF185A",
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
