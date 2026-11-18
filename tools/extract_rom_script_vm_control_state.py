#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $0F..$11, $13, $14 & State Query Subroutines:
Persistent Modifiers, GOSUB & Ali Vital Metrics (0x010140..0x01027C, 316 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.470.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_control_state_m15_470.json")

RANGE_START = 0x010140
RANGE_END = 0x01027C
RANGE_SIZE = RANGE_END - RANGE_START  # 316 bytes

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
        "milestone": "M15.470",
        "title": "ROM Script Virtual Machine Opcodes $0F..$11, $13, $14 & State Query Subroutines: Persistent Modifiers, GOSUB & Ali Vital Metrics",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "opcodes": [
            {
                "opcode": "0x0F",
                "name": "ScriptOp_ModifyBank1Bit",
                "start": "0x010140",
                "end": "0x010170",
                "size_bytes": 48,
                "description": "Modifies bit d0 = +2(a0) in Bank 1 ($FF0DBC): 0=bset, 1=bclr, 2=bchg. Advances PC by 4."
            },
            {
                "opcode": "0x10",
                "name": "ScriptOp_ModifyBank2Bit",
                "start": "0x010170",
                "end": "0x0101A0",
                "size_bytes": 48,
                "description": "Modifies bit d0 = +2(a0) in Bank 2 ($FF0E1C): 0=bset, 1=bclr, 2=bchg. Advances PC by 4."
            },
            {
                "opcode": "0x11",
                "name": "ScriptOp_ModifyBank3Bit",
                "start": "0x0101A0",
                "end": "0x0101D0",
                "size_bytes": 48,
                "description": "Modifies bit d0 = +2(a0) in Bank 3 ($FF0E2C): 0=bset, 1=bclr, 2=bchg. Advances PC by 4."
            },
            {
                "opcode": "0x13",
                "name": "ScriptOp_SubroutineCallGosub",
                "start": "0x0101D0",
                "end": "0x0101E4",
                "size_bytes": 20,
                "description": "Saves script PC return address to $FF1836 and branches to 24-bit/32-bit subroutine address."
            },
            {
                "opcode": "0x14",
                "name": "ScriptOp_SubroutineReturn",
                "start": "0x0101E4",
                "end": "0x0101EC",
                "size_bytes": 8,
                "description": "Restores script PC from $FF1836 (movea.l $FF1836, a0; rts)."
            }
        ],
        "state_helpers": [
            {
                "name": "ScriptState_CountLivingEnemies",
                "start": "0x0101EC",
                "end": "0x010214",
                "size_bytes": 40,
                "description": "Scans 17 Pool 2 slots ($FF1CD8, stride $BC=188); counts slots with +$00 > 0 and bit 0 of +$3A == 0."
            },
            {
                "name": "ScriptState_CountActiveEntities",
                "start": "0x010214",
                "end": "0x010232",
                "size_bytes": 30,
                "description": "Scans 17 Pool 2 slots; counts all slots with +$00 > 0."
            },
            {
                "name": "ScriptState_FindEntityTypeInPool2",
                "start": "0x010232",
                "end": "0x010254",
                "size_bytes": 34,
                "description": "Scans 17 Pool 2 slots matching type d1.w against +$00; returns Carry set on match, clear on absence."
            },
            {
                "name": "ScriptState_CalcAliHpPercent",
                "start": "0x010254",
                "end": "0x010268",
                "size_bytes": 20,
                "description": "Computes (cur_hp * 100) / max_hp using Ali HP at $FF1A72 / $FF1A70, returning percentage in d0.w."
            },
            {
                "name": "ScriptState_CalcAliSpPercent",
                "start": "0x010268",
                "end": "0x01027C",
                "size_bytes": 18,
                "description": "Computes (cur_sp * 100) / max_sp using Ali SP at $FF1A76 / $FF1A74, returning percentage in d0.w."
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
