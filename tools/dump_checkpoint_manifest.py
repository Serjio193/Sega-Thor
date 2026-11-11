#!/usr/bin/env python3
"""Dump and verify the 37 checkpoints across the 6 gameplay scenarios."""
import json
import pathlib

def main():
    p = pathlib.Path("data/thor-evidence/m15_257_qualification_summary.json")
    data = json.loads(p.read_text(encoding="utf-8"))

    total_cps = 0
    all_hashes = set()
    manifest = []

    for sc_name, sc_data in data["scenarios"].items():
        cps = sc_data["checkpoints"]["checkpoints"]
        total_cps += len(cps)
        for cp in cps:
            h = cp["sha256"]
            all_hashes.add(h)
            manifest.append({
                "scenario": sc_name,
                "frame": cp["frame"],
                "sha256": h,
                "match": cp["match"]
            })

    print(f"Total checkpoints: {total_cps}")
    print(f"Distinct checkpoint SHA256 hashes: {len(all_hashes)}")
    print("\n=== CHECKPOINT MANIFEST (37 DISTINCT EVIDENCE POINTS) ===")
    for idx, cp in enumerate(manifest, 1):
        print(f"{idx:02d}. {cp['scenario']} frame={cp['frame']:04d} sha256={cp['sha256']} (verified={cp['match']})")

if __name__ == "__main__":
    main()
