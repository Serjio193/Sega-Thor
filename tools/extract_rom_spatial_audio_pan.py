#!/usr/bin/env python3
"""Extract and verify ROM spatial 2D audio attenuation & stereo panning (0x00E78E..0x00E90C)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_spatial_audio_pan_m15_342.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00E790) == 0x0200      # Spatial threshold (id >= 0x200)
    assert u16(rom, 0x00E83C) == 0x0300      # Hi-res volume vs stepped FM/PSG threshold
    assert u16(rom, 0x00E844) == 0x0058      # Hi-res attenuation start dist (88px)
    assert u16(rom, 0x00E86E) == 0x0040      # Hi-res stereo pan threshold (64px)
    assert u16(rom, 0x00E8AA) == 0x00B0      # FM/PSG step 1 dist (176px)
    assert u16(rom, 0x00E8B4) == 0x00E0      # FM/PSG step 2 dist (224px)
    assert u16(rom, 0x00E8BE) == 0x0110      # FM/PSG cull dist (272px)
    assert u16(rom, 0x00E8C6) == 0x0060      # FM/PSG stereo pan threshold (96px)

    return {
        "milestone": "M15.342",
        "title": "ROM Spatial 2D Audio Attenuation & Stereo Panning",
        "routines": {
            "play_spatial_at_xy": {"start": "0x00E78E", "end": "0x00E7E6"},
            "play_spatial_at_entity": {"start": "0x00E7E8", "end": "0x00E90C"},
        },
        "constants": {
            "spatial_min_sound_id": "0x0200",
            "hires_min_sound_id": "0x0300",
            "hires_attenuate_start_px": 0x58,
            "hires_max_drop_steps": 7,
            "hires_pan_dx_px": 0x40,
            "fm_step1_dist_px": 0xB0,
            "fm_step2_dist_px": 0xE0,
            "fm_cull_dist_px": 0x110,
            "fm_pan_dx_px": 0x60,
            "pan_left_flag": "0x40",
            "pan_right_flag": "0x80",
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
