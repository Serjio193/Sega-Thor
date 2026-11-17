#!/usr/bin/env python3
"""Extract and verify ROM master entity subsystems clear, universal constructor & spirit glow pulse (0x008C5A..0x008E22)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_entity_constructor_lifecycle_m15_396.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x008C5A: master_entity_subsystems_clear
    assert u32(rom, 0x008C5E + 2) == 0x00FF2954  # Pool 1 base
    assert u16(rom, 0x008C64) == 0x7003          # moveq #3, d0 (4 slots)
    assert u16(rom, 0x008C66 + 2) == 0x005A      # stride 0x5A
    assert u32(rom, 0x008C6E + 2) == 0x00FF2D8C  # Pool 2 base
    assert u16(rom, 0x008C74) == 0x7005          # moveq #5, d0 (6 slots)
    assert u32(rom, 0x008C7A + 2) == 0x00FF2ABC  # Pool 3 base
    assert u16(rom, 0x008C80) == 0x7007          # moveq #7, d0 (8 slots)
    assert u32(rom, 0x008C86 + 2) == 0x00FF19C8  # Pool 4 base
    assert u16(rom, 0x008C8C) == 0x7003          # moveq #3, d0 (4 slots)
    assert u16(rom, 0x008C8E + 2) == 0x0008      # stride 0x08
    assert u32(rom, 0x008C96 + 2) == 0x00FF1B60  # Pool 5 base
    assert u16(rom, 0x008C9C) == 0x7012          # moveq #$12, d0 (19 slots)
    assert u16(rom, 0x008C9E + 2) == 0x00BC      # stride 0xBC

    # 0x008D06: universal_entity_constructor
    assert u32(rom, 0x008D12 + 2) == 0x00FF2954  # cmpa.l #$FF2954, a6
    assert u32(rom, 0x008D2A + 2) == 0x0003F2FA  # lea $3F2FA, a0 (descriptor table)
    assert u32(rom, 0x008D80 + 2) == 0x00FF2954  # large entity slot threshold

    # 0x008DA4: entity_script_pointers_reinit
    assert u32(rom, 0x008DA8 + 2) == 0x0003F2FA  # lea $3F2FA, a0

    # 0x008DD4: spirit_glow_pulse_controller
    assert u32(rom, 0x008DD4 + 2) == 0x00FF183E  # move.b $FF183E, d0
    assert u32(rom, 0x008DDA + 2) == 0x00FF1AA4  # tst.w $FF1AA4
    assert u32(rom, 0x008DF0 + 4) == 0x00FF199E  # cmpi.w #$14, $FF199E
    assert u32(rom, 0x008DFA + 4) == 0x00FF199F  # btst.b #0, $FF199F
    assert u32(rom, 0x008E1A + 2) == 0x00FF183E  # move.b d0, $FF183E

    pools = [
        {"base": "0x00FF2954", "slots": 4, "stride": 90, "clear_type": "0x8CAC"},
        {"base": "0x00FF2D8C", "slots": 6, "stride": 90, "clear_type": "0x8CAC"},
        {"base": "0x00FF2ABC", "slots": 8, "stride": 90, "clear_type": "0x8CAC"},
        {"base": "0x00FF19C8", "slots": 4, "stride": 8, "clear_type": "0x8CD0"},
        {"base": "0x00FF1B60", "slots": 19, "stride": 188, "clear_type": "0x8CDC"},
    ]

    return {
        "milestone": "M15.396",
        "title": "ROM Master Entity Subsystems Clear, Universal Constructor & Spirit Glow Pulse",
        "routines": {
            "master_entity_subsystems_clear": {
                "start": "0x008C5A",
                "end": "0x008D04",
                "bytes": 172,
                "pools": pools,
            },
            "universal_entity_constructor": {
                "start": "0x008D06",
                "end": "0x008DA2",
                "bytes": 158,
                "descriptor_table_rom": "0x0003F2FA",
                "descriptor_stride_bytes": 16,
                "small_entity_threshold_ram": "0x00FF2954",
                "small_entity_clear_words": 44,
                "large_entity_clear_words": 93,
            },
            "entity_script_pointers_reinit": {
                "start": "0x008DA4",
                "end": "0x008DD2",
                "bytes": 48,
                "descriptor_table_rom": "0x0003F2FA",
                "reloaded_pointers": ["+0x1A", "+0x1E", "+0x26", "+0x22", "+0x5E"],
            },
            "spirit_glow_pulse_controller": {
                "start": "0x008DD4",
                "end": "0x008E22",
                "bytes": 78,
                "glow_level_ram": "0x00FF183E",
                "active_spirit_ram": "0x00FF1AA4",
                "timer_word_ram": "0x00FF199E",
                "timer_subbyte_ram": "0x00FF199F",
            },
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
