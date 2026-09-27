"""Check exact frame boundaries and master times in wave chunks."""

from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_68K_RAM,
    RECORD_STRUCT,
    unpack_record,
)

corpus_dir = Path("build/m12-w6-live-discovery/natural/count-128")

def analyze_chunk_details(wf_path):
    first_r = None
    last_r = None
    records_count = 0
    pcs = set()
    master_times = []
    with wf_path.open("rb") as f:
        while chunk := f.read(RECORD_STRUCT.size):
            r = unpack_record(chunk)
            if first_r is None:
                first_r = r
            last_r = r
            records_count += 1
            if r.is_instruction and r.cpu_id == CPU_68K:
                pcs.add(r.pc)
    return {
        "file": wf_path.name,
        "count": records_count,
        "first_seq": first_r.stream_sequence,
        "last_seq": last_r.stream_sequence,
        "first_time": first_r.master_time,
        "last_time": last_r.master_time,
        "unique_m68k_pcs": len(pcs),
    }

w1 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000001.bin")
w2 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000002.bin")
w3 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000003.bin")
w4 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000004.bin")
w5 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000005.bin")
w106 = analyze_chunk_details(corpus_dir / "live-discovery-wave-000106.bin")

print("Wave 1:", w1)
print("Wave 2:", w2)
print("Wave 3:", w3)
print("Wave 4:", w4)
print("Wave 5:", w5)
print("Wave 106:", w106)
