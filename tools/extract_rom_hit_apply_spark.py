#!/usr/bin/env python3
"""M15.355 extractor: ROM hit-apply, guard deflect, 4-way knockback & hit-spark (0x00BE98..0x00C0BC)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_hit_apply_spark_m15_355.json")

KNOCKBACK_PROFILES = [
    {"tier": 0, "target_addr": "0x00BF2E", "vx_mag": 0x300, "vy_mag": 0x2C0, "vz": 0x00000, "airborne_falls_through_to": 2},
    {"tier": 1, "target_addr": "0x00BF98", "vx_mag": 0x000, "vy_mag": 0x000, "vz": 0x70000},
    {"tier": 2, "target_addr": "0x00BF58", "vx_mag": 0x400, "vy_mag": 0x3C0, "vz": 0x28000},
    {"tier": 3, "target_addr": "0x00BF78", "vx_mag": 0x700, "vy_mag": 0x680, "vz": 0x30000},
]


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _i16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">h", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x00BE9A) == 0x00FF19E8
    assert _u16(rom_bytes, 0x00BEAC) == 0x13FC
    assert _u16(rom_bytes, 0x00BEAE) == 0x005A
    assert _u32(rom_bytes, 0x00C004) == 0x00900202
    assert _u32(rom_bytes, 0x00C014) == 0x00FF19C8

    jump_targets = [
        f"0x{0x00BF26 + i * 2 + _i16(rom_bytes, 0x00BF26 + i * 2):06X}"
        for i in range(4)
    ]
    assert jump_targets == [p["target_addr"] for p in KNOCKBACK_PROFILES]

    return {
        "milestone": "M15.355",
        "rom_range": "0x00BE98..0x00C0BC",
        "enemy_gauge_latch": {
            "timer_addr": "0xFF184F",
            "timer_frames": 0x5A,
            "target_ptr_addr": "0xFF1850",
            "skip_if_ali": "0xFF19E8",
            "hide_flag_offset": "0x3A",
            "hide_flag_bit": 0,
        },
        "guard_deflect": {
            "guard_flag_offset": "0x6D",
            "facing_offset": "0x16",
            "opposite_xor_mask": 2,
            "block_sfx_id": "0x00900202",
        },
        "knockback_jump_table": jump_targets,
        "knockback_profiles": KNOCKBACK_PROFILES,
        "hit_spark_pool": {
            "base_addr": "0xFF19C8",
            "slot_count": 4,
            "slot_stride_bytes": 8,
            "spark_type": 2,
            "spark_timer": 5,
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
