#!/usr/bin/env python3
"""
Extracts and analyzes ROM Tilemap Rectangle Fill & Block Copy Subsystem
(0x00DB1A..0x00DC26, 268 bytes) from Beyond Oasis (USA).md into structured JSON evidence for milestone M15.443.
"""

import json
from pathlib import Path
import capstone

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUTPUT_PATH = Path("data/thor-evidence/rom_tilemap_rect_ops_m15_443.json")

START_ADDR = 0x00DB1A
END_ADDR = 0x00DC26
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
        "milestone": "M15.443",
        "description": "ROM Tilemap Rectangle Fill & Block Copy Subsystem",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": EXPECTED_LEN,
        "first_bytes": slice_bytes[:16].hex(),
        "last_bytes": slice_bytes[-16:].hex(),
        "routines": {
            "tilemap_fill_rect_byte": "0x00DB1A",
            "tilemap_copy_rect_word_and_sync": "0x00DB64",
            "tilemap_copy_rect_byte": "0x00DBCA"
        },
        "vram_sync_helper": "0x00DC26",
        "plane_a_aux": "0xFF1766",
        "plane_b_aux": "0xFF1770",
        "plane_a_context": "0xFF1716",
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(metadata, f, indent=2)

    print(f"Wrote {len(instructions)} instructions to {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
