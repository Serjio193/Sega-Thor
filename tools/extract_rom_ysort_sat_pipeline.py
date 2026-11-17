#!/usr/bin/env python3
"""Extract and verify ROM master Y-sorted SAT render pipeline (0x00A196..0x00A27E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ysort_sat_pipeline_m15_333.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00A1AC) == 0x00FF2954  # 4 high-priority fx (stride 0x5A)
    assert u32(rom, 0x00A1C4) == 0x00FF19C8  # 4 hit-sparks (stride 8)
    assert u32(rom, 0x00A1DA) == 0x00FF19E8  # 21 entities (stride 0xBC)
    assert u16(rom, 0x00A1DE) == 0x7C14      # moveq #$14, d6 (21 slots)
    assert u16(rom, 0x00A1EC) == 0x00BC      # entity stride
    assert u32(rom, 0x00A1F4) == 0x00FF188E  # 21-bit done mask
    assert u32(rom, 0x00A24A) == 0x00FF2ABC  # 8 shadow/ground fx (stride 0x5A)
    assert u32(rom, 0x00A262) == 0x00FF2D8C  # 6 floor fx (stride 0x5A)

    return {
        "milestone": "M15.333",
        "title": "ROM Master Y-Sorted SAT Render Pipeline",
        "routine": {"start": "0x00A196", "end": "0x00A27E"},
        "render_passes": [
            {"name": "high_priority_fx", "ram": "0x00FF2954", "count": 4, "stride": "0x5A"},
            {"name": "hit_sparks", "ram": "0x00FF19C8", "count": 4, "stride": "0x08"},
            {"name": "y_sorted_entities_desc", "ram": "0x00FF19E8", "count": 21, "stride": "0xBC"},
            {"name": "ground_shadow_fx", "ram": "0x00FF2ABC", "count": 8, "stride": "0x5A"},
            {"name": "floor_fx", "ram": "0x00FF2D8C", "count": 6, "stride": "0x5A"},
        ],
        "constants": {
            "entity_done_mask_ram": "0x00FF188E",
            "entity_y_offset": "0x0C",
            "initial_max_y_sentinel": -1,
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
