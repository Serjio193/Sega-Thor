#!/usr/bin/env python3
"""Extract and verify ROM Z80-safe VDP DMA executor & HUD portrait DMA (0x002632..0x002678, 0x0027A6..0x002896)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_vdp_dma_exec_m15_315.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # HUD portrait DMA (0x002632..0x002678)
    assert u32(rom, 0x00263C) == 0x00FF183E
    assert u32(rom, 0x002642) == 0x00FF183F
    assert u32(rom, 0x002660) == 0x000AA3EC
    assert u32(rom, 0x002666) == 0x00FF1892
    assert u16(rom, 0x00266E) == 0xF1C0
    assert u16(rom, 0x002674) == 0x0060

    # Z80-safe VDP DMA (0x0027A6..0x0027FA & 0x0027FC..0x002896)
    assert u16(rom, 0x0027A8) == 0x007C  # ori.w #$700, sr
    assert u16(rom, 0x0027AA) == 0x0700
    assert u32(rom, 0x0027B0) == 0x00A11100
    assert u32(rom, 0x0027C6) == 0x00FF0BF4
    assert u32(rom, 0x002806) == 0x00A11100
    assert u32(rom, 0x00281C) == 0x00FF0BF4

    # VDP DMA register prefixes in 0x002826..0x002856
    dma_regs = [
        u16(rom, 0x002828),  # 0x9400
        u16(rom, 0x002832),  # 0x9300
        u16(rom, 0x00283C),  # 0x9500
        u16(rom, 0x002846),  # 0x9600
        u16(rom, 0x002850),  # 0x9700
    ]
    assert dma_regs == [0x9400, 0x9300, 0x9500, 0x9600, 0x9700]

    return {
        "milestone": "M15.315",
        "title": "ROM Z80-Safe VDP DMA Executor & HUD Portrait DMA",
        "routines": {
            "hud_portrait_dma": {
                "start": "0x002632",
                "end": "0x002678",
                "requested_portrait_ram": "0x00FF183E",
                "cached_portrait_ram": "0x00FF183F",
                "portrait_table_rom": "0x000AA3EC",
                "portrait_stride_bytes": 96,
                "vram_dst": "0xF1C0",
                "len_words": "0x0060",
                "dma_desc_ptr_ram": "0x00FF1892",
            },
            "vdp_dma_exec_regs": {"start": "0x0027A6", "end": "0x0027FA"},
            "vdp_dma_exec_struct": {"start": "0x0027FC", "end": "0x002896"},
        },
        "hardware_ports": {
            "z80_busreq": "0x00A11100",
            "vdp_data": "0x00C00000",
            "vdp_ctrl": "0x00C00004",
            "vdp_reg1_shadow_ram": "0x00FF0BF4",
            "vdp_reg1_dma_enable_bit": 4,
            "irq_mask_sr": "0x0700",
        },
        "dma_reg_prefixes": [f"0x{r:04X}" for r in dma_regs],
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
