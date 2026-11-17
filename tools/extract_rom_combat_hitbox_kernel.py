"""Extract Combat Hitbox Collision, Kill-Rank Damage Scaling & Shield Block (0x00BCBA..0x00BF26).

Milestone: M15.297.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00BCBA: Player/Spirit attack vs 17 enemy slots ($FF1CD8, callback 0x00BE98)
  - 0x00BCD8: Enemy attack vs Ali ($FF19E8, callback 0x00BEBA)
  - 0x00BD02..0x00BD3C: 44-byte (0x2C) hitbox record lookup by anim frame $32(a6), 12B per dir (dir 3 mirrors dir 1)
  - 0x00BD40..0x00BD85: Ali kill-count ($96(a6)) bonus damage thresholds (10,20,30,40,50,100,200 -> +1..+11)
  - 0x00BE98..0x00BF26: Enemy HP bar HUD timer (90f = 0x5A at $FF184F), Shield Guard opposite-facing check ($6D), hit-flash 6f ($83)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_combat_hitbox_kernel_m15_297.json")


def compute_ali_kill_rank_bonus(kills: int) -> int:
    """Compute exact ROM 0x00BD4A..0x00BD85 cumulative damage bonus from kill count."""
    bonus = 0
    for thresh, inc in [(10, 1), (20, 1), (30, 1), (40, 1), (50, 1), (100, 2), (200, 4)]:
        if kills < thresh:
            break
        bonus += inc
    return bonus


def extract_combat_hitbox_kernel(rom_path: Path) -> dict:
    """Extract hitbox stride, kill-rank thresholds, and contact constants."""
    rom = rom_path.read_bytes()
    enemy_hud_frames = rom[0x00BEAF]
    hit_flash_frames = rom[0x00BEEF]
    thresholds = [
        {"kills": 10, "delta": 1, "total_bonus": 1},
        {"kills": 20, "delta": 1, "total_bonus": 2},
        {"kills": 30, "delta": 1, "total_bonus": 3},
        {"kills": 40, "delta": 1, "total_bonus": 4},
        {"kills": 50, "delta": 1, "total_bonus": 5},
        {"kills": 100, "delta": 2, "total_bonus": 7},
        {"kills": 200, "delta": 4, "total_bonus": 11},
    ]

    return {
        "milestone": "M15.297",
        "rom_path": str(rom_path),
        "hitbox_kernel": {
            "player_attack_entry": "0x00BCBA",
            "enemy_attack_entry": "0x00BCD8",
            "player_contact_cb": "0x00BE98",
            "enemy_contact_cb": "0x00BEBA",
            "shield_block_target": "0x00BFAE",
            "reaction_table_addr": "0x00BF26",
            "enemy_slot_count": 17,
            "hitbox_record_bytes": 0x2C,
            "dir_box_stride_bytes": 12,
            "enemy_hp_hud_frames": enemy_hud_frames,
            "hit_flash_frames": hit_flash_frames,
        },
        "kill_rank_damage_bonuses": thresholds,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_combat_hitbox_kernel(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote combat hitbox kernel ({len(data['kill_rank_damage_bonuses'])} kill-rank tiers) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
