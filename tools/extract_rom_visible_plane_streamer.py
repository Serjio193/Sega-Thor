#!/usr/bin/env python3
"""
Extracts and analyzes ROM Visible Tilemap Plane VRAM Streamer & Full-Screen Blank Tile Plane Clearer
(0x00D7C0..0x00D9A4, 484 bytes) from Beyond Oasis (USA).md into structured JSON evidence for milestone M15.441.
"""

import json
from pathlib import Path
import capstone

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUTPUT_PATH = Path("data/thor-evidence/rom_visible_plane_streamer_m15_441.json")

START_ADDR = 0x00D7C0
END_ADDR = 0x00D9A4
EXPECTED_LEN = END_ADDR - START_ADDR

def main():
    if not ROM_PATH.exists():
        raise FileNotFoundError(f"ROM file not found: {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_bytes = f.read()

    slice_bytes = rom_bytes[START_ADDR:END_ADDR]
    assert len(slice_bytes) == EXPECTED_LEN, f"Expected {EXPECTED_LEN} bytes, got {len(slice_bytes)}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    for ins in md.disasm(slice_bytes, START_ADDR):
        instructions.append({
            "address": f"0x{ins.address:06X}",
            "mnemonic": ins.mnemonic,
            "op_str": ins.op_str,
            "bytes": ins.bytes.hex()
        })

    metadata = {
        "milestone": "M15.441",
        "description": "ROM Visible Tilemap Plane VRAM Streamer & Full-Screen Blank Tile Plane Clearer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": EXPECTED_LEN,
        "first_bytes": slice_bytes[:16].hex(),
        "last_bytes": slice_bytes[-16:].hex(),
        "streamer_entry": "0x00D7C0",
        "blank_fill_safe_entry": "0x00D950",
        "blank_fill_raw_entry": "0x00D95E",
        "blank_fill_body_entry": "0x00D962",
        "blank_tile_pattern": "0x0780",
        "blank_fill_cols": 40,
        "blank_fill_rows": 28,
        "vdp_data_port": "0xC00000",
        "vdp_ctrl_port": "0xC00004",
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(metadata, f, indent=2)

    print(f"Wrote {len(instructions)} instructions to {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
