#!/usr/bin/env python3
"""M15.370 extractor: ROM Ali 4-dir ledge vault launch & 12-frame climb-hold gate (0x005BA2..0x005C80)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_ledge_vault_gate_m15_370.json")

LAUNCH_SPECS = [
    {
        "dir": "up",
        "entry_addr": "0x005BA2",
        "dpad_bit": 0,
        "facing": 0,
        "vx": 0x00000000,
        "vy": -0x00026666,  # 0xFFFD999A
        "vz": 0x00020000,
        "airborne_state": "0x0E",
        "anim_id": 6,
        "sfx_id": "0x00800340",
    },
    {
        "dir": "right",
        "entry_addr": "0x005BCA",
        "dpad_bit": 3,
        "facing": 1,
        "vx": 0x0002B333,
        "vy": 0x00000000,
        "vz": 0x00030000,
        "airborne_state": "0x0E",
        "anim_id": 6,
        "sfx_id": "0x00800340",
    },
    {
        "dir": "down",
        "entry_addr": "0x005BE0",
        "dpad_bit": 1,
        "facing": 2,
        "vx": 0x00000000,
        "vy": 0x00021999,
        "vz": 0x00030000,
        "airborne_state": "0x0E",
        "anim_id": 6,
        "sfx_id": "0x00800340",
    },
    {
        "dir": "left",
        "entry_addr": "0x005C1A",
        "dpad_bit": 2,
        "facing": 3,
        "vx": -0x0002B333,  # 0xFFFD4CCD
        "vy": 0x00000000,
        "vz": 0x00030000,
        "airborne_state": "0x0E",
        "anim_id": 6,
        "sfx_id": "0x00800340",
    },
]


def _i32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">i", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _i32(rom_bytes, 0x005BB0) == -0x00026666
    assert _i32(rom_bytes, 0x005BD8) == 0x0002B333
    assert _i32(rom_bytes, 0x005BEE) == 0x00021999
    assert _i32(rom_bytes, 0x005C26) == -0x0002B333
    assert _u32(rom_bytes, 0x005C70) == 0x00800340

    return {
        "milestone": "M15.370",
        "rom_range": "0x005BA2..0x005C80",
        "climb_hold_gate": {
            "helper_addr": "0x005C82",
            "frame_counter_addr": "0xFF1970",
            "required_frames": 12,
            "dpad_addr": "0xFF165E",
            "cancel_button_bit": 13,
            "saved_sp_addr": "0xFF1934",
        },
        "vault_common": {
            "target_addr": "0x005C42",
            "target_state": "0x0E",
            "target_anim": 6,
            "flags_38_or_mask": "0x0D",
            "sfx_id": "0x00800340",
            "clears_input_ring_count": "0xFF16EA",
        },
        "launch_profiles": LAUNCH_SPECS,
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
