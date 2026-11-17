"""Extract 8-Direction Fixed-Point Velocity Vector & Cardinal Facing Kernel (0x00E2D4..0x00E36E).

Milestone: M15.303.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00E2D4..0x00E337: Speed (d1, d2) + dir (0..7) -> 16.16 fixed-point (vx, vy)
                        using exact 3/4 diagonal scaling ((v >> 1) + (v >> 2))
  - 0x00E2F2..0x00E301: 8-entry signed self-relative direction jump table
  - 0x00E338..0x00E36E: Velocity (vx, vy) -> cardinal facing 0..3 (N/E/S/W)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_velocity_vector_math_m15_303.json")

DIR_TABLE_ADDR = 0x00E2F2


def compute_8dir_velocity(speed_x: int, speed_y: int, direction: int) -> tuple[int, int]:
    """Compute exact ROM 0x00E2D4 16.16 fixed-point (vx, vy) for direction 0..7."""
    vx = (speed_x & 0xFFFF) << 8
    vy = (speed_y & 0xFFFF) << 8
    d = direction & 7
    if d == 0:
        return (0, -vy)
    if d == 1:
        return (vx, 0)
    if d == 2:
        return (0, vy)
    if d == 3:
        return (-vx, 0)
    if d == 4:
        vy = -vy
    elif d == 6:
        vx = -vx
    elif d == 7:
        vx = -vx
        vy = -vy
    # 0x00E322: (v >> 1) + (v >> 2) == 0.75 * v (arithmetic shift)
    vx = (vx >> 1) + (vx >> 2)
    vy = (vy >> 1) + (vy >> 2)
    return (vx, vy)


def compute_cardinal_facing(vx: int, vy: int) -> int:
    """Compute exact ROM 0x00E338 cardinal facing (0=N, 1=E, 2=S, 3=W)."""
    ax = abs(vx)
    ay = abs(vy)
    if ax <= ay:
        return 0 if vy < 0 else 2
    return 3 if vx < 0 else 1


def extract_velocity_vector_math(rom_path: Path) -> dict:
    """Extract 8-entry direction jump table from 0x00E2F2."""
    rom = rom_path.read_bytes()
    names = ["N", "E", "S", "W", "NE", "SE", "SW", "NW"]
    dirs = []
    for idx, name in enumerate(names):
        entry_addr = DIR_TABLE_ADDR + idx * 2
        off = struct.unpack_from(">h", rom, entry_addr)[0]
        dirs.append({
            "dir": idx,
            "name": name,
            "entry_addr": f"0x{entry_addr:06X}",
            "target_addr": f"0x{entry_addr + off:06X}",
        })

    return {
        "milestone": "M15.303",
        "rom_path": str(rom_path),
        "velocity_math": {
            "entry_addr": "0x00E2D4",
            "dir_table_addr": f"0x{DIR_TABLE_ADDR:06X}",
            "diagonal_scale": "(v >> 1) + (v >> 2) [0.75]",
            "facing_from_vel_addr": "0x00E338",
        },
        "directions": dirs,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_velocity_vector_math(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['directions'])} velocity vector directions to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
