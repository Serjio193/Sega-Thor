#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $0E & $17: Fixed Position &
Dual Cross-Linked Compound Entity Spawners (0x00FD8C..0x00FEDE, 338 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.467.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_compound_spawn_m15_467.json")

RANGE_START = 0x00FD8C
RANGE_END = 0x00FEDE
RANGE_SIZE = RANGE_END - RANGE_START  # 338 bytes

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

    evidence = {
        "milestone": "M15.467",
        "title": "ROM Script Virtual Machine Opcodes $0E & $17: Fixed Position & Dual Cross-Linked Compound Entity Spawners",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "opcodes": [
            {
                "opcode": "0x0E",
                "name": "ScriptOp_SpawnScriptEntityFixedPos",
                "start": "0x00FD8C",
                "end": "0x00FE0A",
                "size_bytes": 126,
                "description": (
                    "Checks quest state via $0101EC against threshold $FF17B7. Allocates entity slot via $DA2A "
                    "and checks limit $0102BE. Reads entity type d1.w (stored to +$00), constructor $8D06, visual flag "
                    "+$66, facing +$16, parameter +$96, positions via $103B0, sets HP +$88/+$8A, drop +$8C, runs animation "
                    "$9D00, and starts AI from $DF54 for type >= $16."
                ),
                "constants": {
                    "position_routine": "0x0103B0",
                    "ai_table": "0x00DF54"
                }
            },
            {
                "opcode": "0x17",
                "name": "ScriptOp_SpawnDualCompoundEntities",
                "start": "0x00FE0A",
                "end": "0x00FEDE",
                "size_bytes": 212,
                "description": (
                    "Dual-compound entity spawner (Types $72 and $70). Checks quest state vs threshold $FF17B7 - 1. "
                    "Allocates primary slot a5 (Type $72) and secondary slot a6 (Type $70) via $DA2A. Configures visual "
                    "flags +$66(a5) = 9, +$66(a6) = 5. Assigns shared HP d2 to both entities. Cross-links mutual pointers: "
                    "+$A6(a5) = a6 and +$A6(a6) = a5. Sets composite positions via $1046C and clones coords (x, y) from a6 to a5. "
                    "Activates coordinated compound AI routines: $01DF1C for a6 and $01D140 for a5."
                ),
                "constants": {
                    "primary_type": "0x0072",
                    "secondary_type": "0x0070",
                    "primary_visual_flag": 9,
                    "secondary_visual_flag": 5,
                    "cross_link_offset": "0x00A6",
                    "primary_ai": "0x01DF1C",
                    "secondary_ai": "0x01D140"
                }
            }
        ],
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
