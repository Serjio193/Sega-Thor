#!/usr/bin/env python3
"""
extract_rom_spirits_sp_and_state_tables.py — Milestone M15.271

Extracts ROM semantic knowledge for all 4 Elemental Spirits' live SP drain
intervals, despawn handlers, and per-spirit state jump tables:
1. Dytto  (0x0018 @ 0x011B4E): drain interval 45 frames (0x2D), despawn 0x012314, table 0x011BAC.
2. Efreet (0x001A @ 0x01254C): drain interval 35 frames (0x23), despawn 0x012BA8, table 0x0125AE.
3. Shade  (0x001C @ 0x012F1E): drain interval 40 frames (0x28), despawn 0x0136CC, table 0x013008.
4. Bow    (0x001E @ 0x01387C): drain interval 30 frames (0x1E), despawn 0x013F22, table 0x0138E6.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_spirits_sp_and_state_tables_m15_271.json"

# (name, type_id, drain_pc, expected_drain_frames, despawn_pc, table_pc)
SPIRITS = [
    ("Dytto", 0x0018, 0x011B72, 45, 0x012314, 0x011BAC),
    ("Efreet", 0x001A, 0x012574, 35, 0x012BA8, 0x0125AE),
    ("Shade", 0x001C, 0x012F52, 40, 0x0136CC, 0x013008),
    ("Bow", 0x001E, 0x0138A0, 30, 0x013F22, 0x0138E6),
]


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def decode_state_table(rom: bytes, table_pc: int, max_entries: int = 12) -> dict:
    """Decode self-relative 16-bit state table entries until non-even/out-of-range."""
    entries = {}
    for i in range(max_entries):
        addr = table_pc + i * 2
        off = read_s16_be(rom, addr)
        tgt = addr + off
        if off <= 0 or (tgt & 1) != 0 or tgt >= table_pc + 0x2000:
            break
        entries[f"0x{i * 2:02X}"] = f"0x{tgt:06X}"
    return entries


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    result = {}
    for name, tid, drain_pc, expected_frames, despawn_pc, table_pc in SPIRITS:
        # move.w #imm, $8e(a6) -> 3D7C <imm> 008E
        assert read_u16_be(rom, drain_pc) == 0x3D7C
        drain_frames = read_u16_be(rom, drain_pc + 2)
        assert drain_frames == expected_frames, f"{name}: got {drain_frames}, expected {expected_frames}"
        assert read_u16_be(rom, drain_pc + 4) == 0x008E

        states = decode_state_table(rom, table_pc)
        assert len(states) >= 6, f"{name}: too few states decoded at 0x{table_pc:06X}"

        result[name] = {
            "entity_type": f"0x{tid:04X}",
            "sp_drain_interval_frames": drain_frames,
            "despawn_handler": f"0x{despawn_pc:06X}",
            "state_table_addr": f"0x{table_pc:06X}",
            "state_count": len(states),
            "states": states,
        }

    evidence = {
        "milestone": "M15.271",
        "description": "ROM Live SP Drain Intervals, Despawn Handlers, and State Jump Tables for All 4 Spirits",
        "player_sp_field": "$8E($FF19E8)",
        "spirit_drain_timer_field": "$8E(A6)",
        "spirits": result,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(result)} spirits)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
