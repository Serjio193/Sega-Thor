"""Extract Ali Player 20-State Action & Combat Machine (0x005580..0x0059E2).

Milestone: M15.285.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x005580: Player tick entry (a6 = $FF19E8)
  - 0x0055B0: Low HP heartbeat warning (HP <= 0x40, period (HP>>1)+10, SFX 0xA0038C)
  - 0x0055EE: Low SP warning (SP <= 0x18, period SP+12, SFX 0xA004DD)
  - 0x005638: Safe respawn pos tracking ($FF1972 when state in {0,2,4} & !airborne)
  - 0x005718: Hazard/lava tile periodic damage ($FF0DBA bit 0 + $FF16F0 bit 5, every 16 or 8 frames)
  - 0x005762: Passive SP regen ($FF1AA4==0 & (water or ring), +1 every 8 or 4 frames)
  - 0x0059AE..0x0059E1: 20-entry signed self-relative state jump table (states 0x00..0x26)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_player_state_machine_m15_285.json")

STATE_TABLE_ADDR = 0x0059BA
STATE_COUNT = 20

STATE_NAMES = [
    (0x00, "idle_standing"),
    (0x02, "walking"),
    (0x04, "running_or_dash"),
    (0x06, "jumping_airborne"),
    (0x08, "knockback_hurt"),
    (0x0A, "climbing"),
    (0x0C, "swimming"),
    (0x0E, "falling_landing"),
    (0x10, "crouching"),
    (0x12, "crouch_recovery"),
    (0x14, "spirit_summon_cast"),
    (0x16, "jump_attack"),
    (0x18, "stun_recovery"),
    (0x1A, "primary_attack_start"),
    (0x1C, "primary_attack_hold"),
    (0x1E, "attack_follow_through"),
    (0x20, "secondary_attack_start"),
    (0x22, "secondary_attack_hold"),
    (0x24, "attack_recovery"),
    (0x26, "aerial_special"),
]


def read_s16(rom: bytes, addr: int) -> int:
    """Read big-endian signed 16-bit integer from ROM."""
    return struct.unpack_from(">h", rom, addr)[0]


def extract_player_state_machine(rom_path: Path) -> dict:
    """Extract Ali's 20-state jump table and tick thresholds from ROM."""
    rom = rom_path.read_bytes()
    states = []
    for idx, (state_val, name) in enumerate(STATE_NAMES):
        entry_addr = STATE_TABLE_ADDR + idx * 2
        offset = read_s16(rom, entry_addr)
        target_addr = entry_addr + offset
        states.append({
            "index": idx,
            "state_id": state_val,
            "state_hex": f"0x{state_val:02X}",
            "name": name,
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": offset,
            "target_addr": f"0x{target_addr:06X}",
        })

    return {
        "milestone": "M15.285",
        "rom_path": str(rom_path),
        "player_tick": {
            "entry_addr": "0x005580",
            "dispatcher_addr": "0x005670",
            "state_table_addr": f"0x{STATE_TABLE_ADDR:06X}",
            "player_struct_addr": "0xFF19E8",
            "safe_respawn_addr": "0xFF1972",
            "low_hp_threshold": 0x40,
            "low_hp_sfx": "0xA0038C",
            "low_sp_threshold": 0x18,
            "low_sp_sfx": "0xA004DD",
        },
        "states": states,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_player_state_machine(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['states'])} player states to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
