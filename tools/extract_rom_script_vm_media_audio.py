#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $09, $0C, $0D, $1A..$1D:
Media, Audio, Palette & Screen Shake Subsystem (0x010034..0x010140, 268 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.469.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_media_audio_m15_469.json")

RANGE_START = 0x010034
RANGE_END = 0x010140
RANGE_SIZE = RANGE_END - RANGE_START  # 268 bytes

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
        "milestone": "M15.469",
        "title": "ROM Script Virtual Machine Opcodes $09, $0C, $0D, $1A..$1D: Media, Audio, Palette & Screen Shake Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "opcodes": [
            {
                "opcode": "0x09",
                "name": "ScriptOp_SpawnItemDrop",
                "start": "0x010034",
                "end": "0x010086",
                "size_bytes": 82,
                "description": (
                    "Allocates Pool 2 item slot via $D9BE (yields frame on full). Instantiates Type 8 item entity "
                    "via $8D06. Sets category +$32 = d1 >> 8 and ID +$52 = d1 & $FF, coordinates (x, y), sprite setup $9AD6, "
                    "launch bit +$39 bit 0, and PRNG lifetime 0..63 frames +$02 via $E268."
                ),
                "constants": {
                    "item_entity_type": 8
                }
            },
            {
                "opcode": "0x0C",
                "name": "ScriptOp_LoadPaletteBuffer",
                "start": "0x010086",
                "end": "0x0100AA",
                "size_bytes": 36,
                "description": (
                    "Reads palette index d0 = (a0)+, saves to $FF16F8. Copies 32 bytes from ROM table $5D046 + d0 * 32 "
                    "into RAM palette buffer $FF136C."
                ),
                "constants": {
                    "rom_palette_table": "0x05D046",
                    "ram_palette_buffer": "0xFF136C"
                }
            },
            {
                "opcode": "0x0D",
                "name": "ScriptOp_LoadAndApplyPaletteCram",
                "start": "0x0100AA",
                "end": "0x0100D6",
                "size_bytes": 44,
                "description": (
                    "Reads palette index d0 = (a0)+, copies 32 bytes from $5D046 + d0 * 32 into $FF136C, and sets "
                    "CRAM transfer request bit 0 of $FF164D."
                ),
                "constants": {
                    "cram_request_flag": "0xFF164D"
                }
            },
            {
                "opcode": "0x1A",
                "name": "ScriptOp_PlaySfx",
                "start": "0x0100D6",
                "end": "0x0100E4",
                "size_bytes": 14,
                "description": "Reads sound parameters d1, d2, d0 from script stream and jumps to SFX dispatcher $00E78E."
            },
            {
                "opcode": "0x1B",
                "name": "ScriptOp_PlayBgmMusic",
                "start": "0x0100E4",
                "end": "0x0100EE",
                "size_bytes": 10,
                "description": "Reads 32-bit music command d0 from script stream and jumps to Z80 music port $060004."
            },
            {
                "opcode": "0x1C",
                "name": "ScriptOp_TriggerScreenShake",
                "start": "0x0100EE",
                "end": "0x0100F8",
                "size_bytes": 10,
                "description": "Sets screen shake / quake duration counter $FF17C4 = (a0)+."
            },
            {
                "opcode": "0x1D",
                "name": "ScriptOp_DispatchChannelEffect",
                "start": "0x0100F8",
                "end": "0x010140",
                "size_bytes": 72,
                "description": (
                    "Reads channel selector d0 = (a0)+, chooses channel buffer $FF1716 vs $FF173E, and invokes "
                    "plane effect handler $00DB64, $00DB68, or $00DBCA. Advances script PC by 12 bytes."
                )
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
