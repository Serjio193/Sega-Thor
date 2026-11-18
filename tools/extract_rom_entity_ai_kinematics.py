#!/usr/bin/env python3
"""
Extract ROM Entity AI Target Angle Projector, Hover Bobbing Elevation Tracker
& Multi-Segment Kinematics Engine (0x01108C..0x011276, 490 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.480.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_kinematics_m15_480.json")

RANGE_START = 0x01108C
RANGE_END = 0x011276
RANGE_SIZE = RANGE_END - RANGE_START  # 490 bytes

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
        "milestone": "M15.480",
        "title": "ROM Entity AI Target Angle Projector, Hover Bobbing Elevation Tracker & Multi-Segment Kinematics Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "Math_TargetAngleAndVelocityProjector_Signed",
                "start": "0x01108C",
                "end": "0x011102",
                "size_bytes": 118,
                "description": "Calculates relative delta to target (d0, d1) from entity +$8/+$C, evaluates quadrant and octant arctangent via 0x011102, scales velocities by asr.l #7, and projects 2D sin/cos components from ROM table $5D706 into (d0, d1)."
            },
            {
                "name": "Math_ArctangentLookup_Octant_Helper",
                "start": "0x011102",
                "end": "0x011128",
                "size_bytes": 38,
                "description": "Computes (dy << 8) / dx and scans 8-entry tangent threshold table at ROM $5D906, returning angle in steps of 8 (or 0x40 on dx=0)."
            },
            {
                "name": "AI_UpdateEntityHoverBobbingAndElevationTracking",
                "start": "0x011128",
                "end": "0x011176",
                "size_bytes": 78,
                "description": "Increments oscillation phase counter +$BB(a6), alternates adding/subtracting $8000 to hover offset +$A2(a6) based on bit 4, targets Ali elevation $FF19FC with base offset +$14(a6), and nudges elevation +$10(a6) by +/- 1 unit towards target with clamp >= 0."
            },
            {
                "name": "AI_HookSurfaceContactHandler1",
                "start": "0x011176",
                "end": "0x0111A8",
                "size_bytes": 50,
                "description": "Registers trampoline callback 0x01118A in $FF1940 and invokes surface contact dispatcher 0x00BCE2; callback preserves facing, turns towards Ali via 0x010752, calls 0x00BEBA, and restores facing."
            },
            {
                "name": "AI_HookSurfaceContactHandler2",
                "start": "0x0111A8",
                "end": "0x0111E0",
                "size_bytes": 56,
                "description": "Registers trampoline callback 0x0111BC in $FF1940 and invokes surface contact dispatcher 0x00BCE2; callback preserves facing, turns towards Ali, calls 0x00BEBA, sets bit 5 of +$86(a0), and restores facing."
            },
            {
                "name": "AI_CountVacantSlotsPool2",
                "start": "0x0111E0",
                "end": "0x011206",
                "size_bytes": 38,
                "description": "Iterates through 17 slots of Pool 2 at $FF1CD8 (stride 188 bytes) and counts vacant slots where slot type +$0(a0) == 0, returning count in d0.w."
            },
            {
                "name": "AI_UpdateMultiSegmentEntityKinematics",
                "start": "0x011206",
                "end": "0x011276",
                "size_bytes": 112,
                "description": "Updates multi-segment child entity offsets from displacement table +$A8(a6) indexed by +$A4(a6)*2; if parent pointer +$A6(a6) is set, shifts 8-word history buffer in parent and records child-to-parent anchor deltas in +$AA/+$AB(a4)."
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
