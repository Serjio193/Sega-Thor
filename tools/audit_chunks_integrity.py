import hashlib
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from src.tools.thor_evidence.w3_z80_evidence import RECORD_STRUCT, unpack_record

corpus_dir = Path("build/m12-w6-live-discovery/natural/count-128")
chunks = sorted(corpus_dir.glob("live-discovery-wave-*.bin"))
print(f"Total chunk files: {len(chunks)}")

names = set(c.name for c in chunks)
assert len(names) == len(chunks) == 106, "Duplicate chunk filenames!"

results = []
total_records = 0
total_bytes = 0

for idx, c in enumerate(chunks, start=1):
    sz = c.stat().st_size
    assert sz % RECORD_STRUCT.size == 0, f"{c.name} not 48-byte aligned!"
    rec_count = sz // RECORD_STRUCT.size
    total_records += rec_count
    total_bytes += sz

    h = hashlib.sha256(c.read_bytes()).hexdigest()

    with c.open("rb") as f:
        first_raw = f.read(RECORD_STRUCT.size)
        first_rec = unpack_record(first_raw)
        f.seek(sz - RECORD_STRUCT.size)
        last_raw = f.read(RECORD_STRUCT.size)
        last_rec = unpack_record(last_raw)

    results.append({
        "idx": idx, "file": c.name, "size": sz, "records": rec_count,
        "sha256": h,
        "first_stream": first_rec.stream_sequence, "last_stream": last_rec.stream_sequence,
        "first_time": first_rec.master_time, "last_time": last_rec.master_time,
    })

print(f"Total records: {total_records}")
print(f"Total bytes: {total_bytes}")

# Check stream sequence monotonicity across chunks
for i in range(len(results) - 1):
    curr_c = results[i]
    next_c = results[i + 1]
    assert next_c["first_stream"] > curr_c["last_stream"], f"Stream sequence non-monotonic between wave {curr_c['idx']} and {next_c['idx']}: {curr_c['last_stream']} vs {next_c['first_stream']}"

print("Stream sequence monotonicity verified across all 106 chunks: STRICTLY MONOTONIC INCREASING.")

sample_indices = [1, 2, 3, 52, 53, 54, 104, 105, 106]
print("\nSample Chunks (First 3, Middle 3, Last 3):")
for r in results:
    if r["idx"] in sample_indices:
        print(f"Wave {r['idx']:3d}: {r['file']} | size={r['size']:8d} | recs={r['records']:6d} | sha256={r['sha256']} | stream={r['first_stream']:9d}..{r['last_stream']:9d} | subframe_time={r['first_time']}..{r['last_time']}")
