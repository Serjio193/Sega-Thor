#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Scene Entity Limit Checker, Parameter Buffer Indexer
& ID Allocator Subsystem (0x01027C..0x0103B0, 308 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.471.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_entity_limits_m15_471.json")

RANGE_START = 0x01027C
RANGE_END = 0x0103B0
RANGE_SIZE = RANGE_END - RANGE_START  # 308 bytes

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
        "milestone": "M15.471",
        "title": "ROM Script Virtual Machine Scene Entity Limit Checker, Parameter Buffer Indexer & ID Allocator Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "ScriptState_CheckEntitySlotLimit1",
                "start": "0x01027C",
                "end": "0x0102BE",
                "size_bytes": 66,
                "description": "Evaluates parameter buffer $FF17C6 using range at +16(a0) and flag at +15(a0) for Opcode $01. Returns Carry clear on success with allocated ID in d0, Carry set on limit exhaustion."
            },
            {
                "name": "ScriptState_CheckEntitySlotLimit2",
                "start": "0x0102BE",
                "end": "0x010300",
                "size_bytes": 66,
                "description": "Evaluates parameter buffer $FF17C6 using range at +2(a0) and flag at +1(a0) for Opcode $0E. Returns Carry clear on success with allocated ID in d0, Carry set on limit exhaustion."
            },
            {
                "name": "ScriptState_CheckCompoundEntitySlotLimit",
                "start": "0x010300",
                "end": "0x010382",
                "size_bytes": 130,
                "description": "Evaluates parameter buffer $FF17C6 for dual compound entities (Opcode $17) using ranges at +2(a0) and +4(a0), packing dual IDs with flags +6/+7 into d0.l."
            },
            {
                "name": "ScriptState_FindEntityIdInPool2",
                "start": "0x010382",
                "end": "0x0103B0",
                "size_bytes": 46,
                "description": "Scans 17 Pool 2 slots ($FF1CD8, stride $BC=188) for active entity with matching ID d0 (& $7FF) at +$18. Returns Carry clear if found, Carry set if available (not found)."
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
