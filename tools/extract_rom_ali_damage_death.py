#!/usr/bin/env python3
"""M15.368 extractor: ROM Ali HP damage, death-arc trigger & hitstun/knockdown states (0x005850..0x0059B8)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_damage_death_m15_368.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x005858) == 0x00FF198E
    assert _u16(rom_bytes, 0x005870) == 0x0FFF
    assert _u32(rom_bytes, 0x00588C) == 0x00FF0E3C
    assert _u32(rom_bytes, 0x0058A6) == 0x00FF1987
    assert _u16(rom_bytes, 0x0058DA) == 0x0018
    assert _u16(rom_bytes, 0x005914) == 0x001A
    assert _u16(rom_bytes, 0x005920) == 0x0020

    return {
        "milestone": "M15.368",
        "rom_range": "0x005850..0x0059B8",
        "mount_redirect_invuln_frames": 7,
        "damage_calc": {
            "damage_mask": "0x0FFF",
            "defense_offset": "0x92",
            "min_damage": 1,
            "hp_offset": "0x8A",
            "damage_accum_addr": "0xFF0E3C",
            "damage_clamp_helper": "0x0085C4",
        },
        "death_trigger": {
            "launch_arc_helper": "0x00E4D0",
            "forced_knockdown_mask": "0xC000",
            "dead_flag_addr": "0xFF1987",
            "sp_offset": "0x8E",
            "spirit_dismiss_state": "0x1A",
        },
        "hit_reactions": {
            "light_hitstun": {"state": "0x18", "anim_base": "0x18"},
            "knockdown_front": {"state": "0x1A", "anim_id": "0x26"},
            "knockdown_back": {"state": "0x20", "anim_id": "0x2C"},
            "knockdown_z_lift": 0x10,
        },
        "ali_state_jump_table": "0x0059BA",
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
