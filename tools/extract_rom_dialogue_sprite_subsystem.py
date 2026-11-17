#!/usr/bin/env python3
"""Extract and verify ROM dialogue & HUD sprite SAT compositor and allocation subsystem (0x00A342..0x00A524)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_dialogue_sprite_subsystem_m15_411.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00A342: Dialogue / HUD sprite compositor entry
    assert u16(rom, 0x00A342) == 0x4BF9          # lea.l $ff13cc.l, a5
    assert u32(rom, 0x00A342 + 2) == 0x00FF13CC
    assert u16(rom, 0x00A348) == 0xDAF9          # adda.w $ff188c.l, a5
    assert u32(rom, 0x00A348 + 2) == 0x00FF188C
    assert u16(rom, 0x00A34E) == 0x3A39          # move.w $ff188a.l, d5
    assert u32(rom, 0x00A34E + 2) == 0x00FF188A

    # 0x00A406: Interrupt mask protection for DMA queue
    assert u16(rom, 0x00A406) == 0x007C          # ori.w #$700, sr
    assert u16(rom, 0x00A406 + 2) == 0x0700
    assert u16(rom, 0x00A420) == 0x027C          # andi.w #$f9ff, sr
    assert u16(rom, 0x00A420 + 2) == 0xF9FF
    assert u16(rom, 0x00A436) == 0x4E75          # rts

    # 0x00A4C8: Static reservation pass
    assert u16(rom, 0x00A4C8) == 0x0679          # addi.w #$30, $ff188c.l
    assert u16(rom, 0x00A4C8 + 2) == 0x0030
    assert u32(rom, 0x00A4C8 + 4) == 0x00FF188C
    assert u16(rom, 0x00A4D0) == 0x5C79          # addq.w #6, $ff188a.l
    assert u32(rom, 0x00A4D0 + 2) == 0x00FF188A

    # Extract template tables (72 bytes each = 9 sprite attributes)
    table_a438 = []
    for i in range(0, 72, 8):
        y, link_size, attr, x = struct.unpack_from(">HHHH", rom, 0x00A438 + i)
        table_a438.append({"y": y, "link_size": link_size, "attr": attr, "x": x})

    table_a480 = []
    for i in range(0, 72, 8):
        y, link_size, attr, x = struct.unpack_from(">HHHH", rom, 0x00A480 + i)
        table_a480.append({"y": y, "link_size": link_size, "attr": attr, "x": x})

    return {
        "milestone": "M15.411",
        "title": "ROM Dialogue & HUD Sprite SAT Compositor and Allocation Subsystem",
        "address_range": {
            "start": "0x00A342",
            "end": "0x00A524",
            "bytes": 482,
        },
        "vram_cursor_dma": {
            "vram_dest": "0xB8E0",
            "rom_base": "0x000AAEEB",
            "entry_size": 144,
            "dma_len_words": "0x90",
        },
        "reservations": {
            "dialogue_frame_bytes": 48,
            "dialogue_frame_sprites": 6,
            "portrait_bytes": 24,
            "portrait_sprites": 3,
            "cursor_bytes": 8,
            "cursor_sprites": 1,
        },
        "tables": {
            "primary_template_a438": table_a438,
            "secondary_template_a480": table_a480,
        },
    }


def main() -> None:
    if not ROM_PATH.exists():
        raise FileNotFoundError(f"Missing ROM: {ROM_PATH}")
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
