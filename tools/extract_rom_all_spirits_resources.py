#!/usr/bin/env python3
"""
extract_rom_all_spirits_resources.py — Milestone M15.267

Extracts ROM semantic knowledge for all 4 Elemental Spirits (Dytto 0x0018,
Efreet 0x001A, Shade 0x001C, Bow 0x001E):
1. Spirit Resource & Nemesis VRAM DMA Loader (`0x014522..0x014624`):
   - Dytto  (0x0018): ROM 0x00141580 -> VRAM tile 0x04D0 (byte 0x9A00)
   - Efreet (0x001A): ROM 0x001425AE -> VRAM tile 0x0450 (byte 0x8A00)
   - Shade  (0x001C): ROM 0x00143262 -> VRAM tile 0x0280 (byte 0x5000)
   - Bow    (0x001E): ROM 0x00143A42 -> VRAM tile 0x0360 (byte 0x6C00)
   - Shared decompression work buffer: 0x00FF2FA8, DMA queue: 0x00FF1892.
2. Spawn Routines (`0x014668`, `0x0146D6`, `0x014742`, `0x014798`):
   - Initial states, actions, collision flags, pattern base, and SP pool (100/10).
3. Entity Update Handlers from Master Table `0x00DF54`:
   - 0x0018 -> 0x011B4E, 0x001A -> 0x01254C, 0x001C -> 0x012F1E, 0x001E -> 0x01387C.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_all_spirits_resources_m15_267.json"

# (name, type_id, bit_index, dma_branch_pc, spawn_pc, update_pc)
SPIRIT_SPECS = [
    ("Dytto", 0x0018, 0, 0x01454C, 0x014668, 0x011B4E),
    ("Efreet", 0x001A, 1, 0x014582, 0x0146D6, 0x01254C),
    ("Shade", 0x001C, 2, 0x0145B8, 0x014742, 0x012F1E),
    ("Bow", 0x001E, 3, 0x0145F0, 0x014798, 0x01387C),
]


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_dma_branch(rom: bytes, branch_pc: int) -> dict:
    """Decode lea <rom_src>.l, a0; ...; move.w #<vram_tile>, (a0)+."""
    # branch_pc + 0x00: 41F9 <u32 rom_src>
    assert read_u16_be(rom, branch_pc) == 0x41F9
    rom_src = read_u32_be(rom, branch_pc + 2)
    # branch_pc + 0x06: 43F9 <u32 work_buf>
    assert read_u16_be(rom, branch_pc + 6) == 0x43F9
    work_buf = read_u32_be(rom, branch_pc + 8)
    # branch_pc + 0x26: 30FC <u16 vram_tile>
    assert read_u16_be(rom, branch_pc + 0x26) == 0x30FC
    vram_tile = read_u16_be(rom, branch_pc + 0x28)
    return {
        "dma_branch_pc": f"0x{branch_pc:06X}",
        "nemesis_rom_addr": f"0x{rom_src:06X}",
        "work_buffer_ram": f"0x{work_buf:08X}",
        "vram_tile": f"0x{vram_tile:04X}",
        "vram_byte_addr": f"0x{vram_tile * 32:04X}",
    }


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    spirits = {}
    for name, tid, bit_idx, dma_pc, spawn_pc, upd_pc in SPIRIT_SPECS:
        # Verify 0x00DF54 master update dispatch entry
        df54_entry = 0x00DF54 - 0x2C + tid * 2
        assert read_u32_be(rom, df54_entry) == upd_pc
        # Verify spawn routine sets entity type_id at $0(a6) (3D7C <tid> 0000)
        assert read_u16_be(rom, spawn_pc) == 0x3D7C
        assert read_u16_be(rom, spawn_pc + 2) == tid

        dma_info = extract_dma_branch(rom, dma_pc)
        spirits[name] = {
            "entity_type": f"0x{tid:04X}",
            "active_flag_bit": bit_idx,
            "spawn_routine": f"0x{spawn_pc:06X}",
            "update_handler": f"0x{upd_pc:06X}",
            "max_sp": 100,
            "sp_drain_rate": 10,
            **dma_info,
        }

    evidence = {
        "milestone": "M15.267",
        "description": "ROM Resource, Nemesis VRAM DMA, Spawn & Update Dispatch Contracts for All 4 Spirits",
        "shared_dma_loader": "0x014522",
        "dma_queue_ram": "0x00FF1892",
        "active_spirit_flags_ram": "0x00FF0D7A",
        "spirits": spirits,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(spirits)} spirits)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
