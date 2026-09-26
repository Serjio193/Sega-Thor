"""Small closed-experiment receipts and explicit fail-closed raw cleanup."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
from typing import Any

try:
    from .raw_elimination import file_sha256
except ImportError:
    from raw_elimination import file_sha256


SCHEMA = "oasis.experiment-closure.v1"
FINAL = {"MERGED", "NO_NEW_KNOWLEDGE", "INVALID"}
COUNT_FIELDS = ("input_events", "merged_events", "already_known_events",
                "unresolved_events", "rejected_events")
MAP_CHECKS = ("objects_valid", "edge_endpoints_valid", "occurrences_valid",
              "paths_reference_occurrences", "ordering_valid", "capture_boundaries_valid",
              "loop_multiplicity_preserved", "alternate_tails_preserved",
              "unresolved_preserved", "conflicts_preserved", "rom_identities_valid")


def _sha(value: Any) -> bool:
    return isinstance(value, str) and len(value) == 64 and all(
        char in "0123456789abcdef" for char in value)


def _map_identity(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict) or not isinstance(value.get("generation"), str) or \
            not value["generation"] or not _sha(value.get("map_sha256")):
        raise ValueError("STOP_MAP_GENERATION_IDENTITY_INVALID:" + label)
    return {"generation": value["generation"], "map_sha256": value["map_sha256"]}


def build_receipt(*, experiment_id: str, raw_path: Path, raw_root: Path,
                  format_name: str, producer: str, schema_version: str,
                  counts: dict[str, int], map_before: dict[str, Any],
                  map_after: dict[str, Any], result: str,
                  map_self_check: dict[str, Any], source_owned_before: int,
                  source_owned_after: int, new_nodes: int = 0,
                  new_edges: int = 0, new_occurrences: int = 0,
                  new_paths: int = 0, new_unresolved: int = 0,
                  invalid_reason: str | None = None,
                  temporary_artifacts: list[Path] | None = None) -> dict[str, Any]:
    """Create a closed receipt only from complete accounting and map proof."""
    if not experiment_id or result not in FINAL:
        raise ValueError("STOP_EXPERIMENT_ID_OR_FINAL_STATUS_INVALID")
    root = Path(raw_root).resolve()
    raw = Path(raw_path).resolve()
    try:
        relative = raw.relative_to(root)
    except ValueError as exc:
        raise ValueError("STOP_RAW_OUTSIDE_AUDITED_ROOT") from exc
    if not raw.is_file():
        raise FileNotFoundError("STOP_RAW_MISSING")
    required = {key: counts.get(key) for key in COUNT_FIELDS}
    if any(type(value) is not int or value < 0 for value in required.values()):
        raise ValueError("STOP_EVENT_ACCOUNTING_INVALID")
    if required["input_events"] != sum(required[key] for key in COUNT_FIELDS[1:]):
        raise ValueError("STOP_EVENT_ACCOUNTING_INCOMPLETE")
    before, after = _map_identity(map_before, "before"), _map_identity(map_after, "after")
    if not isinstance(map_self_check, dict) or map_self_check.get("status") != "PASS" or \
            map_self_check.get("map_sha256") != after["map_sha256"] or \
            any(map_self_check.get(key) is not True for key in MAP_CHECKS):
        raise ValueError("STOP_CANONICAL_MAP_SELF_CHECK_FAILED")
    if source_owned_before != source_owned_after:
        raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
    if result == "MERGED" and before == after:
        raise ValueError("STOP_MERGED_WITHOUT_MAP_GENERATION_CHANGE")
    if result == "NO_NEW_KNOWLEDGE" and before != after:
        raise ValueError("STOP_NO_NEW_KNOWLEDGE_MAP_CHANGED")
    if result == "INVALID" and before != after:
        raise ValueError("STOP_INVALID_INPUT_MUTATED_MAP")
    if result == "INVALID" and not invalid_reason:
        raise ValueError("STOP_INVALID_REASON_MISSING")
    size = raw.stat().st_size
    temporary = []
    for artifact_path in temporary_artifacts or []:
        artifact = Path(artifact_path).resolve()
        try:
            artifact_relative = artifact.relative_to(root)
        except ValueError as exc:
            raise ValueError("STOP_TEMPORARY_ARTIFACT_OUTSIDE_AUDITED_ROOT") from exc
        if not artifact.is_file() or artifact == raw:
            raise ValueError("STOP_TEMPORARY_ARTIFACT_INVALID")
        temporary.append({"path": artifact_relative.as_posix(),
            "sha256": file_sha256(artifact), "bytes": artifact.stat().st_size})
    if len({item["path"] for item in temporary}) != len(temporary):
        raise ValueError("STOP_DUPLICATE_TEMPORARY_ARTIFACT")
    return {"schema": SCHEMA, "experiment_id": experiment_id,
        "raw_file": {"path": relative.as_posix(), "sha256": file_sha256(raw),
                     "bytes": size},
        "raw_root": str(root), "format": format_name, "producer": producer,
        "temporary_artifacts": temporary,
        "schema_version": schema_version, "event_counts": required,
        "accounted_events": required["input_events"], "unaccounted_events": 0,
        "map_generation_before": before, "map_generation_after": after,
        "map_self_check": {"status": "PASS", "map_sha256": after["map_sha256"],
                            **{key: True for key in MAP_CHECKS}},
        "new_nodes": new_nodes, "new_edges": new_edges,
        "new_occurrences": new_occurrences, "new_paths": new_paths,
        "new_unresolved": new_unresolved,
        "source_owned_before": source_owned_before,
        "source_owned_after": source_owned_after, "source_owned_delta": 0,
        "final_status": result, "invalid_reason": invalid_reason,
        "experiment_closed": True, "raw_disposable": True}


def write_receipt(path: Path, receipt: dict[str, Any]) -> None:
    """Publish receipt atomically, never overwriting an existing closure."""
    path = Path(path)
    if path.exists():
        raise FileExistsError("STOP_RECEIPT_ALREADY_EXISTS")
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix=path.name + ".", suffix=".partial",
                                        dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(receipt, stream, sort_keys=True, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
    except Exception:
        Path(name).unlink(missing_ok=True)
        raise


def _eligible(receipt_path: Path, audited_root: Path) -> tuple[Path, dict[str, Any]]:
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    if receipt.get("schema") != SCHEMA or receipt.get("experiment_closed") is not True or \
            receipt.get("raw_disposable") is not True or receipt.get("final_status") not in FINAL:
        raise ValueError("STOP_EXPERIMENT_NOT_CLOSED:" + receipt_path.name)
    root = audited_root.resolve()
    if Path(receipt.get("raw_root", "")).resolve() != root:
        raise ValueError("STOP_RECEIPT_ROOT_MISMATCH:" + receipt_path.name)
    raw_data = receipt.get("raw_file")
    if not isinstance(raw_data, dict) or not isinstance(raw_data.get("path"), str) or \
            type(raw_data.get("bytes")) is not int or not _sha(raw_data.get("sha256")):
        raise ValueError("STOP_RAW_RECEIPT_IDENTITY_INVALID:" + receipt_path.name)
    raw = (root / raw_data["path"]).resolve()
    try:
        raw.relative_to(root)
    except ValueError as exc:
        raise ValueError("STOP_RAW_OUTSIDE_AUDITED_ROOT") from exc
    if not raw.is_file() or raw.stat().st_size != raw_data["bytes"] or \
            file_sha256(raw) != raw_data["sha256"]:
        raise ValueError("STOP_RAW_IDENTITY_MISMATCH:" + receipt_path.name)
    self_check = receipt.get("map_self_check")
    if receipt.get("unaccounted_events") != 0 or not isinstance(self_check, dict) or \
            self_check.get("status") != "PASS" or \
            receipt.get("source_owned_delta") != 0:
        raise ValueError("STOP_CLOSURE_PROOF_INVALID:" + receipt_path.name)
    counts = receipt.get("event_counts")
    if not isinstance(counts, dict) or any(type(counts.get(key)) is not int or
            counts[key] < 0 for key in COUNT_FIELDS) or \
            sum(counts[key] for key in COUNT_FIELDS[1:]) != counts.get("input_events") or \
            receipt.get("accounted_events") != counts.get("input_events"):
        raise ValueError("STOP_EVENT_ACCOUNTING_INVALID:" + receipt_path.name)
    before = _map_identity(receipt.get("map_generation_before"), "before")
    after = _map_identity(receipt.get("map_generation_after"), "after")
    if self_check.get("map_sha256") != after["map_sha256"] or \
            any(self_check.get(key) is not True for key in MAP_CHECKS):
        raise ValueError("STOP_CANONICAL_MAP_SELF_CHECK_FAILED:" + receipt_path.name)
    status = receipt["final_status"]
    if (status == "MERGED" and before == after) or \
            (status in {"NO_NEW_KNOWLEDGE", "INVALID"} and before != after) or \
            (status == "INVALID" and not receipt.get("invalid_reason")) or \
            receipt.get("source_owned_before") != receipt.get("source_owned_after"):
        raise ValueError("STOP_CLOSURE_RESULT_INCONSISTENT:" + receipt_path.name)
    temporary = receipt.get("temporary_artifacts", [])
    if not isinstance(temporary, list):
        raise ValueError("STOP_TEMPORARY_ARTIFACT_LIST_INVALID")
    targets = {raw}
    for artifact in temporary:
        if not isinstance(artifact, dict) or not isinstance(artifact.get("path"), str) or \
                type(artifact.get("bytes")) is not int or not _sha(artifact.get("sha256")):
            raise ValueError("STOP_TEMPORARY_ARTIFACT_IDENTITY_INVALID")
        path = (root / artifact["path"]).resolve()
        try:
            path.relative_to(root)
        except ValueError as exc:
            raise ValueError("STOP_TEMPORARY_ARTIFACT_OUTSIDE_AUDITED_ROOT") from exc
        if path in targets or not path.is_file() or path.stat().st_size != artifact["bytes"] or \
                file_sha256(path) != artifact["sha256"]:
            raise ValueError("STOP_TEMPORARY_ARTIFACT_IDENTITY_MISMATCH")
        targets.add(path)
    return raw, receipt


def cleanup_closed(receipt_paths: list[Path], audited_root: Path,
                   *, execute: bool = False) -> dict[str, Any]:
    """Delete only exact, hash-verified raw paths named by closed receipts."""
    if not receipt_paths:
        raise ValueError("STOP_NO_EXPERIMENT_RECEIPTS")
    planned: list[dict[str, Any]] = []
    seen: set[Path] = set()
    for receipt_path in receipt_paths:
        raw, receipt = _eligible(Path(receipt_path), Path(audited_root))
        targets = [(raw, receipt["raw_file"], "raw")]
        targets.extend(((Path(audited_root).resolve() / artifact["path"]).resolve(),
                        artifact, "temporary")
                       for artifact in receipt.get("temporary_artifacts", []))
        for path, artifact, kind in targets:
            if path in seen:
                raise ValueError("STOP_DUPLICATE_RAW_TARGET")
            seen.add(path)
            planned.append({"path": str(path), "bytes": artifact["bytes"], "kind": kind,
                            "experiment_id": receipt["experiment_id"],
                            "receipt": str(Path(receipt_path).resolve())})
    if execute:
        # Revalidate every receipt and hash before the first irreversible unlink.
        raw_plans = [item for item in planned if item["kind"] == "raw"]
        for item, receipt_path in zip(raw_plans, receipt_paths):
            raw, _ = _eligible(Path(receipt_path), Path(audited_root))
            if str(raw) != item["path"]:
                raise ValueError("STOP_RAW_TARGET_CHANGED_DURING_CLEANUP")
        for item in planned:
            Path(item["path"]).unlink()
    return {"status": "DELETED" if execute else "PLAN_ONLY", "files": planned,
            "bytes": sum(item["bytes"] for item in planned),
            "automatic_wildcard_delete": False}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cleanup", choices=("cleanup",))
    parser.add_argument("--closed-only", action="store_true", required=True)
    parser.add_argument("--root", type=Path, required=True, help="exact audited raw root")
    parser.add_argument("--receipts", type=Path, required=True,
                        help="directory containing *.experiment.json closure receipts")
    parser.add_argument("--execute", action="store_true",
                        help="unlink only validated raw files listed by closed receipts")
    args = parser.parse_args()
    receipts_root = args.receipts.resolve()
    receipt_paths = sorted(receipts_root.glob("*.experiment.json"))
    plan = cleanup_closed(receipt_paths, args.root)
    print(json.dumps(plan, sort_keys=True, indent=2))
    if args.execute:
        result = cleanup_closed(receipt_paths, args.root, execute=True)
        print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
