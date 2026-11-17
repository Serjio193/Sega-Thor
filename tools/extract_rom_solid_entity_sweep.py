"""Extract 3D AABB Solid Entity Sweep & Push-Contact Kernel (0x009DD6..0x009EB8).

Milestone: M15.305.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x009DD6..0x009EB8: Swept +X 3D AABB collision across 21 entity slots ($FF19E8, stride 0xBC)
                        using (x +/- $42, y + floor_z +/- $44, z + floor_z .. +$4A)
  - 0x009E7E..0x009EA8: Sets $40 bit 7, $68 = blocker ptr, $6C = 3f push-contact timer;
                        nudges blocker +1px ($72(a5) += 1) if clearance == 0 and pushable
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_solid_entity_sweep_m15_305.json")


def sweep_solid_x_pos(
    x: int,
    y: int,
    z: int,
    floor_z: int,
    half_w: int,
    half_h: int,
    height_z: int,
    dx: int,
    blockers: list[dict],
) -> dict:
    """Apply exact ROM 0x009DD6..0x009EB8 positive-X 3D AABB sweep against solid entities."""
    d1 = x + half_w
    y_eff = y + floor_z
    y_min = y_eff - half_h
    y_max = y_eff + half_h
    z_min = z + floor_z
    z_max = z_min + height_z
    best_dx = dx
    hit_idx = -1

    for idx, b in enumerate(blockers):
        if not b.get("solid", True):
            continue
        if b["x"] <= x:
            continue
        gap = b["x"] - b["half_w"] - d1
        if gap < 0:
            gap = 0
        if gap >= best_dx:
            continue
        by_eff = b["y"] + b["floor_z"]
        if by_eff + b["half_h"] <= y_min or by_eff - b["half_h"] >= y_max:
            continue
        bz_min = b["z"] + b["floor_z"]
        if bz_min + b["height_z"] <= z_min or bz_min >= z_max:
            continue
        best_dx = gap
        hit_idx = idx

    return {
        "clamped_dx": best_dx,
        "blocker_index": hit_idx,
        "push_timer": 3 if hit_idx >= 0 else 0,
        "nudge_blocker": (hit_idx >= 0 and best_dx == 0),
    }


def extract_solid_entity_sweep(rom_path: Path) -> dict:
    """Extract 3D AABB solid entity sweep constants from ROM."""
    rom = rom_path.read_bytes()
    return {
        "milestone": "M15.305",
        "rom_path": str(rom_path),
        "solid_sweep": {
            "entry_x_pos_addr": "0x009DD6",
            "entity_table_base": "0xFF19E8",
            "slot_count": rom[0x009E05] + 1,
            "slot_stride_bytes": 0xBC,
            "push_contact_timer_frames": rom[0x009E8B],
            "collision_flag_bit": 7,
            "skip_trait_bit_3a": 1,
            "skip_render_bit_38": 2,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_solid_entity_sweep(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote 3D AABB solid sweep spec ({data['solid_sweep']['slot_count']} slots) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
