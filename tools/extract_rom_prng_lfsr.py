"""Extract 16-Bit LFSR PRNG & Bounded Random Helpers (0x00E268..0x00E2D3).

Milestone: M15.308.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x000424 / 0x008A62: Initial PRNG seed $FF1928 = 0x7F28
  - 0x00E268..0x00E2A1: 4-bit-step 16-bit LFSR (d1 = rol16(d0, 2) ^ d0; lsr #1 -> X; addx d0, d0)
  - 0x00E2A2..0x00E2B9: Bounded u16 random ((next_rng16() * max_u16) >> 16)
  - 0x00E2BA..0x00E2D3: Bounded u8 random ((next_rng16() * (max_u8 & 0xFF)) >> 16)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_prng_lfsr_m15_308.json")


def step_lfsr16(state: int) -> int:
    """Advance Beyond Oasis 16-bit LFSR (0x00E268) by one call (4 bit shifts)."""
    d0 = state & 0xFFFF
    for _ in range(4):
        rol2 = ((d0 << 2) | (d0 >> 14)) & 0xFFFF
        d1 = rol2 ^ d0
        x_bit = d1 & 1
        d0 = ((d0 << 1) | x_bit) & 0xFFFF
    return d0


def extract_prng_lfsr(rom_path: Path) -> dict:
    """Extract PRNG seed, RAM address, and first 8 deterministic outputs."""
    rom = rom_path.read_bytes()
    init_seed = struct.unpack_from(">H", rom, 0x000426)[0]
    s = init_seed
    seq = []
    for _ in range(8):
        s = step_lfsr16(s)
        seq.append(f"0x{s:04X}")

    return {
        "milestone": "M15.308",
        "rom_path": str(rom_path),
        "prng": {
            "ram_seed_addr": "0xFF1928",
            "initial_seed": f"0x{init_seed:04X}",
            "next_rng16_addr": "0x00E268",
            "rand_u16_addr": "0x00E2A2",
            "rand_u8_addr": "0x00E2BA",
            "shifts_per_call": 4,
            "tap_rol_bits": 2,
            "first_8_outputs_from_init": seq,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_prng_lfsr(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote 16-bit LFSR PRNG spec (seed={data['prng']['initial_seed']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
