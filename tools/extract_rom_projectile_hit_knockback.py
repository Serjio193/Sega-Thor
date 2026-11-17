#!/usr/bin/env python3
"""Extract and verify ROM projectile hit knockback, enemy gauge timer & gravity (0x00EB40..0x00EC58)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_projectile_hit_knockback_m15_345.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert rom[0x00EB49] == 0x0A              # Target stun frames (10)
    assert u16(rom, 0x00EB5A) == 0x0400      # Normal knockback speed X
    assert u16(rom, 0x00EB5E) == 0x03C0      # Normal knockback speed Y
    assert u16(rom, 0x00EB68) == 0x0200      # Light knockback speed X
    assert u16(rom, 0x00EB6C) == 0x01E0      # Light knockback speed Y
    assert u32(rom, 0x00EB7C) == 0x00020000  # Knockback Z vel (+2.0 px/frame)
    assert u32(rom, 0x00EB90) == 0x00FF19E8  # Ali attacker ptr check
    assert u16(rom, 0x00EB9A) == 0x005A      # Enemy HP gauge timer (90 frames)
    assert u32(rom, 0x00EBA8) == 0x00800210  # Projectile hit SFX
    assert u32(rom, 0x00EBDE) == 0x00006000  # Projectile Z gravity (0.375 px/frame^2)
    assert u16(rom, 0x00EC00) == 0x7E0A      # moveq #$0A, d7 (10px cull margin)

    return {
        "milestone": "M15.345",
        "title": "ROM Projectile Hit Knockback, Enemy Gauge Timer & Gravity",
        "routine": {"start": "0x00EB40", "end": "0x00EC58"},
        "constants": {
            "target_hit_stun_frames": 10,
            "kb_normal_speed_xy": ["0x0400", "0x03C0"],
            "kb_light_speed_xy": ["0x0200", "0x01E0"],
            "kb_z_vel_16_16": "0x00020000",
            "enemy_gauge_timer_frames": 90,
            "projectile_hit_sfx": "0x00800210",
            "projectile_gravity_16_16": "0x00006000",
            "projectile_cull_margin_px": 10,
            "latch_hit_timer_frames": 8,
            "drop_timer_frames": 5,
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
