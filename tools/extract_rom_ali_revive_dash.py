#!/usr/bin/env python3
"""M15.374 extractor: ROM Ali auto-revive refill, item 0x2A scanner & 1.5x dash footstep (0x00611E..0x0062CA)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_revive_dash_m15_374.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x00617E) == 0x00C0034B  # refill chime
    assert _u32(rom_bytes, 0x0061C0) == 0x00D00355  # revive jingle
    assert _u32(rom_bytes, 0x0061D8) == 0x00FF0D90  # inventory scanner base
    assert _u16(rom_bytes, 0x0061DE) == 0x0C10  # cmpi.b #$2a, (a0)
    assert _u16(rom_bytes, 0x0061E0) == 0x002A
    assert _u32(rom_bytes, 0x0062A2) == 0x02400357  # water dash sfx
    assert _u32(rom_bytes, 0x0062B2) == 0x0040038E  # ground dash sfx

    return {
        "milestone": "M15.374",
        "rom_range": "0x00611E..0x0062CA",
        "auto_revive": {
            "hp_sp_step": 3,
            "refill_chime_sfx": "0x00C0034B",
            "revive_jingle_sfx": "0x00D00355",
            "dead_flag_addr": "0xFF1987",
            "hud_flash_addr": "0xFF1857",
            "standing_states": {
                "default": {"state": "0x24", "anim": "0x30"},
                "alt_prone": {"state": "0x1E", "anim": "0x2A"},
            },
        },
        "revive_item_scanner": {
            "inventory_base_addr": "0xFF0D90",
            "slot_count": 16,
            "revive_item_id": "0x2A",
            "disable_flag_addr": "0xFF0E40",
            "disable_flag_val": "0x10",
        },
        "dash_dispatcher": {
            "dash_multiplier": 1.5,
            "walk_state": 2,
            "walk_anim": 2,
            "dash_state": 4,
            "dash_anim": 4,
            "water_dash_sfx": "0x02400357",
            "ground_dash_sfx": "0x0040038E",
            "spirit_shadow_sfx": "0x01500218",
            "idle_fallback": "0x0062CC",
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
