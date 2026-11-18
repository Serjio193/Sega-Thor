#!/usr/bin/env python3
"""
extract_rom_boss_stomp_engine.py — Extract ROM Boss Entity Type $3A Spawner,
Ground Stomp & Combat State Engine (Part 1)
(0x00ED04..0x00EE94, 400 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_stomp_engine_m15_455.json")

START_ADDR = 0x00ED04
END_ADDR = 0x00EE94


def extract_instructions_and_evidence():
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
        "routine": "rom_boss_stomp_engine",
        "description": (
            "ROM Boss Entity Type $3A Spawner, Ground Stomp & Combat State Engine (Part 1)"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "spawn_init_boss_type3a": {
                "start": "0x00ED04",
                "end": "0x00EDAA",
                "callers_count": 4,
                "notes": (
                    "Sets boss active flag $FF198F = 0xFF, clears Ali $6(a4), "
                    "initializes type #$3A at +$00 via constructor 0x008D06, adjusts elevation "
                    "-0x40000, inherits Ali sprite priority, sets animation table 0x009D00, "
                    "sets combat stats (+$A8=0x8007, +$A6=2, state=8, timer=8), "
                    "triggers roar SFX #$B00225 and ground shake #$600382 via 0x00E7E8, "
                    "and sets screen shake timer $FF17C4 = 4."
                )
            },
            "boss_type3a_phase_and_damage_evaluator": {
                "start": "0x00EDAA",
                "end": "0x00EDF2",
                "callers_count": 2,
                "notes": (
                    "Checks phase change byte +$66(a6); if set, updates phase +$A6, "
                    "timers, elevation, and resets state +$04 to 0. Checks incoming damage +$86; "
                    "if heavy knockback (& 0xC000 != 0), bumps elevation +$10 += 1."
                )
            },
            "boss_type3a_state_jump_table": {
                "start": "0x00EDF2",
                "end": "0x00EE08",
                "callers_count": 1,
                "notes": "Dispatches state index +$04(a6) via PC-relative word jump table at 0x00EDFE."
            },
            "boss_type3a_state3_stomp_attack": {
                "start": "0x00EE08",
                "end": "0x00EE34",
                "callers_count": 1,
                "notes": (
                    "Calls helper 0x00EF5E, clears priority bit 11, sets scan callback 0x00F054, "
                    "invokes combat scan 0x00BC98. If hit registered, branches to recovery 0x00F038."
                )
            },
            "boss_type3a_state4_ground_quake": {
                "start": "0x00EE34",
                "end": "0x00EE94",
                "callers_count": 1,
                "notes": (
                    "Bumps elevation, calls helper 0x00EF5E, triggers combat scan 0x00BC98. "
                    "On frame match (+$30 == 8 and +$2E == 0): sets quake flag bit 4 on +$39, "
                    "triggers screen shake $FF17C4 = 4 and roar SFX #$B00225."
                )
            }
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions_and_evidence()
