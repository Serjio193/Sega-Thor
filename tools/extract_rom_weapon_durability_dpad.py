#!/usr/bin/env python3
"""M15.356 extractor: ROM weapon durability revert, HP/timer clamp & 16-way D-pad velocity (0x008530..0x0086B2)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_weapon_durability_dpad_m15_356.json")

# Cardinal / diagonal 16.16 fixed-point velocities from 0x00861A..0x0086B2
DPAD_TARGETS = {
    0x00861A: {"moving": False, "facing": None, "vx": 0, "vy": 0},
    0x008624: {"moving": True, "facing": 1, "vx": 0x00036000, "vy": 0},
    0x008634: {"moving": True, "facing": 3, "vx": -0x00036000, "vy": 0},
    0x008644: {"moving": True, "facing": 2, "vx": 0, "vy": 0x00030000},
    0x008654: {"moving": True, "facing": 0, "vx": 0, "vy": -0x00030000},
    0x008664: {"moving": True, "facing": 2, "vx": 0x0002A000, "vy": 0x00025800},
    0x008678: {"moving": True, "facing": 0, "vx": 0x0002A000, "vy": -0x00025800},
    0x00868C: {"moving": True, "facing": 2, "vx": -0x0002A000, "vy": 0x00025800},
    0x0086A0: {"moving": True, "facing": 0, "vx": -0x0002A000, "vy": -0x00025800},
}


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _i16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">h", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008538) == 0x00FF0DB0
    assert _u32(rom_bytes, 0x008540) == 0x00FF0D80
    assert _u32(rom_bytes, 0x0085B4) == 0x00C00269
    assert _u32(rom_bytes, 0x0085C6) == 0x00FF0E3C
    assert _u32(rom_bytes, 0x0085CC) == 0x00FF1A70
    assert _u32(rom_bytes, 0x0085E8) == 0x00FF165E

    pad_table = []
    for nibble in range(16):
        target = 0x0085FA + nibble * 2 + _i16(rom_bytes, 0x0085FA + nibble * 2)
        spec = DPAD_TARGETS[target]
        pad_table.append({
            "pad_nibble": nibble,
            "target_addr": f"0x{target:06X}",
            **spec,
        })

    return {
        "milestone": "M15.356",
        "rom_range": "0x008530..0x0086B2",
        "weapon_durability_decrement": {
            "addr": "0x008530..0x0085C2",
            "equipped_slot_addr": "0xFF0DB0",
            "inventory_table_addr": "0xFF0D80",
            "consumable_id_min": 2,
            "consumable_id_max": 12,
            "infinite_durability": 0xFF,
            "revert_weapon_id": 1,
            "revert_timer_word": 0x0A,
            "hud_toast_timer": 0x5A,
            "break_sfx_id": "0x00C00269",
        },
        "threshold_reset_guard": {
            "addr": "0x0085C4..0x0085E0",
            "value_addr": "0xFF0E3C",
            "limit_addr": "0xFF1A70",
            "tripped_flag_addr": "0xFF0DBB",
        },
        "dpad_velocity_table": pad_table,
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
