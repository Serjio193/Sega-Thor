#!/usr/bin/env python3
"""M15.366 extractor: ROM Ali status tick, low-HP/SP heartbeat alarms & respawn anchor (0x00557A..0x0056D7)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_status_alarms_m15_366.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x005582) == 0x00FF19E8
    assert _u16(rom_bytes, 0x0055D0) == 0x0040
    assert _u32(rom_bytes, 0x0055E4) == 0x00A0038C
    assert _u16(rom_bytes, 0x00561C) == 0x0018
    assert _u32(rom_bytes, 0x00562E) == 0x00A004DD
    assert _u32(rom_bytes, 0x005662) == 0x00FF1972

    return {
        "milestone": "M15.366",
        "rom_range": "0x00557A..0x0056D7",
        "ali_entity_addr": "0xFF19E8",
        "low_hp_alarm": {
            "timer_addr": "0xFF1993",
            "hp_offset": "0x8A",
            "max_trigger_hp": 0x40,
            "cooldown_shift": 1,
            "cooldown_bias": 10,
            "sfx_id": "0x00A0038C",
        },
        "low_sp_alarm": {
            "timer_addr": "0xFF1994",
            "active_spirit_addr": "0xFF1AA4",
            "excluded_spirit_type": "0x16",
            "sp_offset": "0x8E",
            "max_trigger_sp": 0x18,
            "cooldown_bias": 12,
            "sfx_id": "0x00A004DD",
        },
        "safe_respawn_anchor": {
            "hazard_offset": "0x6F",
            "hazard_bit": 7,
            "allowed_states": [0, 2, 4],
            "anchor_xy_addr": "0xFF1972",
        },
        "invuln_and_weapon_swap": {
            "freeze_flag_addr": "0xFF1985",
            "invuln_timer_addr": "0xFF198E",
            "weapon_swap_flag_addr": "0xFF198F",
            "weapon_class_addr": "0xFF0D7E",
            "spirit_ball_type_guard": "0x3A",
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
