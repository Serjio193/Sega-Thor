#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $15, $16, $18, $19:
Persistent Gater, Spawner & Ballistic Projectile Engine (0x00FEDE..0x010034, 342 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.468.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_persistent_projectiles_m15_468.json")

RANGE_START = 0x00FEDE
RANGE_END = 0x010034
RANGE_SIZE = RANGE_END - RANGE_START  # 342 bytes

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
        "milestone": "M15.468",
        "title": "ROM Script Virtual Machine Opcodes $15, $16, $18, $19: Persistent Gater, Spawner & Ballistic Projectile Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "opcodes": [
            {
                "opcode": "0x15",
                "name": "ScriptOp_PersistentEntityGater",
                "start": "0x00FEDE",
                "end": "0x00FF1A",
                "size_bytes": 60,
                "description": (
                    "Reads bank selector +1(a0) (Bank 1 $FF0DBC, Bank 2 $FF0E1C, Bank 3 $FF0E2C) and persistent bit "
                    "+4(a0). Tests bit in bank: if set (already collected/defeated), skips 18-byte entity spawn instruction "
                    "via lea.l $12(a0), a0; rts. If clear, falls into Opcode 0x16 to spawn entity."
                ),
                "constants": {
                    "skip_bytes": 18
                }
            },
            {
                "opcode": "0x16",
                "name": "ScriptOp_SpawnPersistentEntity",
                "start": "0x00FF1A",
                "end": "0x00FF98",
                "size_bytes": 126,
                "description": (
                    "Allocates entity slot via $DA2A. Unpacks 18-byte record: entity type +2(a0) (constructor $8D06), "
                    "persistent bank +$A2 = +1(a0), persistent bit +$A4 = +4(a0), flags +$17, +$9E, +$A0, coordinates "
                    "(x, y) at +$08/+$0C, drop item +$8C, priority +$18, default HP = 10, animation $9D00, AI from $DF54 "
                    "for type >= $16. Advances script PC by 18 bytes."
                ),
                "constants": {
                    "record_size_bytes": 18,
                    "default_hp": 10
                }
            },
            {
                "opcode": "0x18",
                "name": "ScriptOp_SpawnScriptParticle",
                "start": "0x00FF98",
                "end": "0x00FFDE",
                "size_bytes": 70,
                "description": (
                    "Allocates particle/secondary slot via $D9F0 (yields frame if full). Sets flag +$4C = (a0)+, "
                    "type (a0)+ (constructor $8D06), init routine pointer +$56 = (a0)+, coordinates (x, y) at +$08/+$0C, "
                    "priority +$18, calls sprite setup $9AD6 setting VRAM tile +$14, and invokes custom init if non-zero."
                )
            },
            {
                "opcode": "0x19",
                "name": "ScriptOp_SpawnBallisticProjectile3D",
                "start": "0x00FFDE",
                "end": "0x010034",
                "size_bytes": 86,
                "description": (
                    "Allocates projectile slot via $D9F0 (yields frame if full). Sets flag +$4C = (a0)+, type (a0)+, "
                    "forces physics routine +$56 = $017A96 (ballistic trajectory update), coordinates (x, y), priority +$18, "
                    "and unpacks 3D velocity vector: vx = +$4E, vy = +$52, vz = +$54. Calls sprite setup $9AD6 and invokes $017A96."
                ),
                "constants": {
                    "ballistic_physics_routine": "0x017A96"
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
