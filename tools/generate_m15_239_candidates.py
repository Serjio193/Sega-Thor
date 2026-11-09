#!/usr/bin/env python3
"""
Generate M15.239 recompiled candidate clusters with automatic partitioning.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import pathlib
import sys
sys.path.append('.')

from tools.thor_recompiler.decoder import decode_at
from tools.thor_recompiler.partitioner import partition_and_generate

def decode_routine(rom: bytes, start_pc: int, max_insns: int = 50):
    pc = start_pc
    insns = []
    while len(insns) < max_insns:
        insn = decode_at(rom, pc)
        if not insn: break
        insns.append(insn)
        pc += len(insn.raw_words) * 2
        if insn.mnemonic in ('RTS', 'RTE'):
            break
    return insns

def main():
    rom_path = pathlib.Path("local-roms/Beyond Oasis (USA).md")
    rom = rom_path.read_bytes()
    out_dir = pathlib.Path("src/tools/hybrid/generated")

    # Candidate 1: 0x00E682 (Camera Viewport State/Flags Coordinator)
    # Range: 0x00E682..0x00E6A8 (12 instructions)
    insns_00e682 = []
    pc = 0x00E682
    while pc <= 0x00E6A8:
        insn = decode_at(rom, pc)
        insns_00e682.append(insn)
        pc += len(insn.raw_words) * 2
    m1 = partition_and_generate("m68k_recompiled_00e682", 0x00E682, insns_00e682, rom, out_dir, max_insns_per_part=6)
    print(f"Candidate 1 (0x00E682): generated {m1['instruction_count']} insns across {m1['partition_count']} parts.")

    # Candidate 2: 0x015AE2 (Entity Movement / AI Branch Selector)
    # Range: 0x015AE2..0x015AFC (9 instructions)
    insns_015ae2 = decode_routine(rom, 0x015AE2, max_insns=15)
    m2 = partition_and_generate("m68k_recompiled_015ae2", 0x015AE2, insns_015ae2, rom, out_dir, max_insns_per_part=5)
    print(f"Candidate 2 (0x015AE2): generated {m2['instruction_count']} insns across {m2['partition_count']} parts.")

    # Candidate 3: 0x00C9B2 (System Status Gatekeeper Routine)
    # Range: 0x00C9B2..0x00C9EA (15 instructions)
    insns_00c9b2 = []
    pc = 0x00C9B2
    while pc <= 0x00C9EA:
        insn = decode_at(rom, pc)
        insns_00c9b2.append(insn)
        pc += len(insn.raw_words) * 2
    m3 = partition_and_generate("m68k_recompiled_00c9b2", 0x00C9B2, insns_00c9b2, rom, out_dir, max_insns_per_part=8)
    print(f"Candidate 3 (0x00C9B2): generated {m3['instruction_count']} insns across {m3['partition_count']} parts.")

    print("Candidate generation complete.")

if __name__ == "__main__":
    main()
