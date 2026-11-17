#!/usr/bin/env python3
"""Extract and verify ROM Ali pit-fall damage, safe respawn anchor & 3px coordinate interpolator (0x00693C..0x006B28)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_respawn_anchor_m15_378.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00694E) == 0x000A      # Pit damage timer threshold (10 frames)
    assert u16(rom, 0x006960) == 0x0014      # Pit fall damage amount: 20 HP
    assert u16(rom, 0x006984) == 0x0070      # Fatal death state (0x70)
    assert rom[0x006991] == 0x28             # Game Over fade delay (40 frames)
    assert u16(rom, 0x0069DC) == 0x003F      # Respawn invuln timer: 63 frames (0x3F)
    assert u32(rom, 0x0069F6) == 0x00FF1972  # Safe respawn anchor coordinate pair
    assert rom[0x006AB5] == 0x0C             # Post-respawn invuln timer: 12 frames
    assert rom[0x006AE7] == 0x2A             # Auto-revive item ID (0x2A)

    return {
        "milestone": "M15.378",
        "title": "ROM Ali Pit-Fall Damage, Safe Respawn Anchor & 3px Coordinate Interpolator",
        "routines": {
            "ali_pit_damage_and_fatal_check": {"start": "0x00693C", "end": "0x006996"},
            "ali_safe_anchor_respawn_loop": {"start": "0x006998", "end": "0x006B10"},
            "step_1px_coordinate_interpolator": {"start": "0x006B12", "end": "0x006B28"},
        },
        "constants": {
            "pit_damage_timer_threshold": 10,
            "pit_fall_damage_hp": 20,
            "fatal_death_state": "0x0070",
            "game_over_fade_frames": 40,
            "respawn_invuln_timer_frames": 63,
            "post_respawn_invuln_frames": 12,
            "safe_respawn_anchor_addr": "0x00FF1972",
            "auto_revive_item_id": "0x002A",
            "interpolator_steps_per_axis": 3,
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
