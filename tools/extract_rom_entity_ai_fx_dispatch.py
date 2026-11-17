#!/usr/bin/env python3
"""M15.364 extractor: ROM entity AI dispatch table, iso extent updater & FX loops (0x00DEEC..0x00E150)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_entity_ai_fx_dispatch_m15_364.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x00DEFA) == 0x00FF1AA4
    assert _u16(rom_bytes, 0x00DF0A) == 0x0016
    assert _u32(rom_bytes, 0x00E0C8) == 0x00FF2D8C
    assert _u32(rom_bytes, 0x00E0F6) == 0x00FF1856
    assert _u32(rom_bytes, 0x00E114) == 0x00FF2ABC

    # 0x00DF54..0x00E0B7: 89 u32 handler pointers (type_id 0x16..0xC6 step 2)
    handlers = []
    count = (0x00E0B8 - 0x00DF54) // 4
    for i in range(count):
        addr = _u32(rom_bytes, 0x00DF54 + i * 4)
        handlers.append({
            "type_id": f"0x{0x16 + i * 2:02X}",
            "handler_addr": f"0x{addr:06X}",
        })

    return {
        "milestone": "M15.364",
        "rom_range": "0x00DEEC..0x00E150",
        "entity_ai_loop_00deec": {
            "timestop_addr": "0xFF17B8",
            "timestop_bit": 1,
            "pool_base_addr": "0xFF1AA4",
            "slot_count": 20,
            "slot_stride_bytes": 0xBC,
            "min_type_id": "0x16",
            "table_origin_pc": "0x00DF28",
            "first_entry_addr": "0x00DF54",
            "handler_count": len(handlers),
            "handlers": handlers,
        },
        "fx_callback_loop_00e0ba": {
            "pool_base_addr": "0xFF2D8C",
            "slot_count": 6,
            "slot_stride_bytes": 0x5A,
            "fn_ptr_offset": "0x56",
        },
        "toast_and_pickup_loop_00e0f4": {
            "toast_timer_addr": "0xFF1856",
            "pool_base_addr": "0xFF2ABC",
            "slot_count": 8,
            "slot_stride_bytes": 0x5A,
            "state_offset": "0x04",
            "jump_table_addr": "0x00E140",
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
