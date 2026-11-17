#!/usr/bin/env python3
"""Extract and verify ROM demo mode overlay SAT compositor & camera particle spark generator (0x00A7E6..0x00A9A0)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_demo_hud_spark_generator_m15_414.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00A7E6: Demo overlay entry
    assert u16(rom, 0x00A7E6) == 0x3039          # move.w $ff1656.l, d0
    assert u32(rom, 0x00A7E6 + 2) == 0x00FF1656
    assert u16(rom, 0x00A7F4) == 0x4BF9          # lea.l $ff13cc.l, a5
    assert u32(rom, 0x00A7F4 + 2) == 0x00FF13CC

    # 0x00A864: 3-sprite mode DMA transfer
    assert u16(rom, 0x00A864) == 0x2AFC          # move.l #$aa1fd, (a5)+
    assert u32(rom, 0x00A864 + 2) == 0x000AA1FD

    # 0x00A8C4: 2-sprite mode DMA transfer
    assert u16(rom, 0x00A8C4) == 0x2AFC          # move.l #$aa1ad, (a5)+
    assert u32(rom, 0x00A8C4 + 2) == 0x000AA1AD

    # 0x00A8DA: SAT entry emit helper
    assert u16(rom, 0x00A8DA) == 0x0C45          # cmpi.w #$50, d5
    assert u16(rom, 0x00A8DA + 2) == 0x0050

    # 0x00A8F0: Particle spark sprite generator
    assert u16(rom, 0x00A8F0) == 0x302E          # move.w $6(a6), d0
    assert u16(rom, 0x00A8F0 + 2) == 0x0006
    assert u16(rom, 0x00A8F4) == 0x5340          # subq.w #1, d0
    assert u16(rom, 0x00A938) == 0x43F9          # lea.l $ff1716.l, a1
    assert u32(rom, 0x00A938 + 2) == 0x00FF1716
    assert u16(rom, 0x00A94A) == 0x43F9          # lea.l $ff173e.l, a1
    assert u32(rom, 0x00A94A + 2) == 0x00FF173E
    assert u16(rom, 0x00A99A) == 0x4E75          # rts
    assert u16(rom, 0x00A99E) == 0x4E75          # rts

    return {
        "milestone": "M15.414",
        "title": "ROM Demo Mode Overlay SAT Compositor & Camera Particle Spark Generator",
        "address_range": {
            "start": "0x00A7E6",
            "end": "0x00A9A0",
            "bytes": 442,
        },
        "demo_overlay": {
            "status_reg": "0x00FF1656",
            "mode_3_sprite": {
                "dma_src": "0x000AA1FD",
                "dma_vram": "0xF360",
                "dma_words": "0xC0",
                "sprites_x": [240, 272, 304],
                "y": 228,
            },
            "mode_2_sprite": {
                "dma_src": "0x000AA1AD",
                "dma_vram": "0xF360",
                "dma_words": "0x50",
                "sprites_x": [268, 292],
                "y": 228,
            },
        },
        "spark_particle": {
            "lifetime_offset": "0x06",
            "x_offset": "0x04",
            "y_offset": "0x02",
            "camera_plane_1": "0x00FF1716",
            "camera_plane_2": "0x00FF173E",
            "screen_offset": 120,
            "bounds": {"max_y": 240, "max_x": 336},
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
