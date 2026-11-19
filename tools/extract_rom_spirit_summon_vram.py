#!/usr/bin/env python3
"""
Extract ROM Companion Spirit VRAM Graphics Decompressor & DMA Uploader,
Special Lock Reset & 4-Spirit Summon Dispatcher (0x014522..0x0147E0, 702 bytes).

Disassembles exact M68K instructions with Capstone, validates opcodes/constants/DMA
descriptors, and generates evidence JSON for Milestone M15.502.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_spirit_summon_vram_m15_502.json"

RANGE_START = 0x014522
RANGE_END = 0x0147E0
RANGE_SIZE = RANGE_END - RANGE_START  # 702 bytes

ROUTINES = [
    {
        "name": "Spirits_DecompressAndQueueVramGfx",
        "start": "0x014522",
        "end": "0x014628",
        "size_bytes": 0x014628 - 0x014522,
        "description": (
            "Reads spirit entity type from $FF1AA4 and dispatches to decompress sprite graphics "
            "via 0x003820 into $FF2FA8, then queues an 8-byte VDP DMA descriptor ($007F97D4, "
            "$F500, vram_tile) at $FF1892 under SR |= $0700: Dytto 0x0018 ($141580 -> $04D0), "
            "Efreet 0x001A ($1425AE -> $0450), Shade 0x001C ($143262 -> $0280), "
            "Bow 0x001E ($143A42 -> $0360)."
        ),
    },
    {
        "name": "Spirits_ClearOverlayStateAndDispatch",
        "start": "0x014628",
        "end": "0x014658",
        "size_bytes": 0x014658 - 0x014628,
        "description": (
            "Clears $FF199E (hold counter) and $FF19A1 (double-tap timer), sets $FF19A0 = $FF "
            "(button A suppression sentinel), and dispatches on $FF19A6 (0 = Dytto 0x014658, "
            "1 = Efreet 0x0146C0, 2 = Shade 0x014732, 3 = Bow 0x014798; >= 4 returns)."
        ),
    },
    {
        "name": "Spirits_SpawnDytto",
        "start": "0x014658",
        "end": "0x0146C0",
        "size_bytes": 0x0146C0 - 0x014658,
        "description": (
            "Initializes Dytto (type 0x0018) via 0x008D06, sets +$16=2, +$18=$04F8, "
            "+$88=+$8A=100, +$8C=+$8E=10, preserves coordinates (+$08, +$0C, +$10, +$14), "
            "sets State $02 (+$04=2, +$2A=4), calls 0x014522, and clears spawn lock $FF19A7."
        ),
    },
    {
        "name": "Spirits_SpawnEfreet",
        "start": "0x0146C0",
        "end": "0x014732",
        "size_bytes": 0x014732 - 0x0146C0,
        "description": (
            "Initializes Efreet (type 0x001A) with Z offset +$00080000 via 0x008D06, "
            "sets +$18=$04F8, +$88=+$8A=100, +$8C=+$8E=10, preserves XY/Z/ground_z, "
            "sets State $04 (+$04=4, +$2A=4, +$16=2, +$A6=a6), calls 0x014522, and clears $FF19A7."
        ),
    },
    {
        "name": "Spirits_SpawnShade",
        "start": "0x014732",
        "end": "0x014798",
        "size_bytes": 0x014798 - 0x014732,
        "description": (
            "Initializes Shade (type 0x001C) via 0x008D06, sets +$18=$04F8, +$88=+$8A=100, "
            "+$8C=+$8E=10, preserves coordinates (+$08, +$0C, +$10, +$14), sets State $02 "
            "(+$04=2, +$2A=0, +$16=0), calls 0x014522, and clears spawn lock $FF19A7."
        ),
    },
    {
        "name": "Spirits_SpawnBow",
        "start": "0x014798",
        "end": "0x0147E0",
        "size_bytes": 0x0147E0 - 0x014798,
        "description": (
            "Sets entity type +$00=0x001E, calls 0x014522, invokes bow root emerge sub-entity "
            "at 0x0147E0 with XYZ coords, sets State $0E (+$04=$0E, +$2A=$1E, +$16=0, +$18=$07A8, "
            "+$36:0=0), plays SFX #$0080039F via 0x00E7FC, and clears spawn lock $FF19A7."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM sources, VRAM tiles, DMA commands, and summon constants."""
    assert u32_be(rom, 0x014528) == 0x00FF1AA4  # move.w $ff1aa4.l, d0
    assert u32_be(rom, 0x01454E) == 0x00141580  # Dytto compressed GFX
    assert u16_be(rom, 0x014570) == 0xF500      # Dytto DMA length
    assert u16_be(rom, 0x014574) == 0x04D0      # Dytto VRAM tile
    assert u32_be(rom, 0x014584) == 0x001425AE  # Efreet compressed GFX
    assert u16_be(rom, 0x0145A6) == 0xF500      # Efreet DMA length
    assert u16_be(rom, 0x0145AA) == 0x0450      # Efreet VRAM tile
    assert u32_be(rom, 0x0145BA) == 0x00143262  # Shade compressed GFX
    assert u16_be(rom, 0x0145DC) == 0xF500      # Shade DMA length
    assert u16_be(rom, 0x0145E0) == 0x0280      # Shade VRAM tile
    assert u32_be(rom, 0x0145F2) == 0x00143A42  # Bow compressed GFX
    assert u16_be(rom, 0x014614) == 0xF500      # Bow DMA length
    assert u16_be(rom, 0x014618) == 0x0360      # Bow VRAM tile
    assert u32_be(rom, 0x01463E) == 0x00FF19A6  # move.b $ff19a6.l, d0
    assert u32_be(rom, 0x0146CE) == 0x00080000  # addi.l #$80000, d2
    assert u16_be(rom, 0x0147C4) == 0x07A8      # move.w #$7a8, $18(a6)
    assert u32_be(rom, 0x0147D0) == 0x0080039F  # move.l #$80039f, d0


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    disasm_bytes = 0
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        disasm_bytes += insn.size

    assert disasm_bytes == RANGE_SIZE, f"Disassembled {disasm_bytes} != {RANGE_SIZE}"

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [
            ins for ins in instructions
            if r_start <= int(ins["address"], 16) < r_end
        ]
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        }
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.502",
        "title": "ROM Companion Spirit VRAM Graphics Decompressor & DMA Uploader, Special Lock Reset & 4-Spirit Summon Dispatcher",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "spirits_vram_table": {
            "0x0018": {
                "name": "Dytto",
                "spirit_type": "0x0018",
                "rom_art_address": "0x00141580",
                "vram_tile": "0x04D0",
                "dma_length_word": "0xF500",
                "dma_cmd_word": "0x007F97D4",
            },
            "0x001A": {
                "name": "Efreet",
                "spirit_type": "0x001A",
                "rom_art_address": "0x001425AE",
                "vram_tile": "0x0450",
                "dma_length_word": "0xF500",
                "dma_cmd_word": "0x007F97D4",
            },
            "0x001C": {
                "name": "Shade",
                "spirit_type": "0x001C",
                "rom_art_address": "0x00143262",
                "vram_tile": "0x0280",
                "dma_length_word": "0xF500",
                "dma_cmd_word": "0x007F97D4",
            },
            "0x001E": {
                "name": "Bow",
                "spirit_type": "0x001E",
                "rom_art_address": "0x00143A42",
                "vram_tile": "0x0360",
                "dma_length_word": "0xF500",
                "dma_cmd_word": "0x007F97D4",
            },
        },
        "summon_dispatcher": {
            "source_ram": "0x00FF19A6",
            "branch_targets": {
                "0": {"spirit": "Dytto", "type": "0x0018", "target_address": "0x014658"},
                "1": {"spirit": "Efreet", "type": "0x001A", "target_address": "0x0146C0"},
                "2": {"spirit": "Shade", "type": "0x001C", "target_address": "0x014732"},
                "3": {"spirit": "Bow", "type": "0x001E", "target_address": "0x014798"},
            },
            "fallback_rts": "0x014656",
        },
        "constants": {
            "dma_cmd_src_word": "0x007F97D4",
            "dma_length_word": "0xF500",
            "spirit_max_hp_sp": 100,
            "spirit_init_drain": 10,
            "spirit_base_vram_attr": "0x04F8",
            "efreet_spawn_z_offset": "0x00080000",
            "bow_summon_vram_attr": "0x07A8",
            "bow_summon_sfx": "0x0080039F",
        },
        "ram_addresses": {
            "spirit_type_slot": "0x00FF1AA4",
            "dma_queue_ptr": "0x00FF1892",
            "spirit_hold_counter": "0x00FF199E",
            "spirit_prev_button_a": "0x00FF19A0",
            "spirit_dtap_timer": "0x00FF19A1",
            "spirit_summon_index": "0x00FF19A6",
            "spirit_spawn_lock": "0x00FF19A7",
            "gfx_decomp_buffer": "0x00FF2FA8",
        },
        "external_targets": {
            "nemesis_decompress": "0x003820",
            "entity_type_init": "0x008D06",
            "play_sfx": "0x00E7FC",
            "bow_root_emerge": "0x0147E0",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    insn_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{insn_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
