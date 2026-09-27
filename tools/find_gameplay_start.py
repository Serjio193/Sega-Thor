from collections import Counter
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    DOMAIN_68K_RAM,
    RECORD_STRUCT,
    unpack_record,
)

corpus_dir = Path("build/m12-w6-live-discovery/natural/count-128")

# Let's inspect RAM writes in waves 1 to 10
for wave_idx in range(1, 11):
    wf = corpus_dir / f"live-discovery-wave-{wave_idx:06d}.bin"
    ram_writes = Counter()
    pcs_writing_ram = Counter()
    with wf.open("rb") as f:
        while chunk := f.read(RECORD_STRUCT.size):
            r = unpack_record(chunk)
            if r.cpu_id == CPU_68K and r.is_bus_write and r.domain == DOMAIN_68K_RAM:
                ram_writes[r.address] += 1
                pcs_writing_ram[r.pc] += 1
    top_ram = [f"0x{addr:06X}({cnt})" for addr, cnt in ram_writes.most_common(5)]
    top_pcs = [f"0x{pc:06X}({cnt})" for pc, cnt in pcs_writing_ram.most_common(5)]
    print(f"Wave {wave_idx:2d}: RAM writes={sum(ram_writes.values())}, Top RAM: {top_ram}")
    print(f"         Top PCs: {top_pcs}")
