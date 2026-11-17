#!/usr/bin/env python3
"""
Extracts and analyzes ROM Dual Tilemap Plane Header Unpacker, VRAM Plane A/B Layout Configurator,
Stride Shift Calculator & Full Plane Initializer (0x00D542..0x00D7C0, 638 bytes)
from Beyond Oasis (USA).md into structured JSON evidence for milestone M15.440.
"""

import json
from pathlib import Path
import capstone

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUTPUT_PATH = Path("data/thor-evidence/rom_dual_plane_loader_m15_440.json")

START_ADDR = 0x00D542
END_ADDR = 0x00D7C0
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
        "milestone": "M15.440",
        "description": "ROM Dual Tilemap Plane Header Unpacker, VRAM Plane A/B Layout Configurator, Stride Shift Calculator & Full Plane Initializer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": EXPECTED_LEN,
        "first_bytes": slice_bytes[:16].hex(),
        "last_bytes": slice_bytes[-16:].hex(),
        "plane_a_vram_base": "0xC000",
        "plane_b_vram_base": "0xE000",
        "plane_a_context": "0xFF1716",
        "plane_a_aux": "0xFF1766",
        "plane_b_context": "0xFF173E",
        "plane_b_aux": "0xFF1770",
        "dynamic_buffers": ["0xFF179E", "0xFF17A2", "0xFF17A6", "0xFF17AA", "0xFF17AE"],
        "decompression_routine": "0x003820",
        "log2_shift_helper": "0x00D7B0",
        "plane_vram_stream_helper": "0x00D7C0",
        "screen_clamp_bounds": ["0xFF177A", "0xFF177C"],
        "flag_registers": {
            "FF17C1": "room_transition_active",
            "FF17C2": "plane_a_initialized",
            "FF17C3": "plane_b_initialized"
        },
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(metadata, f, indent=2)

    print(f"Wrote {len(instructions)} instructions to {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
