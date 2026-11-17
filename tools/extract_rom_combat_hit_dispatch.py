#!/usr/bin/env python3
"""M15.354 extractor: ROM combat hitbox dispatch & Ali damage scaling (0x00BC8E..0x00BE96)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_combat_hit_dispatch_m15_354.json")

ENTRYPOINTS = [
    {"name": "scan_primary_pool", "addr": "0x00BC8E", "pool_base": "0xFF19E8", "slot_count": 21, "callback": "0x00BEBA", "sets_hit_marker": False},
    {"name": "scan_fx_pool", "addr": "0x00BCA4", "pool_base": "0xFF1CD8", "slot_count": 17, "callback": "0x00BE98", "sets_hit_marker": False},
    {"name": "scan_fx_pool_marked", "addr": "0x00BCBA", "pool_base": "0xFF1CD8", "slot_count": 17, "callback": "0x00BE98", "sets_hit_marker": True, "hit_marker_addr": "0xFF198B", "hit_marker_value": 2},
    {"name": "scan_ali_only", "addr": "0x00BCD8", "pool_base": "0xFF19E8", "slot_count": 1, "callback": "0x00BEBA", "sets_hit_marker": False},
]

ALI_KILL_THRESHOLDS = [
    {"min_kills": 10, "delta": 1},
    {"min_kills": 20, "delta": 1},
    {"min_kills": 30, "delta": 1},
    {"min_kills": 40, "delta": 1},
    {"min_kills": 50, "delta": 1},
    {"min_kills": 100, "delta": 2},
    {"min_kills": 200, "delta": 4},
]


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u16(rom_bytes, 0x00BC8E) == 0x23FC
    assert _u32(rom_bytes, 0x00BC90) == 0x0000BEBA
    assert _u32(rom_bytes, 0x00BCA6) == 0x0000BE98
    assert _u16(rom_bytes, 0x00BCBA) == 0x23FC
    assert _u16(rom_bytes, 0x00BCC4) == 0x13FC
    assert _u16(rom_bytes, 0x00BCD8) == 0x23FC
    assert _u16(rom_bytes, 0x00BD12) == 0x47EB  # lea $2c(a3), a3
    assert _u16(rom_bytes, 0x00BD14) == 0x002C
    assert _u16(rom_bytes, 0x00BE5A) == 0x4E92  # jsr (a2)
    assert _u16(rom_bytes, 0x00BE92) == 0x6100  # bsr.w $8530

    return {
        "milestone": "M15.354",
        "rom_range": "0x00BC8E..0x00BE96",
        "entrypoints": ENTRYPOINTS,
        "entity_stride_bytes": 0xBC,
        "combat_record_stride_bytes": 0x2C,
        "ali_entity_addr": "0xFF19E8",
        "ali_kill_count_offset": "0x96",
        "ali_power_bonus_offset": "0x90",
        "ali_kill_thresholds": ALI_KILL_THRESHOLDS,
        "post_loop_special_cancel": {
            "guard_entity": "0xFF19E8",
            "guard_flag_offset": "0x37",
            "guard_flag_bit": 3,
            "attack_index_addr": "0xFF198C",
            "matching_indices": [6, 10],
            "target_subroutine": "0x008530",
        },
        "verified_opcodes": {
            "0x00BC90_cb_beba": f"0x{_u32(rom_bytes, 0x00BC90):08X}",
            "0x00BCA6_cb_be98": f"0x{_u32(rom_bytes, 0x00BCA6):08X}",
            "0x00BD14_record_stride": f"0x{_u16(rom_bytes, 0x00BD14):04X}",
            "0x00BE5A_jsr_a2": f"0x{_u16(rom_bytes, 0x00BE5A):04X}",
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
