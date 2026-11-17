#!/usr/bin/env python3
"""M15.361 extractor: ROM 5-pool resetter, 32B entity template initializer & tint ramp (0x008C5A..0x008E20)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_entity_template_pool_init_m15_361.json")

POOLS = [
    {"base_addr": "0xFF2954", "slot_count": 4, "stride_bytes": 0x5A, "clear_mode": "fx_5a"},
    {"base_addr": "0xFF2D8C", "slot_count": 6, "stride_bytes": 0x5A, "clear_mode": "fx_5a"},
    {"base_addr": "0xFF2ABC", "slot_count": 8, "stride_bytes": 0x5A, "clear_mode": "fx_5a"},
    {"base_addr": "0xFF19C8", "slot_count": 4, "stride_bytes": 0x08, "clear_mode": "type_only"},
    {"base_addr": "0xFF1B60", "slot_count": 19, "stride_bytes": 0xBC, "clear_mode": "entity_bc"},
]


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def _decode_template(data: bytes, idx: int) -> dict:
    off = 0x03F2FA + idx * 32
    ptrs = struct.unpack_from(">5I", data, off + 12)
    return {
        "index": idx,
        "type_id": (idx + 1) * 2,
        "height_4a": _u16(data, off),
        "radius_42_44_46": data[off + 2],
        "sprite_w_48": data[off + 3],
        "sprite_h_49": data[off + 4],
        "unk_3d": data[off + 5],
        "unk_4c": data[off + 6],
        "flags_3a": f"0x{data[off + 7]:02X}",
        "flags_36": f"0x{data[off + 8]:02X}",
        "flags_37": f"0x{data[off + 9]:02X}",
        "flags_38": f"0x{data[off + 10]:02X}",
        "flags_3c": f"0x{data[off + 11]:02X}",
        "ptr_1a_anim": f"0x{ptrs[0]:06X}",
        "ptr_1e_frame": f"0x{ptrs[1]:06X}",
        "ptr_26_ai": f"0x{ptrs[2]:06X}",
        "ptr_22_hitbox": f"0x{ptrs[3]:06X}",
        "ptr_5e_extra": f"0x{ptrs[4]:06X}",
    }


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008C60) == 0x00FF2954
    assert _u32(rom_bytes, 0x008C98) == 0x00FF1B60
    assert _u32(rom_bytes, 0x008D2C) == 0x0003F2FA
    assert _u32(rom_bytes, 0x008DAA) == 0x0003F2FA
    assert _u32(rom_bytes, 0x008DD6) == 0x00FF183E
    assert _u32(rom_bytes, 0x008DDC) == 0x00FF1AA4

    return {
        "milestone": "M15.361",
        "rom_range": "0x008C5A..0x008E20",
        "pool_resetter_008c5a": POOLS,
        "template_initializer_008d06": {
            "table_addr": "0x03F2FA",
            "record_stride_bytes": 32,
            "full_entity_threshold_addr": "0xFF2954",
            "full_clear_words": 0x5D,
            "fx_clear_words": 0x2C,
            "sample_templates": [_decode_template(rom_bytes, i) for i in range(4)],
        },
        "tint_ramp_008dd4": {
            "intensity_addr": "0xFF183E",
            "active_flag_addr": "0xFF1AA4",
            "pulse_timer_addr": "0xFF199E",
            "pulse_min_timer": 0x14,
            "pulse_mask_addr": "0xFF199F",
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
