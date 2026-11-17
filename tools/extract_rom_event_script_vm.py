#!/usr/bin/env python3
"""M15.365 extractor: ROM room/event script VM dispatcher & cutscene lock (0x00F788..0x00F8C6)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_event_script_vm_m15_365.json")


def _i16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">h", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x00F78C) == 0x00FF17B8
    assert _u32(rom_bytes, 0x00F798) == 0x00FF17B6
    assert _u32(rom_bytes, 0x00F7AC) == 0x00FF17BD
    assert _u32(rom_bytes, 0x00F7B6) == 0x00FF17B2
    assert _u32(rom_bytes, 0x00F896) == 0x00FF183A

    # 31 even-aligned VM opcode entries 0x00..0x1E at 0x00F7D0..0x00F80D
    opcodes = []
    for op in range(0x1F):
        slot_addr = 0x00F7D0 + op * 2
        target = slot_addr + _i16(rom_bytes, slot_addr)
        assert (target & 1) == 0
        opcodes.append({
            "opcode": f"0x{op:02X}",
            "handler_addr": f"0x{target:06X}",
        })

    return {
        "milestone": "M15.365",
        "rom_range": "0x00F788..0x00F8C6",
        "timestop_addr": "0xFF17B8",
        "timestop_bit": 1,
        "wait_timer_addr": "0xFF17B6",
        "cutscene_lock_timer_addr": "0xFF17BD",
        "cutscene_handler_index_addr": "0xFF183A",
        "cutscene_jump_table_addr": "0x00F8C8",
        "vm_pc_addr": "0xFF17B2",
        "opcode_table_addr": "0x00F7D0",
        "opcode_count": len(opcodes),
        "opcodes": opcodes,
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
