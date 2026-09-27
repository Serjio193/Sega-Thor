import json
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_68K_RAM,
    DOMAIN_BANKED_ROM,
    DOMAIN_PSG,
    DOMAIN_ROM,
    DOMAIN_VDP,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    DOMAIN_Z80_WINDOW,
    RECORD_STRUCT,
    iter_records,
)

corpus_dir = Path("build/m12-w6-live-discovery/natural/count-128")
wave_files = sorted(corpus_dir.glob("live-discovery-wave-*.bin"))

print(f"Scanning {len(wave_files)} waves for first-seen and frequency...")

# We want to track first seen wave and frequency for candidate novel findings
# Load findings from DISCOVERY_FINDINGS_TABLE.md or novelty files
novelty_m68k = json.load(open("build/m12-w6-live-discovery/m68k_execution_novelty.json"))
novelty_z80 = json.load(open("build/m12-w6-live-discovery/z80_execution_novelty.json"))

top_m68k_pcs = [int(x, 16) for x in novelty_m68k["new_m68k_pcs"][:10]]
top_z80_pcs = [int(x, 16) for x in novelty_z80["new_z80_pcs"][:10]]

first_seen_m68k = {}
first_seen_z80 = {}
freq_m68k = {pc: 0 for pc in top_m68k_pcs}
freq_z80 = {pc: 0 for pc in top_z80_pcs}

# Track banked rom ranges
# 0x082190..0x08219B, 0x084C41..0x084C4D, 0x08807F..0x08808C
bank_ranges = [(0x082190, 0x08219B), (0x084C41, 0x084C4D), (0x08807F, 0x08808C)]
first_seen_bank = {}
freq_bank = {r: 0 for r in bank_ranges}

# Track handoffs: 0x0004, 0x0017, 0x0018
handoff_addrs = [0x0004, 0x0003, 0x0017, 0x0018]
first_seen_handoff = {}
freq_handoff = {a: 0 for a in handoff_addrs}

# Track audio hits in Resource 1 (0x0BD540..0x0BF768)
first_seen_res1 = None
freq_res1 = 0

for w_idx, wf in enumerate(wave_files, start=1):
    for r in iter_records(wf):
        if r.cpu_id == CPU_68K:
            if r.is_instruction and r.pc in freq_m68k:
                freq_m68k[r.pc] += 1
                if r.pc not in first_seen_m68k:
                    first_seen_m68k[r.pc] = w_idx
            if r.domain == DOMAIN_Z80_WINDOW:
                addr = r.address & 0x1FFF
                if addr in freq_handoff:
                    freq_handoff[addr] += 1
                    if addr not in first_seen_handoff:
                        first_seen_handoff[addr] = w_idx
        elif r.cpu_id == CPU_Z80:
            if r.is_instruction and r.pc in freq_z80:
                freq_z80[r.pc] += 1
                if r.pc not in first_seen_z80:
                    first_seen_z80[r.pc] = w_idx
            if r.is_bus_read and r.domain == DOMAIN_BANKED_ROM:
                phys = r.auxiliary if r.auxiliary != 0 else r.address
                if 0x0BD540 <= phys < 0x0BF768:
                    freq_res1 += 1
                    if first_seen_res1 is None:
                        first_seen_res1 = w_idx
                for br in bank_ranges:
                    if br[0] <= phys <= br[1]:
                        freq_bank[br] += 1
                        if br not in first_seen_bank:
                            first_seen_bank[br] = w_idx

print("M68K PCs:")
for pc in top_m68k_pcs:
    print(f"  0x{pc:06X}: first_seen=Wave {first_seen_m68k.get(pc)}, freq={freq_m68k[pc]}")

print("Z80 PCs:")
for pc in top_z80_pcs:
    print(f"  0x{pc:04X}: first_seen=Wave {first_seen_z80.get(pc)}, freq={freq_z80[pc]}")

print("Banked ROM Ranges:")
for br in bank_ranges:
    print(f"  0x{br[0]:06X}..0x{br[1]:06X}: first_seen=Wave {first_seen_bank.get(br)}, freq={freq_bank[br]}")

print("Handoff Shared Memory:")
for ha in handoff_addrs:
    print(f"  0x{ha:04X}: first_seen=Wave {first_seen_handoff.get(ha)}, freq={freq_handoff[ha]}")

print(f"Resource 1 Hits: first_seen=Wave {first_seen_res1}, freq={freq_res1}")
