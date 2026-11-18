#!/usr/bin/env python3
"""
extract_rom_effect_spawner_jitter.py — Extract ROM Dynamic Effect Type $34 Spawner,
Audio Rate Limiter & PRNG Shake-Jitter Engine
(0x00E90E..0x00EA76, 360 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_effect_spawner_jitter_m15_453.json")

START_ADDR = 0x00E90E
END_ADDR = 0x00EA76


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
        "routine": "rom_effect_spawner_jitter",
        "description": (
            "ROM Dynamic Effect Type $34 Spawner, Audio Rate Limiter "
            "& PRNG Shake-Jitter Engine"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "spawn_effect_type34_normal": {
                "start": "0x00E90E",
                "end": "0x00E93A",
                "callers_count": 8,
                "notes": (
                    "Allocates entity slot from pool 2 via 0x00DA2A, "
                    "initializes 3D coordinates (d1, d2, d3), and calls init."
                )
            },
            "spawn_effect_type34_priority": {
                "start": "0x00E93A",
                "end": "0x00E96C",
                "callers_count": 6,
                "notes": (
                    "Allocates slot, initializes 3D coordinates, and sets "
                    "high-priority bit on sprite priority word (ori.w #$8000, $18(a6))."
                )
            },
            "init_effect_type34_and_sfx": {
                "start": "0x00E96C",
                "end": "0x00E9DE",
                "callers_count": 2,
                "notes": (
                    "Sets type code #$34 at +$00, invokes constructor 0x008D06, "
                    "sets default priority #$55E, anchors base position (+$4E, +$52), "
                    "sets lifetime timer +$9E. Checks frame timer $FF199C vs $FF1944; "
                    "if elapsed >= 7 frames, triggers spatial SFX #$90021B and #$600382."
                )
            },
            "cutscene_invisibility_gate": {
                "start": "0x00E9DE",
                "end": "0x00EA04",
                "callers_count": 2,
                "notes": (
                    "Checks if entity slot 0 at $FF1AA4 has cutscene type #$1E; "
                    "if so, sets bit 1 on +$36 (invisible) and bit 2 on +$38 (intangible)."
                )
            },
            "tick_effect_type34_prng_jitter": {
                "start": "0x00EA04",
                "end": "0x00EA76",
                "callers_count": 4,
                "notes": (
                    "If active hit bit 1 on +$37 set, decrements lifetime timer +$9E(a6). "
                    "When 0, despawns entity slot. While active, calls PRNG 0x00E268 "
                    "to generate [-16..+15] jitter offsets around anchor position (+$4E, +$52), "
                    "producing vibrating ground dust shake, and triggers spatial audio."
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
