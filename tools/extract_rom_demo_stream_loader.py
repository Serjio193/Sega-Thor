#!/usr/bin/env python3
"""M15.359 extractor: ROM attract-mode 9-slot demo-stream loader & delta decoder (0x0089B2..0x008AA5)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_demo_stream_loader_m15_359.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x0089E2) == 0x00FF1930
    assert _u32(rom_bytes, 0x0089EE) == 0x00FF1996
    assert _u32(rom_bytes, 0x008A06) == 0x00FF0BFC
    assert _u32(rom_bytes, 0x008A1E) == 0x001698FE
    assert _u32(rom_bytes, 0x008A28) == 0x00FFBFAC
    assert _u32(rom_bytes, 0x008A32) == 0x00FFDFB2
    assert _u16(rom_bytes, 0x008A38) == 0x1FFE
    assert _u16(rom_bytes, 0x008A64) == 0x7F28

    offsets = [_u16(rom_bytes, 0x008A94 + i * 2) for i in range(9)]
    streams = [
        {
            "index": i,
            "offset_u16": f"0x{off:04X}",
            "rom_addr": f"0x{0x1698FE + off:06X}",
        }
        for i, off in enumerate(offsets)
    ]

    return {
        "milestone": "M15.359",
        "rom_range": "0x0089B2..0x008AA5",
        "demo_index_addr": "0xFF0BFC",
        "demo_index_max_before_wrap": 8,
        "stream_base_rom_addr": "0x1698FE",
        "offset_table_addr": "0x008A94",
        "decompressor_addr": "0x003820",
        "decomp_dest_addr": "0xFFBFAC",
        "delta_buffer_addr": "0xFFDFB2",
        "delta_loop_count": 0x1FFE,
        "delta_byte_count": 0x2000,
        "replay_ptr_addr": "0xFF1998",
        "replay_limit_addr": "0xFF1928",
        "replay_limit_value": "0x7F28",
        "continue_target_addr": "0x00CA24",
        "streams": streams,
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
