#!/usr/bin/env python3
"""
Extract ROM Cutscene & Event Script Virtual Machine Core, 98-Opcode Dispatcher
& Camera Pan Engine (0x00F788..0x00F920, 408 bytes).

Extracts bytecode interpreter tick loop, jump table offsets and resolved targets,
camera pan mode table and direction handlers, and Opcode 0 frame exit.
Generates evidence JSON for Milestone M15.463.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_core_m15_463.json")

RANGE_START = 0x00F788
RANGE_END = 0x00F920
RANGE_SIZE = RANGE_END - RANGE_START  # 408 bytes

BYTECODE_TABLE_BASE = 0x00F7D0
BYTECODE_OPCODE_COUNT = 98  # 196 bytes / 2

CAMERA_PAN_TABLE_BASE = 0x00F8C8
CAMERA_PAN_MODE_COUNT = 4   # 8 bytes / 2

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Extract 98-entry bytecode jump table
    bytecode_table = []
    for op in range(BYTECODE_OPCODE_COUNT):
        entry_addr = BYTECODE_TABLE_BASE + op * 2
        rel_offset = struct.unpack(">h", rom[entry_addr:entry_addr + 2])[0]
        target_addr = entry_addr + rel_offset
        bytecode_table.append({
            "opcode": f"0x{op:02X}",
            "table_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_offset,
            "target_addr": f"0x{target_addr:06X}"
        })

    # Extract 4-entry camera pan mode jump table
    camera_pan_table = []
    pan_names = ["PanUp", "PanRight", "PanDown", "PanLeft"]
    for mode in range(CAMERA_PAN_MODE_COUNT):
        entry_addr = CAMERA_PAN_TABLE_BASE + mode * 2
        rel_offset = struct.unpack(">h", rom[entry_addr:entry_addr + 2])[0]
        target_addr = entry_addr + rel_offset
        camera_pan_table.append({
            "mode": mode,
            "name": pan_names[mode],
            "table_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_offset,
            "target_addr": f"0x{target_addr:06X}"
        })

    # Disassemble code regions
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # 1. Main tick & loop: 0x00F788..0x00F7D0
    loop_insns = []
    for insn in md.disasm(rom[0x00F788:0x00F7D0], 0x00F788):
        loop_insns.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # 2. Camera pan mode runner: 0x00F894..0x00F8C8
    pan_runner_insns = []
    for insn in md.disasm(rom[0x00F894:0x00F8C8], 0x00F894):
        pan_runner_insns.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # 3. Pan direction handlers: 0x00F8D0..0x00F91C
    pan_dir_insns = []
    for insn in md.disasm(rom[0x00F8D0:0x00F91C], 0x00F8D0):
        pan_dir_insns.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # 4. Opcode 0 frame exit: 0x00F91C..0x00F920
    exit_insns = []
    for insn in md.disasm(rom[0x00F91C:0x00F920], 0x00F91C):
        exit_insns.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.463",
        "title": "ROM Cutscene & Event Script Virtual Machine Core, 98-Opcode Dispatcher & Camera Pan Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "bytecode_table": {
            "base_address": f"0x{BYTECODE_TABLE_BASE:06X}",
            "opcode_count": BYTECODE_OPCODE_COUNT,
            "entries": bytecode_table
        },
        "camera_pan_table": {
            "base_address": f"0x{CAMERA_PAN_TABLE_BASE:06X}",
            "mode_count": CAMERA_PAN_MODE_COUNT,
            "entries": camera_pan_table
        },
        "subroutines": [
            {
                "name": "ScriptEngine_MainTickAndDispatchLoop",
                "start": "0x00F788",
                "end": "0x00F7D0",
                "size_bytes": 72,
                "description": (
                    "Checks script disabled flag bit 1 of $FF17B8 (exits if set). Checks delay timer $FF17B6 "
                    "(decrements and exits if > 0). If camera pan timer $FF17BD != 0, delegates to pan runner at $F894. "
                    "Otherwise loads script PC a0 from $FF17B2 and enters bytecode interpretation loop: reads opcode "
                    "byte *(a0)++, indexes 98-entry jump table at $F7D0, calls handler via jsr (a1), writes updated PC "
                    "to $FF17B2, and repeats until an opcode handler exits."
                )
            },
            {
                "name": "ScriptEngine_CameraPanModeRunner",
                "start": "0x00F894",
                "end": "0x00F8C8",
                "size_bytes": 52,
                "description": (
                    "Reads camera pan direction mode from $FF183A, indexes 4-entry mode table at $F8C8, and calls "
                    "directional displacement updater. Decrements pan timer $FF17BD; upon reaching 0, clears camera "
                    "lock flag $FF1982, and clears player movement suppression flags $FF19EC and $FF1A12."
                )
            },
            {
                "name": "ScriptEngine_DirectionalCameraPanHandlers",
                "start": "0x00F8D0",
                "end": "0x00F91C",
                "size_bytes": 76,
                "description": (
                    "Applies subpixel velocity from $FF183C to camera coordinates: "
                    "Mode 0 (Up): camera Y ($FF1A5E) -= speed << 16. "
                    "Mode 1 (Right): camera X ($FF1A5A) += speed << 16. "
                    "Mode 2 (Down): camera Y ($FF1A5E) += speed << 16. "
                    "Mode 3 (Left): camera X ($FF1A5A) -= speed << 16."
                )
            },
            {
                "name": "ScriptEngine_Opcode00_FrameYieldExit",
                "start": "0x00F91C",
                "end": "0x00F920",
                "size_bytes": 4,
                "description": "Opcode 0x00 implementation: executes addq.l #4, sp; rts, cleanly exiting interpreter loop."
            }
        ],
        "disassembly": {
            "main_loop": loop_insns,
            "pan_runner": pan_runner_insns,
            "pan_directions": pan_dir_insns,
            "frame_exit": exit_insns
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted VM Core ({RANGE_SIZE} bytes), 98 opcodes, 4 pan modes to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
