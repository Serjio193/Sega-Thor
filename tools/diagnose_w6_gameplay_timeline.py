"""Diagnostic script to inspect the timeline of waves in M12 W6 corpus."""

from collections import Counter
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_68K_RAM,
    DOMAIN_ROM,
    DOMAIN_VDP,
    FLAG_INSTRUCTION,
    RECORD_STRUCT,
    unpack_record,
)

corpus_dir = Path("build/m12-w6-live-discovery/natural/count-128")
wave_files = sorted(corpus_dir.glob("live-discovery-wave-*.bin"))

print(f"Total wave files: {len(wave_files)}")

# Let's inspect each wave's master_time range, PC distribution, and notable RAM accesses
for idx, wf in enumerate(wave_files, start=1):
    m68k_pcs = Counter()
    z80_pcs = Counter()
    ram_writes = Counter()
    first_time = None
    last_time = None
    record_count = 0
    frame_boundaries = 0

    with wf.open("rb") as f:
        while chunk := f.read(RECORD_STRUCT.size):
            r = unpack_record(chunk)
            record_count += 1
            if first_time is None:
                first_time = r.master_time
            last_time = r.master_time

            if r.is_frame_boundary:
                frame_boundaries += 1

            if r.cpu_id == CPU_68K:
                if r.is_instruction:
                    m68k_pcs[r.pc] += 1
                if r.is_bus_write and r.domain == DOMAIN_68K_RAM:
                    ram_writes[r.address] += 1
            elif r.cpu_id == CPU_Z80 and r.is_instruction:
                z80_pcs[r.pc] += 1

    top_m68k = [f"0x{pc:06X}({cnt})" for pc, cnt in m68k_pcs.most_common(3)]
    top_z80 = [f"0x{pc:04X}({cnt})" for pc, cnt in z80_pcs.most_common(2)]
    print(f"Wave {idx:3d} ({wf.name}): recs={record_count:6d}, time={first_time}..{last_time}, frames={frame_boundaries}, top_68k={top_m68k}, top_z80={top_z80}")
    if idx >= 15 and idx % 10 != 0 and idx < 100:
        continue
