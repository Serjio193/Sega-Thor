"""Extract Ali Command-Input Special Moves (0x0086B4 360 Spin & 0x008778 F-B-F Lunge).

Milestone: M15.299.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0086B4..0x008767: 360-degree Spin Slash detector (scans up to 30 frames in $FF1668,
                        4 cardinal directions in CW +2 or CCW -2 order mod 8, clears $FF16EA)
  - 0x008768..0x008777: 16-byte D-pad nibble -> 8-way direction LUT (0xFF = invalid)
  - 0x008778..0x0087F3: Forward-Back-Forward Lunge detector (per-facing 8-byte seq/mask,
                        aborts on perpendicular input d2/d3, clears $FF16EA)
  - 0x0087F4..0x008813: 4x8-byte F-B-F sequence & mask table for facings 0..3
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_special_move_inputs_m15_299.json")


def extract_special_move_inputs(rom_path: Path) -> dict:
    """Extract 360-spin LUT and 4-facing F-B-F lunge sequence table."""
    rom = rom_path.read_bytes()
    dpad_lut = list(rom[0x008768:0x008778])

    fbf_table = []
    for facing in range(4):
        base = 0x0087F4 + facing * 8
        chunk = list(rom[base:base + 8])
        fbf_table.append({
            "facing": facing,
            "sequence": chunk[:4],
            "masks": chunk[4:],
        })

    return {
        "milestone": "M15.299",
        "rom_path": str(rom_path),
        "spin_360": {
            "detector_addr": "0x0086B4",
            "dpad_lut_addr": "0x008768",
            "max_history_frames": 30,
            "cardinal_steps_required": 4,
            "dpad_lut_16": dpad_lut,
        },
        "fbf_lunge": {
            "detector_addr": "0x008778",
            "table_addr": "0x0087F4",
            "perpendicular_cancel_vertical": [4, 8],
            "perpendicular_cancel_horizontal": [1, 2],
            "facings": fbf_table,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_special_move_inputs(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote 360-spin LUT and {len(data['fbf_lunge']['facings'])} F-B-F lunge sequences to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
