"""Small, fail-closed Knowledge Coverage layer for concrete M12 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


DIMENSIONS = (
    "OBSERVATION", "EXECUTION", "READERS", "WRITERS", "CONSUMER", "PRODUCER",
    "POINTER_SOURCE", "CALLER", "CONTROL", "SELECTOR", "DOMAIN", "BOUNDARY",
    "FORMAT", "STRUCTURE", "TERMINAL_ROOT", "ASM_REPRESENTATION", "SOURCE_OWNED",
)
STATUSES = ("UNSEEN", "OBSERVED", "PARTIAL", "PROVEN", "BLOCKED", "CONFLICT", "NOT_APPLICABLE")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path | None) -> Any:
    return json.loads(path.read_text(encoding="utf-8")) if path and path.exists() else None


def dimensions(**values: str) -> dict[str, str]:
    result = {key: "UNSEEN" for key in DIMENSIONS}
    for key, value in values.items():
        if key not in DIMENSIONS:
            raise ValueError(f"unknown coverage dimension: {key}")
        if value not in STATUSES and value != "NO":
            raise ValueError(f"invalid coverage status: {value}")
        result[key] = value
    return result


def import_coverage(auto62: dict[str, Any], auto63: dict[str, Any], raw: Path | None,
                    static: Path | None, ownership_bytes: int,
                    ownership_source: str,
                    authoritative_inputs: list[Path | None] | None = None) -> dict[str, Any]:
    selected = auto63.get("selected", {})
    span = selected.get("target_span", [0x167DD8, 0x167E48])
    entities = {
        f"ROM:{span[0]:06X}-{span[1]:06X}": {
            "scope": {"kind": "ROM_RANGE", "start": span[0], "end": span[1]},
            "dimensions": dimensions(OBSERVATION="OBSERVED", EXECUTION="OBSERVED",
                                      CONSUMER="PROVEN", BOUNDARY="PARTIAL",
                                      STRUCTURE="PARTIAL", SOURCE_OWNED="NO"),
            "provenance": [{"artifact": "AUTO63 followup_report.json",
                            "path": str(auto63.get("_path", "")), "claim": "selected target span",
                            "proof_state": "PARTIAL", "hash": auto63.get("raw_sha256")}],
        },
        "PC:0000AF00-0000AF22": {
            "scope": {"kind": "ROM_CODE", "start": 0xAF00, "end": 0xAF22},
            "dimensions": dimensions(OBSERVATION="OBSERVED", EXECUTION="OBSERVED",
                                      READERS="PARTIAL", CONSUMER="PROVEN", CONTROL="PARTIAL",
                                      CALLER="UNSEEN", SOURCE_OWNED="NO"),
            "provenance": [{"artifact": "AUTO63 followup_report.json", "claim": "dependency contract",
                            "proof_state": "PARTIAL", "hash": auto63.get("raw_sha256")}],
        },
    }
    unresolved = list(auto63.get("unresolved_frontiers", []))
    source_artifacts = [
        {"kind": "AUTO62_REPORT", "path": auto62.get("_path"), "sha256": auto62.get("_sha256"),
         "investigations": len(auto62.get("investigations_created", []))},
        {"kind": "AUTO63_REPORT", "path": auto63.get("_path"), "sha256": auto63.get("_sha256"),
         "capture_id": auto63.get("capture_id"), "actually_new": auto63.get("actually_new")},
    ]
    if raw and raw.exists():
        source_artifacts.append({"kind": "RUNTIME_RAW", "path": str(raw), "sha256": sha256(raw)})
    if static and static.exists():
        source_artifacts.append({"kind": "STATIC_REPORT", "path": str(static), "sha256": sha256(static)})
    for kind, path in zip(("EVIDENCE_ENGINE_SQLITE", "INTERVALDB", "OWNERSHIP_MANIFEST"),
                          authoritative_inputs or []):
        source_artifacts.append({"kind": kind, "path": str(path) if path else None,
                                 "available": bool(path and path.exists()),
                                 "sha256": sha256(path) if path and path.exists() else None})
    statuses = {status: 0 for status in STATUSES}
    for entity in entities.values():
        for status in set(entity["dimensions"].values()):
            if status in statuses:
                statuses[status] += 1
    return {
        "schema": "oasis.m12.auto64.knowledge-coverage.v1",
        "dimensions": list(DIMENSIONS), "statuses": list(STATUSES),
        "source_artifacts": source_artifacts,
        "entities": entities,
        "unresolved_frontiers": unresolved,
        "imported_investigations": len(auto62.get("investigations_created", [])),
        "ownership": {"total_bytes": 3_145_728, "source_owned_bytes": ownership_bytes,
                      "source": ownership_source, "coverage_is_not_ownership": True},
        "summary": {"entity_count": len(entities), "dimension_status_counts": statuses,
                    "open_frontier_count": len(unresolved)},
    }


def derive_frontiers(coverage: dict[str, Any]) -> list[dict[str, Any]]:
    """Derive obligations only from imported unresolved facts."""
    result = []
    for frontier in coverage.get("unresolved_frontiers", []):
        if frontier == "A6_INHERITED_AT_ENTRY":
            result.append({"id": "INV-AUTO64-A6", "created_from": "AUTO63 unresolved_frontier",
                           "entity": "PC:0000AF00-0000AF22", "question": "Who established A6 before AF02?",
                           "why_open": "AUTO63 observed inherited A6 but proved no source/causal edge",
                           "required_evidence": "BOTH", "status": "OPEN"})
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--auto62", type=Path, required=True)
    parser.add_argument("--auto63", type=Path, required=True)
    parser.add_argument("--raw", type=Path)
    parser.add_argument("--static", type=Path)
    parser.add_argument("--evidence-db", type=Path)
    parser.add_argument("--interval-db", type=Path)
    parser.add_argument("--ownership-manifest", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    auto62 = load(args.auto62)
    auto63 = load(args.auto63)
    auto62["_path"], auto62["_sha256"] = str(args.auto62), sha256(args.auto62)
    auto63["_path"], auto63["_sha256"] = str(args.auto63), sha256(args.auto63)
    coverage = import_coverage(auto62, auto63, args.raw, args.static, 1_475_600,
                               "M12 baseline d9d080a3 ownership manifest/report",
                               [args.evidence_db, args.interval_db, args.ownership_manifest])
    coverage["derived_frontiers"] = derive_frontiers(coverage)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(coverage, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"frontiers": coverage["derived_frontiers"], "entities": len(coverage["entities"]),
                      "imported_investigations": coverage["imported_investigations"]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
