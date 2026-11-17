#!/usr/bin/env python3
"""M15.371 extractor: ROM Ali airborne knockdown physics, wall impact & landing recovery (0x005CF8..0x005DE2)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_knockdown_physics_m15_371.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _i32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">i", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u16(rom_bytes, 0x005CFA) == 0x0010  # height = 16
    assert _u32(rom_bytes, 0x005D0C) == 0x0080035D  # wall hit sfx
    assert _u32(rom_bytes, 0x005D24) == 0x0080035D
    assert _u32(rom_bytes, 0x005D34) == 0x0000C000  # gravity subtraction 0xC000
    assert _u16(rom_bytes, 0x005D84) == 0x000A  # 10f recovery base
    assert _u16(rom_bytes, 0x005D92) == 0x0014  # 20f recovery long
    assert _u16(rom_bytes, 0x005DAC) == 0x005E  # death state

    return {
        "milestone": "M15.371",
        "rom_range": "0x005CF8..0x005DE2",
        "prone_height": 0x10,
        "wall_impact": {
            "sfx_id": "0x0080035D",
            "x_wall_bit": 6,
            "y_wall_bit": 6,
        },
        "airborne_gravity": {
            "gravity_step": -0x0000C000,
            "zero_nudge": -1,
            "probe_helper": "0x00B79A",
        },
        "ground_landing": {
            "landed_flag_offset": "0x39",
            "landed_flag_bit": 2,
            "dust_shake_timer": 0x0F,
            "recovery_delay_base": 10,
            "recovery_delay_long": 20,
            "recovery_anim_28_bonus": 10,
        },
        "death_transition": {
            "death_state": "0x5E",
            "death_anim_front": "0x68",
            "death_anim_back": "0x6C",
        },
        "recovery_wake_up": {
            "state_advance": 2,
            "anim_advance": 2,
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
