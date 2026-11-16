"""Extract Ali Player Complete 59-State Jump Table (0x00..0x74) & Death/Revive/Game-Over.

Milestone: M15.286.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0059BA..0x005A2F: Complete 59-entry signed self-relative state table (states 0x00..0x74)
  - 0x005F92 (0x5E): Death animation -> 0x0061CC auto-revive item check -> 0x6C or 0x60
  - 0x005FEA (0x60): Game Over countdown ($FF1992 = 0x28) -> fade out
  - 0x00611E (0x6C): Auto-revive HP/SP refill (+3 HP on even frames, +3 SP on odd frames)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_player_extended_states_m15_286.json")

STATE_TABLE_ADDR = 0x0059BA
TOTAL_STATE_COUNT = 59  # 0x0059BA..0x005A2F (0x00..0x74 step 2)

EXTENDED_STATE_ROLES = {
    0x28: "climb_dir_0",
    0x2A: "climb_dir_1",
    0x2C: "climb_dir_2",
    0x2E: "climb_dir_3",
    0x30: "dash_or_step",
    0x32: "push_or_pull_start",
    0x34: "push_or_pull_hold",
    0x36: "boss_or_cutscene_lock_36",
    0x38: "swim_hurt_knockback",
    0x3A: "wall_or_ledge_action",
    0x3C: "special_action_3c",
    0x3E: "pit_fall",
    0x40: "throw_or_lift_40",
    0x42: "attack_combo_42",
    0x44: "spirit_interaction_44",
    0x46: "weapon_swing_46",
    0x48: "jump_weapon_48",
    0x4A: "weapon_combo_4a",
    0x4C: "weapon_charge_4c",
    0x4E: "weapon_release_4e",
    0x50: "weapon_special_50",
    0x52: "weapon_special_52",
    0x54: "bow_or_projectile_54",
    0x56: "knockdown_56",
    0x58: "knockdown_ground_58",
    0x5A: "knockdown_rise_5a",
    0x5C: "spin_or_finisher_5c",
    0x5E: "death_and_revive_check",
    0x60: "game_over_fade",
    0x62: "item_pickup_or_chest_62",
    0x64: "item_pickup_or_chest_64",
    0x66: "item_pickup_or_chest_66",
    0x68: "item_hold_68",
    0x6A: "timed_pause_6a",
    0x6C: "auto_revive_hp_sp_refill",
    0x6E: "warp_or_door_6e",
    0x70: "game_over_alt_70",
    0x72: "boss_or_cutscene_lock_72",
    0x74: "event_wait_74",
}


def read_s16(rom: bytes, addr: int) -> int:
    """Read big-endian signed 16-bit integer from ROM."""
    return struct.unpack_from(">h", rom, addr)[0]


def extract_player_extended_states(rom_path: Path) -> dict:
    """Extract all 59 player states (0x00..0x74) and death/revive constants."""
    rom = rom_path.read_bytes()
    states = []
    for idx in range(TOTAL_STATE_COUNT):
        state_id = idx * 2
        entry_addr = STATE_TABLE_ADDR + idx * 2
        offset = read_s16(rom, entry_addr)
        target_addr = entry_addr + offset
        states.append({
            "index": idx,
            "state_id": state_id,
            "state_hex": f"0x{state_id:02X}",
            "role": EXTENDED_STATE_ROLES.get(state_id, f"base_state_0x{state_id:02X}"),
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": offset,
            "target_addr": f"0x{target_addr:06X}",
        })

    return {
        "milestone": "M15.286",
        "rom_path": str(rom_path),
        "table": {
            "addr": f"0x{STATE_TABLE_ADDR:06X}",
            "end_addr": f"0x{STATE_TABLE_ADDR + TOTAL_STATE_COUNT * 2 - 1:06X}",
            "total_states": TOTAL_STATE_COUNT,
        },
        "death_and_revive": {
            "death_state_id": "0x5E",
            "death_handler_addr": "0x005F92",
            "revive_item_check_addr": "0x0061CC",
            "revive_sfx": "0x00D00355",
            "revive_item_id": "0x2A",
            "revive_state_id": "0x6C",
            "revive_handler_addr": "0x00611E",
            "revive_step_per_frame": 3,
            "game_over_state_id": "0x60",
            "game_over_handler_addr": "0x005FEA",
            "game_over_delay_frames": 0x28,
        },
        "states": states,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_player_extended_states(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['states'])} total player states to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
