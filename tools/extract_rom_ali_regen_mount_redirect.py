#!/usr/bin/env python3
"""M15.367 extractor: ROM Ali HP/SP passive regen & mount damage redirect (0x0056D8..0x00584E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_regen_mount_redirect_m15_367.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x0056DA) == 0x00FF1983
    assert _u32(rom_bytes, 0x00571C) == 0x00FF0DBA
    assert _u32(rom_bytes, 0x005728) == 0x00FF16F0
    assert _u32(rom_bytes, 0x005764) == 0x00FF1AA4
    assert _u16(rom_bytes, 0x0057E8) == 0x0028
    assert _u16(rom_bytes, 0x00581A) == 0x001C

    return {
        "milestone": "M15.367",
        "rom_range": "0x0056D8..0x00584E",
        "passive_hp_regen": {
            "requires_idle_state_0": True,
            "accessory_flags_addr": "0xFF0DBA",
            "hp_regen_bit": 0,
            "fast_regen_bit": 3,
            "terrain_flags_addr": "0xFF16F0",
            "water_bit": 5,
            "frame_counter_addr": "0xFF1971",
            "base_mask": "0x0F",
            "fast_mask": "0x07",
        },
        "passive_sp_regen": {
            "requires_no_active_spirit": True,
            "sp_regen_bit": 2,
            "fast_regen_bit": 3,
            "water_bit": 5,
            "base_mask": "0x07",
            "fast_mask": "0x03",
        },
        "ali_default_height": 0x28,
        "mount_damage_redirect": {
            "mount_flag_addr": "0xFF1989",
            "requires_ali_z_zero": True,
            "mount_slot_offset_bytes": 0xBC,
            "mount_entity_type": "0x1C",
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
