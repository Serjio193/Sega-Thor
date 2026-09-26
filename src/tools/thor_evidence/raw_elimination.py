"""Fail-closed raw-evidence inventory and deletion-seal evaluator.

This module never deletes evidence. An audit classifies files; a seal is
accepted only when all accounting, artifact identity, version, and replay gates
are independently supplied and match their retained artifacts.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
from typing import Any


SCHEMA = "oasis.raw-elimination.v1"
RAW_NAMES = {
    "flow-v1-records.bin": ("FLOW_V1", "LIVE_FORWARD_WORKER"),
    "live-forward-wave-records-pass1.bin": ("FLOW_V1", "LIVE_FORWARD_WORKER"),
    "live-forward-wave-records-pass2.bin": ("FLOW_V1", "LIVE_FORWARD_WORKER"),
}
RAW_SUFFIXES = {".raw", ".trace", ".capsule"}
CAPSULE_RE = re.compile(r"^capsule-\d{2}-[A-Za-z0-9_-]+\.bin$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
CHUNK_BYTES = 1024 * 1024


def canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=True).encode("utf-8")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(CHUNK_BYTES), b""):
            digest.update(chunk)
    return digest.hexdigest()


def event_accounting(counts: dict[str, int]) -> dict[str, int]:
    required = ("input_events", "accepted_events", "unresolved_events",
                "rejected_events", "duplicate_events")
    values = {key: counts.get(key) for key in required}
    if any(type(value) is not int or value < 0 for value in values.values()):
        raise ValueError("STOP_EVENT_ACCOUNTING_INVALID")
    accounted = sum(values[key] for key in required[1:])
    return {**values, "accounted_events": accounted,
            "unaccounted_events": values["input_events"] - accounted}


def _artifact_matches(artifact: Any, root: Path) -> bool:
    if not isinstance(artifact, dict):
        return False
    relative = artifact.get("path")
    expected = artifact.get("sha256")
    if not isinstance(relative, str) or not isinstance(expected, str) or \
            not SHA_RE.fullmatch(expected):
        return False
    path = (root / relative).resolve()
    try:
        path.relative_to(root.resolve())
        return path.is_file() and file_sha256(path) == expected
    except (OSError, ValueError):
        return False


def evaluate_seal(receipt: dict[str, Any], receipt_root: Path,
                  current_versions: dict[str, str] | None = None) -> dict[str, Any]:
    """Evaluate a proposed seal; missing/unknown gates always mean KEEP."""
    reasons: list[str] = []
    if receipt.get("schema") != SCHEMA:
        reasons.append("UNSUPPORTED_RECEIPT_SCHEMA")
    raw = receipt.get("raw_file")
    if not isinstance(raw, dict) or not isinstance(raw.get("path"), str) or \
            not isinstance(raw.get("sha256"), str) or not SHA_RE.fullmatch(raw.get("sha256", "")):
        reasons.append("RAW_IDENTITY_MISSING")
    else:
        raw_path = (receipt_root / raw["path"]).resolve()
        try:
            raw_path.relative_to(receipt_root.resolve())
            if not raw_path.is_file() or raw_path.stat().st_size != raw.get("bytes") or \
                    file_sha256(raw_path) != raw["sha256"]:
                reasons.append("RAW_IDENTITY_MISMATCH")
        except (OSError, ValueError):
            reasons.append("RAW_IDENTITY_MISMATCH")

    counts = receipt.get("event_counts")
    try:
        accounting = event_accounting(counts if isinstance(counts, dict) else {})
        if accounting["input_events"] == 0:
            reasons.append("EVENT_SET_EMPTY")
        if accounting["unaccounted_events"] != 0:
            reasons.append("UNACCOUNTED_EVENTS")
    except ValueError:
        accounting = {"input_events": 0, "accounted_events": 0,
                      "unaccounted_events": -1}
        reasons.append("EVENT_ACCOUNTING_INVALID")

    versions = receipt.get("versions")
    required_versions = ("raw_format", "extractor", "normalizer", "validator",
                         "session_generation", "canonical_generation")
    if not isinstance(versions, dict) or any(
            not isinstance(versions.get(key), dict) or
            not isinstance(versions[key].get("version"), str) or
            not versions[key]["version"] or
            not isinstance(versions[key].get("sha256"), str) or
            not SHA_RE.fullmatch(versions[key].get("sha256", ""))
            for key in required_versions):
        reasons.append("VERSION_INVENTORY_INCOMPLETE")
    elif current_versions is None or any(
            current_versions.get(key) != versions[key]["sha256"]
            for key in required_versions):
        reasons.append("CURRENT_VERSION_IDENTITY_UNAVAILABLE_OR_STALE")
    if receipt.get("all_fields_known_to_format_accounted") is not True:
        reasons.append("FORMAT_FIELD_INVENTORY_INCOMPLETE")
    if receipt.get("open_investigations") != 0:
        reasons.append("OPEN_INVESTIGATIONS")

    artifacts = receipt.get("retained_artifacts")
    if not isinstance(artifacts, dict) or not artifacts:
        reasons.append("RETAINED_ARTIFACTS_MISSING")
        artifacts = {}
    normalized_present = "normalized_semantic" in artifacts and \
        _artifact_matches(artifacts.get("normalized_semantic"), receipt_root)
    session_present = "session_store" in artifacts and \
        _artifact_matches(artifacts.get("session_store"), receipt_root)
    if not normalized_present:
        reasons.append("LOSSLESS_NORMALIZED_ARTIFACT_MISSING_OR_MISMATCH")
    for name, artifact in sorted(artifacts.items()):
        if name not in {"normalized_semantic", "session_store"} and \
                not _artifact_matches(artifact, receipt_root):
            reasons.append(f"RETAINED_ARTIFACT_MISMATCH:{name}")

    replay = receipt.get("replay")
    replay_pass = isinstance(replay, dict) and all(replay.get(key) is True for key in
            ("without_raw", "graph_hash_match", "path_hash_match",
             "occurrence_hash_match", "unresolved_identity_match"))
    if not replay_pass:
        reasons.append("REPLAY_WITHOUT_RAW_NOT_PROVEN")
    if receipt.get("unique_branch_witnesses_complete") is not True:
        reasons.append("UNIQUE_BRANCH_WITNESSES_INCOMPLETE")
    if receipt.get("capture_gaps_preserved") is not True:
        reasons.append("CAPTURE_GAP_LINEAGE_UNPROVEN")
    session_replay = receipt.get("session_replay")
    session_replay_pass = isinstance(session_replay, dict) and all(
        session_replay.get(key) is True for key in
        ("without_normalized_evidence", "graph_hash_match", "path_hash_match",
         "occurrence_hash_match", "unresolved_identity_match"))
    shared_gates_pass = not reasons
    raw_safe = shared_gates_pass and normalized_present
    normalized_safe = raw_safe and session_present and session_replay_pass
    normalized_reasons = list(reasons)
    if not session_present:
        normalized_reasons.append("SESSION_STORE_MISSING_OR_MISMATCH")
    if not session_replay_pass:
        normalized_reasons.append("SESSION_REPLAY_WITHOUT_NORMALIZED_EVIDENCE_NOT_PROVEN")
    return {"status": "RAW_BINARY_DELETE_SAFE" if raw_safe else "KEEP",
            "raw_binary_delete_safe": raw_safe,
            "normalized_evidence_delete_safe": normalized_safe,
            "reasons": reasons, "normalized_reasons": normalized_reasons,
            "accounting": accounting}


def classify_file(path: Path) -> tuple[str, str, str]:
    name = path.name.lower()
    if CAPSULE_RE.fullmatch(path.name):
        return "AUTO67_CAPSULE", "AUTO67", "KEEP_ONLY_WITH_CAPSULE_WITNESS_SEAL"
    if re.fullmatch(r"live-discovery-wave-\d+\.bin", name):
        return "W3_V2_RECORD_CHUNK", "W6_DISCOVERY", "LOSSLESS_NORMALIZED_EVIDENCE_NOT_PROVEN"
    if name in RAW_NAMES:
        schema, producer = RAW_NAMES[name]
        reason = "FLOW_HAS_FIELDS_NOT_PROVEN_LOSSLESS_IN_SESSION" if schema == "FLOW_V1" \
            else "UNKNOWN_RAW_FORMAT"
        return schema, producer, reason
    if path.suffix.lower() in RAW_SUFFIXES:
        return "UNKNOWN_RAW_FORMAT", "UNKNOWN", "UNKNOWN_FORMAT"
    if name in {"flow-v1-segments.jsonl", "segment-audits.jsonl"}:
        return "FLOW_SEGMENT_INDEX", "LIVE_FORWARD_WORKER", "PROCESSING_REQUIRED_WITH_RAW"
    if name.endswith("receipt.json"):
        return "RECEIPT", "PIPELINE", "RETAINED_CONTROL_ARTIFACT"
    return "NON_RAW", "UNKNOWN", "OUT_OF_RAW_SCOPE"


def audit_tree(root: Path) -> dict[str, Any]:
    root = root.resolve()
    files: list[dict[str, Any]] = []
    totals: dict[str, dict[str, int]] = {}
    scanned = 0
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        scanned += 1
        schema, producer, reason = classify_file(path)
        if schema in {"NON_RAW", "RECEIPT"} and not (path.suffix.lower() == ".bin" and
                                         path.stat().st_size >= CHUNK_BYTES):
            continue
        if schema == "NON_RAW":
            schema, producer, reason = "UNKNOWN_BINARY", "UNKNOWN", "UNKNOWN_FORMAT"
        size = path.stat().st_size
        category = "UNKNOWN" if reason == "UNKNOWN_FORMAT" else (
            "RAW_BINARY_DELETE_SAFE" if reason == "" else
            "NORMALIZATION_REQUIRED" if schema in {
                "FLOW_V1", "AUTO67_CAPSULE", "W3_V2_RECORD_CHUNK"} else
            "PROCESSING_REQUIRED" if schema == "FLOW_SEGMENT_INDEX" else
            "KEEP")
        file_item: dict[str, Any] = {"path": str(path), "size_bytes": size,
            "sha256": file_sha256(path), "producer": producer, "schema": schema,
            "status": category, "reason": reason}
        if category in {"RAW_BINARY_DELETE_SAFE", "NORMALIZATION_REQUIRED",
                        "PROCESSING_REQUIRED", "KEEP", "UNKNOWN"}:
            summary = totals.setdefault(category, {"files": 0, "bytes": 0})
            summary["files"] += 1
            summary["bytes"] += size
        files.append(file_item)
    binary_count = sum(1 for item in files if item["schema"] not in {
        "FLOW_SEGMENT_INDEX", "RECEIPT"})
    return {"schema": SCHEMA, "mode": "AUDIT_ONLY", "root": str(root),
            "files_examined": scanned, "artifacts_audited": len(files),
            "raw_binary_files": binary_count,
            "raw_binary_bytes": sum(x["size_bytes"] for x in files
                                    if item_is_binary(x)),
            "support_files": len(files) - binary_count,
            "support_bytes": sum(x["size_bytes"] for x in files
                                 if not item_is_binary(x)),
            "totals": totals, "files": files,
            "automatic_delete_performed": False}


def item_is_binary(item: dict[str, Any]) -> bool:
    return item["schema"] not in {"FLOW_SEGMENT_INDEX", "RECEIPT"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--audit", type=Path,
                      help="evidence directory to scan (read-only)")
    mode.add_argument("--mark-delete-safe", action="store_true",
                      help="evaluate and write a separate deletion seal")
    parser.add_argument("--receipt", type=Path,
                        help="raw-elimination receipt for --mark-delete-safe")
    parser.add_argument("--current-versions", type=Path,
                        help="JSON map of current format/extractor/normalizer/validator hashes")
    parser.add_argument("--output", type=Path, required=True,
                        help="JSON inventory or seal output path")
    args = parser.parse_args()
    if args.mark_delete_safe:
        if args.receipt is None or args.current_versions is None:
            parser.error("--mark-delete-safe requires --receipt and --current-versions")
        receipt_bytes = args.receipt.read_bytes()
        receipt = json.loads(receipt_bytes)
        versions = json.loads(args.current_versions.read_text(encoding="utf-8"))
        report = {"schema": "oasis.raw-elimination.seal.v1",
            "receipt_sha256": hashlib.sha256(receipt_bytes).hexdigest(),
            "evaluation": evaluate_seal(receipt, args.receipt.parent, versions),
            "automatic_delete_performed": False}
    else:
        report = audit_tree(args.audit)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in
        (key for key in ("schema", "mode", "root", "files_examined", "artifacts_audited",
                         "raw_binary_files", "raw_binary_bytes", "totals", "evaluation")
         if key in report)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
