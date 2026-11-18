#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $01, $06, $07, $08, $0A, $0B:
Loop Engine, Frame Sleep & Dynamic Entity Spawner (0x00FC6C..0x00FD8C, 288 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.466.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_opcodes_loops_spawn_m15_466.json")

RANGE_START = 0x00FC6C
RANGE_END = 0x00FD8C
RANGE_SIZE = RANGE_END - RANGE_START  # 288 bytes

SPAWN_TABLE_BASE = 0x00FD86
SPAWN_TABLE_COUNT = 3

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Extract spawn position mode jump table at 0x00FD86
    spawn_modes = []
    spawn_names = ["SpawnPosMode0", "SpawnPosMode1", "SpawnPosMode2"]
    for i in range(SPAWN_TABLE_COUNT):
        entry_addr = SPAWN_TABLE_BASE + i * 2
        rel_offset = struct.unpack(">h", rom[entry_addr:entry_addr + 2])[0]
        target_addr = entry_addr + rel_offset
        spawn_modes.append({
            "mode_id": i,
            "name": spawn_names[i],
            "table_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_offset,
            "target_addr": f"0x{target_addr:06X}"
        })

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
        "milestone": "M15.466",
        "title": "ROM Script Virtual Machine Opcodes $01, $06, $07, $08, $0A, $0B: Loop Engine, Frame Sleep & Dynamic Entity Spawner",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "spawn_position_table": {
            "base_address": f"0x{SPAWN_TABLE_BASE:06X}",
            "count": SPAWN_TABLE_COUNT,
            "entries": spawn_modes
        },
        "opcodes": [
            {
                "opcode": "0x06",
                "name": "ScriptOp_LoopDecrementBranch",
                "start": "0x00FC6C",
                "end": "0x00FC94",
                "size_bytes": 40,
                "description": (
                    "Decrements loop counter indexed by +1(a0) at $FF1826. If result is non-zero, adds displacement "
                    "+2(a0) to script PC a0 (looping). If counter reaches zero, advances past 4-byte instruction."
                ),
                "constants": {
                    "loop_counter_base": "0xFF1826"
                }
            },
            {
                "opcode": "0x07",
                "name": "ScriptOp_SetLoopCounter",
                "start": "0x00FC94",
                "end": "0x00FCA6",
                "size_bytes": 18,
                "description": "Sets loop counter indexed by (a0)+ at $FF1826 to immediate 16-bit value (a0)+."
            },
            {
                "opcode": "0x08",
                "name": "ScriptOp_WaitFramesSleep",
                "start": "0x00FCA6",
                "end": "0x00FCB8",
                "size_bytes": 18,
                "description": (
                    "Sets script frame delay counter $FF17B6 = (a0)+, writes script PC to $FF17B2, "
                    "and branches to $F91C yielding current frame execution."
                ),
                "constants": {
                    "delay_timer": "0xFF17B6"
                }
            },
            {
                "opcode": "0x0A",
                "name": "ScriptOp_SendEntityEvent",
                "start": "0x00FCB8",
                "end": "0x00FCC4",
                "size_bytes": 12,
                "description": "Reads entity pointer a6 from (a0)+ and event command d6 from (a0)+, dispatching via $D9A4."
            },
            {
                "opcode": "0x0B",
                "name": "ScriptOp_SetScenarioThreshold",
                "start": "0x00FCC4",
                "end": "0x00FCCE",
                "size_bytes": 10,
                "description": "Sets script scenario limit / abort threshold $FF17B7 = (a0)+."
            },
            {
                "opcode": "0x01",
                "name": "ScriptOp_SpawnScriptEntity",
                "start": "0x00FCCE",
                "end": "0x00FD86",
                "size_bytes": 184,
                "description": (
                    "Checks quest state via $0101EC against threshold $FF17B7 (aborts script if >= threshold). "
                    "Allocates entity slot via $DA2A and verifies scene entity count limit $01027C. Reads spawn mode d7.b "
                    "and entity type d1.w (stored to +$00), runs constructor $8D06, randomizes facing 0..3 via $E268, "
                    "stores parameter +$96, evaluates spatial position via table at $FD86, sets initial & max HP +$88/+$8A, "
                    "drop item +$8C, visual flag +$66, runs animation setup $9D00, and starts entity AI from table $DF54 "
                    "for type >= $16."
                ),
                "constants": {
                    "ai_jump_table": "0x00DF54",
                    "ai_min_type": 22
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
