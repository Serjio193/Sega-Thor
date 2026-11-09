#!/usr/bin/env python3
"""
Generate partitioned C++20 scheduler-safe units for M15.240 candidates:
- Candidate 1: 0x00E6BA..0x00E6EE (Slot 242 - Newton-Raphson Integer Sqrt)
- Candidate 2: 0x00F0D2..0x00F0EA (Slot 243 - 2D Euclidean Distance Vector Math)
- Candidate 3: 0x016294..0x0162CE (Slot 244 - Entity AI Patrol State Timer & Selector)
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
import sys

sys.path.append('.')
from tools.thor_recompiler.decoder import decode_at
from tools.thor_recompiler.partitioner import partition_and_generate


def decode_routine(rom: bytes, start_pc: int, end_pc: int):
    pc = start_pc
    insns = []
    while pc <= end_pc:
        insn = decode_at(rom, pc)
        if not insn:
            break
        insns.append(insn)
        pc += len(insn.raw_words) * 2
        if insn.mnemonic in ('RTS', 'RTE') and pc > end_pc:
            break
    return insns


def main():
    root = Path(__file__).resolve().parent.parent
    rom_path = root / "local-roms" / "Beyond Oasis (USA).md"
    rom = rom_path.read_bytes()
    out_dir = root / "src" / "tools" / "hybrid" / "generated"

    # Candidate 1: 0x00E6BA (Newton-Raphson Integer Sqrt)
    insns_00e6ba = decode_routine(rom, 0x00E6BA, 0x00E6EE)
    m1 = partition_and_generate("m68k_recompiled_00e6ba", 0x00E6BA, insns_00e6ba, rom, out_dir, max_insns_per_part=6)
    print(f"Candidate 1 (0x00E6BA): generated {m1['instruction_count']} insns across {m1['partition_count']} parts.")

    # Candidate 2: 0x00F0D2 (2D Euclidean Distance Vector Math)
    insns_00f0d2 = decode_routine(rom, 0x00F0D2, 0x00F0EA)
    m2 = partition_and_generate("m68k_recompiled_00f0d2", 0x00F0D2, insns_00f0d2, rom, out_dir, max_insns_per_part=5)
    print(f"Candidate 2 (0x00F0D2): generated {m2['instruction_count']} insns across {m2['partition_count']} parts.")

    # Candidate 3: 0x016294 (Entity AI Patrol State Timer & Selector)
    insns_016294 = decode_routine(rom, 0x016294, 0x0162CE)
    m3 = partition_and_generate("m68k_recompiled_016294", 0x016294, insns_016294, rom, out_dir, max_insns_per_part=7)
    print(f"Candidate 3 (0x016294): generated {m3['instruction_count']} insns across {m3['partition_count']} parts.")


if __name__ == "__main__":
    main()
