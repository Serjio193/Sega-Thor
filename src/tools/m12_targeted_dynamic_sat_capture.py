"""Fail-closed analyzer for the targeted dynamic SAT capture."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

SOURCE_OWNED = 1_487_672
SHADOW_BASE = 0xFF13CC
ENTRY_BYTES = 8


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def changed_entries(snapshots: list[dict[str, Any]]) -> list[dict[str, Any]]:
    changes = []
    for previous, current in zip(snapshots, snapshots[1:]):
        before, after = previous["sat"], current["sat"]
        for entry in range(min(len(before), len(after)) // ENTRY_BYTES):
            start = entry * ENTRY_BYTES
            changed = [offset for offset in range(ENTRY_BYTES) if before[start + offset] != after[start + offset]]
            if changed:
                changes.append({
                    "from_frame": previous["frame"], "to_frame": current["frame"],
                    "entry": entry, "changed_offsets": changed,
                    "before": [before[start + offset] for offset in changed],
                    "after": [after[start + offset] for offset in changed],
                })
    return changes


def correlate(change: dict[str, Any], snapshots: list[dict[str, Any]], writes: list[dict[str, Any]]) -> dict[str, Any]:
    frame = change["to_frame"]
    source_frame = change["from_frame"]
    current = next(item for item in snapshots if item["frame"] == frame)
    source = next(item for item in snapshots if item["frame"] == source_frame)
    previous = next((item for item in snapshots if item["frame"] == source_frame - 1), None)
    start = change["entry"] * ENTRY_BYTES
    shadow_offsets = [] if previous is None else [
        offset for offset in range(start, start + ENTRY_BYTES)
        if previous["shadow"][offset] != source["shadow"][offset]
    ]
    shadow_entry = source["shadow"][start:start + ENTRY_BYTES]
    sat_entry = current["sat"][start:start + ENTRY_BYTES]
    writer_window = [item for item in writes if source_frame - 1 <= item["frame"] <= source_frame
                     and int(item["address"], 16) < SHADOW_BASE + start + ENTRY_BYTES
                     and int(item["address"], 16) + 4 > SHADOW_BASE + start]
    frame_writes = [item for item in writer_window if item["frame"] == source_frame]
    writer_pcs = sorted({item["pc"] for item in writer_window})
    return {**change, "shadow_offsets": shadow_offsets,
            "shadow_addresses": [f"0x{SHADOW_BASE + offset:06X}" for offset in shadow_offsets],
            "shadow_entry_equals_sat_entry": shadow_entry == sat_entry,
            "shadow_entry": shadow_entry, "sat_entry": sat_entry,
            "same_frame_shadow_writes": frame_writes, "writer_window": writer_window,
            "source_shadow_frame": source_frame,
            "writer_window_frames": [source_frame - 1, source_frame],
            "writer_window_pcs": writer_pcs,
            "correlation_status": "SAT_ENTRY_SHADOW_WRITER_CORRELATED" if shadow_entry == sat_entry and writer_pcs
            else "SAT_CHANGED_BUT_SHADOW_CORRELATION_INCOMPLETE"}


def analyze(document: dict[str, Any]) -> dict[str, Any]:
    snapshots = document.get("snapshots", [])
    writes = document.get("shadow_writes", [])
    changes = changed_entries(snapshots)
    correlated = [correlate(item, snapshots, writes) for item in changes]
    grouped: dict[int, int] = defaultdict(int)
    for item in correlated:
        grouped[item["entry"]] += 1
    candidates = sorted(correlated, key=lambda item: (not item["shadow_offsets"], item["to_frame"], item["entry"]))
    selected = next((item for item in candidates
                     if item["shadow_offsets"] and item["shadow_entry_equals_sat_entry"]
                     and item["writer_window_pcs"]), None)
    return {
        "schema": "oasis.m12.targeted-dynamic-sat-analysis.v1",
        "status": "PASS" if selected else "STOP",
        "stop_reason": None if selected else "STOP_NO_DYNAMIC_SAT_ENTRY_IDENTIFIED",
        "capture_run": {"canonical_rom_sha256": document.get("canonical_rom_sha256"),
                         "scenario_path": document.get("scenario_path"),
                         "frames_executed": document.get("frames_executed"),
                         "snapshot_count": len(snapshots), "shadow_write_count": len(writes)},
        "sat": {"base": document.get("sat_base"), "entry_bytes": ENTRY_BYTES,
                "changed_transition_count": len(changes),
                "changed_entry_counts": {f"{entry}": count for entry, count in sorted(grouped.items())}},
        "selected_candidate": selected,
        "candidates": candidates,
        "source_owned_contract": {"before": SOURCE_OWNED, "after": SOURCE_OWNED, "delta": 0},
        "semantic_boundary": "dynamic SAT entry and writer correlation only; no player/entity label",
        "commit_created": "NO", "push_performed": "NO",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    document = json.loads(args.capture.read_text(encoding="utf-8"))
    report = analyze(document)
    report["capture_artifact_sha256"] = sha256(args.capture)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["status"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
