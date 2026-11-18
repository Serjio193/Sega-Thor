#!/usr/bin/env python3
"""
Extract ROM Entity AI Mass Confusion, Palette Transfer, Ali Range Tracker
& Impact Knockback Dispatcher (0x011276..0x0114AE, 568 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.481.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_knockback_m15_481.json")

RANGE_START = 0x011276
RANGE_END = 0x0114AE
RANGE_SIZE = RANGE_END - RANGE_START  # 568 bytes

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
        "milestone": "M15.481",
        "title": "ROM Entity AI Mass Confusion, Palette Transfer, Ali Range Tracker & Impact Knockback Dispatcher",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "AI_ApplyFearConfusionToAllEnemiesPool2",
                "start": "0x011276",
                "end": "0x0112CC",
                "size_bytes": 86,
                "description": "Scans 17 slots of Pool 2 at $FF1CD8, filters living enemies excluding self a6, inverts facing bit 1 (bchg.b #1), stores to +$84, calls 0x00E4D0, and sets +$86 to $CFFF."
            },
            {
                "name": "AI_FindSlotWithTypePool2",
                "start": "0x0112CC",
                "end": "0x0112F6",
                "size_bytes": 42,
                "description": "Searches 17 slots of Pool 2 for an entity slot matching type d0.w. Returns d0=0 if found, or d0=-1 ($FF) if not found."
            },
            {
                "name": "AI_ProbeEntityAllocationEviction",
                "start": "0x0112F6",
                "end": "0x011310",
                "size_bytes": 26,
                "description": "Calls slot probe 0x00DA72 and inverts Carry flag return convention: Carry clear becomes Carry set (failure), Carry set becomes Carry clear (success)."
            },
            {
                "name": "Gfx_TransferCRAMPaletteLine_Lines",
                "start": "0x011310",
                "end": "0x011378",
                "size_bytes": 104,
                "description": "Copies 16 color words (32 bytes) from (a1)+ into CRAM palette buffers $FF136C or $FF138C and asserts bit 0 of CRAM transfer request flag $FF164D."
            },
            {
                "name": "AI_ComputeAliDistanceAndDominantFacing",
                "start": "0x011378",
                "end": "0x0113CC",
                "size_bytes": 84,
                "description": "Computes delta vector (d1, d2) to Player Ali ($FF19F0/$FF19F4), determines dominant axis |dx| vs |dy|, returns 4-way facing (0=Up, 1=Right, 2=Down, 3=Left) in d0, and dominant distance in d5."
            },
            {
                "name": "AI_TriggerTargetImpactKnockback",
                "start": "0x0113CC",
                "end": "0x0114AE",
                "size_bytes": 226,
                "description": "Tests attack hitbox against target a0 via 0x00B926, links attacker a6 into +$7E(a0), sets impact state +$82=3, +$83=6, upward launch velocity +$56=$20000, projects directional knockback velocities via 0x00E2D4, plays impact SFX $80022C via 0x00E7FC, and returns Carry set."
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
