#!/usr/bin/env python3
"""
Extract ROM Bow Spirit (0x1E) 4-Node Vine Tether Physics, Round-Robin Pool 2
Target Scanner & Nearest Manhattan Finder (0x014242..0x014522, 736 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.501.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_bow_tether_target_m15_501.json"
)

RANGE_START = 0x014242
RANGE_END = 0x014522
RANGE_SIZE = RANGE_END - RANGE_START  # 736 bytes


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    routines = [
        {
            "name": "AI_BowSpirit_VineTetherPhysics",
            "start": "0x014242",
            "end": "0x0143C8",
            "size_bytes": 0x0143C8 - 0x014242,
            "description": (
                "Bow Spirit 4-node vine tether physics engine. Decrements 4 node timers at "
                "$FF19AC, $FF19B2, $FF19B8, $FF19BE if > 0. Executes 2-pass 3D distance "
                "constraint solver via 0x00F160: forward pass from Bow (a6) down through nodes "
                "3, 2, 1, 0, and backward pass from Ali anchor (a4) up through nodes 0, 1, 2, 3 "
                "to Bow (a6). Early-exits on Carry Clear (distance satisfied without clamping). "
                "Clamps Bow vertical elevation relative to Ali base height."
            )
        },
        {
            "name": "AI_BowSpirit_RoundRobinTargetScanner",
            "start": "0x0143C8",
            "end": "0x014494",
            "size_bytes": 0x014494 - 0x0143C8,
            "description": (
                "Round-robin Pool 2 enemy target scanner and lunge launcher. Advances cursor offset "
                "+$aa(a6) by 0xBC (188 bytes) across 17 slots ($FF1CD8..$FF2954). Filters active "
                "enemies (+0 > 0, +$38 bit 3 == 0, +$3a bit 0 == 0, special case type $66). "
                "Checks 3D Euclidean distance to Ali via 0x00F0EC <= 0x36. On match, projects 3D "
                "lunge velocity vector via 0x00F10C (speed $60), computes facing via 0x00E338, sets "
                "anim $22, timer 12, substate 3, and returns Carry=1 (success). Returns Carry=0 on fail."
            )
        },
        {
            "name": "AI_BowSpirit_FindNearestPool2Enemy",
            "start": "0x014494",
            "end": "0x014522",
            "size_bytes": 0x014522 - 0x014494,
            "description": (
                "Nearest Pool 2 enemy Manhattan distance finder. Scans all 17 slots in Pool 2 "
                "($FF1CD8..$FF2954) relative to Ali anchor +$4e/+$52/+$56(a4). Applies entity filters "
                "(active, not friendly/invulnerable bit 3, not target-immune bit 0 unless type $66). "
                "Computes 3D Manhattan metric |dx| + |dy| + |dz|. Tracks minimum distance and pointer. "
                "Returns Carry=0 (success) with a0=nearest entity, d0=distance, or Carry=1 on none."
            )
        }
    ]

    evidence = {
        "milestone": "M15.501",
        "title": (
            "ROM Bow Spirit (0x1E) 4-Node Vine Tether Physics, Round-Robin Pool 2 Target Scanner "
            "& Nearest Manhattan Finder"
        ),
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(
        f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) "
        f"across {len(routines)} routines to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
