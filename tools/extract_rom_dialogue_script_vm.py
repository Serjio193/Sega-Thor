#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Dialogue Script Virtual Machine &
Message Window Presentation Controller routines (0x00C2EC..0x00C566, 634 bytes)
and the Dialogue String Pointer Table at 0x051514 (116 entries).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_dialogue_script_vm_m15_428.json"

START_ADDR = 0x00C2EC
END_ADDR = 0x00C566
ROUTINE_SIZE = END_ADDR - START_ADDR  # 634 bytes

STRING_TABLE_ADDR = 0x051514
NUM_STRINGS = 0x62  # 98 dialogue strings (0x00..0x61). 0x62 contains Japanese placeholder 'Kari Serihu' text.

OP_TERMINATE = 0x00
OP_NEWLINE = 0x01
OP_PROMPT_NEWLINE = 0x02
OP_DELAY_FRAMES = 0x03
OP_PROMPT_WAIT = 0x05
OP_CLEAR_PAGE = 0x06
OP_PROMPT_CLEAR_PAGE = 0x07


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    # Extract string table
    strings = []
    for i in range(NUM_STRINGS):
        offset = struct.unpack_from(">h", rom, STRING_TABLE_ADDR + i * 2)[0]
        str_addr = STRING_TABLE_ADDR + i * 2 + offset
        raw = []
        pos = str_addr
        while pos < len(rom) and rom[pos] != 0:
            raw.append(rom[pos])
            pos += 1
        # Include terminating 0
        raw.append(0)

        # Parse tokens
        tokens = []
        idx = 0
        while idx < len(raw):
            b = raw[idx]
            idx += 1
            if b >= 0x20:
                char_str = chr(b) if 0x20 <= b <= 0x7E else f"\\x{b:02x}"
                if tokens and tokens[-1]["type"] == "text":
                    tokens[-1]["text"] += char_str
                else:
                    tokens.append({"type": "text", "text": char_str})
            elif b == OP_TERMINATE:
                tokens.append({"type": "opcode", "op": "TERMINATE", "code": 0})
                break
            elif b == OP_NEWLINE:
                tokens.append({"type": "opcode", "op": "NEWLINE", "code": 1})
            elif b == OP_PROMPT_NEWLINE:
                tokens.append({"type": "opcode", "op": "PROMPT_NEWLINE", "code": 2})
            elif b == OP_DELAY_FRAMES:
                param = raw[idx] if idx < len(raw) else 0
                idx += 1
                tokens.append({"type": "opcode", "op": "DELAY_FRAMES", "code": 3, "frames": param})
            elif b == OP_PROMPT_WAIT:
                tokens.append({"type": "opcode", "op": "PROMPT_WAIT", "code": 5})
            elif b == OP_CLEAR_PAGE:
                tokens.append({"type": "opcode", "op": "CLEAR_PAGE", "code": 6})
            elif b == OP_PROMPT_CLEAR_PAGE:
                tokens.append({"type": "opcode", "op": "PROMPT_CLEAR_PAGE", "code": 7})
            else:
                tokens.append({"type": "opcode", "op": f"UNKNOWN_{b:02X}", "code": b})

        strings.append({
            "id": i,
            "address": f"0x{str_addr:06X}",
            "offset": offset,
            "byte_len": len(raw),
            "tokens": tokens,
        })

    evidence = {
        "routine": "dialogue_script_vm_and_presentation_controller",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "display_dialogue_message": f"0x{START_ADDR:06X}",
            "dialogue_box_exec_window": "0x00C326",
            "dialogue_wait_button": "0x00C536",
            "dialogue_close_window": "0x00C45C",
        },
        "string_table": {
            "address": f"0x{STRING_TABLE_ADDR:06X}",
            "count": NUM_STRINGS,
        },
        "opcodes": {
            "0x00": "OP_TERMINATE",
            "0x01": "OP_NEWLINE",
            "0x02": "OP_PROMPT_NEWLINE",
            "0x03": "OP_DELAY_FRAMES",
            "0x05": "OP_PROMPT_WAIT",
            "0x06": "OP_CLEAR_PAGE",
            "0x07": "OP_PROMPT_CLEAR_PAGE",
        },
        "strings": strings,
        "code_hex": routine_bytes.hex(),
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted evidence: {ROUTINE_SIZE} bytes, {NUM_STRINGS} strings -> {OUTPUT_JSON}")
    return evidence


if __name__ == "__main__":
    extract_evidence()
