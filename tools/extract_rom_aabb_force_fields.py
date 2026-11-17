#!/usr/bin/env python3
"""
extract_rom_aabb_force_fields.py — Extract ROM 3D AABB Entity Force Vector Field,
Conveyor Belt & Elevation Current Displacement Subsystem
(0x00DDA2..0x00DEEC, 330 bytes).
"""

import json
import os
import sys
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_aabb_force_fields_m15_445.json")

START_ADDR = 0x00DDA2
END_ADDR = 0x00DEEC


def extract_instructions():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "routine": "entity_3d_aabb_force_field_subsystem",
        "description": (
            "ROM 3D AABB Entity Force Vector Field, Conveyor Belt & Elevation Current "
            "Displacement Subsystem"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "entrypoints": {
            "entity_apply_box_force_vector": {
                "address": "0x00DDA2",
                "length": 94,
                "notes": (
                    "Iterates 21 entity slots at $FF19E8 (stride $BC). Performs 3D AABB "
                    "check (Y, X, Z) and adds 32-bit (d1, d2) displacement to ($72, $76)."
                )
            },
            "entity_apply_box_force_vector_with_elevation_boost": {
                "address": "0x00DE00",
                "length": 108,
                "notes": (
                    "Performs 3D AABB check. If entity Z ($10) != 0, doubles the (d1, d2) "
                    "displacement force."
                )
            },
            "entity_apply_conveyor_current_force": {
                "address": "0x00DE6C",
                "length": 128,
                "notes": (
                    "Checks immunity flags bit 0 and bit 3 at +$3A(a0). Performs 3D AABB check "
                    "and doubles force when entity Z ($10) != 0."
                )
            }
        },
        "registers": {
            "a0": "Entity pointer (traverses 21 slots from $FF19E8 with stride $BC / 188B)",
            "d0": "Loop counter (initialized to #$14 for 21 entities)",
            "d1": "32-bit X force vector / displacement accumulator addend",
            "d2": "32-bit Y force vector / displacement accumulator addend",
            "d3": "Lower word: min Y; upper word (swapped): min X",
            "d4": "Lower word: max Y; upper word (swapped): max X",
            "d5": "Lower word: max Z; upper word (swapped): min Z",
            "d6": "Entity coordinate scratch register"
        },
        "entity_offsets": {
            "type_status": "0x00",
            "pos_x": "0x08",
            "pos_y": "0x0C",
            "elevation_z": "0x10",
            "flags_immunity": "0x3A",
            "accum_dx": "0x72",
            "accum_dy": "0x76"
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions()
