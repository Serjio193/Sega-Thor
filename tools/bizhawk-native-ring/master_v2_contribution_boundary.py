"""Run-scoped MASTER V2 contribution reconciliation and extension helpers."""
from __future__ import annotations

from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import struct
from typing import Any, Callable

from master_v2_shadow import (EXTENDED_SECTION_NAMES, FOOTER_MAGIC, MAGIC, SCHEMA,
                              SECTION_NAMES, read_master_v2_section)

SECTION = "run_contributions"
LEDGER_SCHEMA = "oasis.m12.master-v2.run-contributions.v1"
STOP_CONFLICT = "STOP_RUN_CONTRIBUTION_CONFLICT"


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=True, sort_keys=True,
                      separators=(",", ":")).encode("utf-8")


def contribution_hash(contribution: dict[str, Any]) -> str:
    value = {key: item for key, item in contribution.items()
             if key not in {"contribution_hash", "commit_state",
                            "result_master_generation", "result_master_hash"}}
    return hashlib.sha256(canonical_bytes(value)).hexdigest()


def _run_key(value: Any) -> str:
    try:
        run_id = int(value)
    except (TypeError, ValueError) as error:
        raise ValueError("STOP_RUN_CONTRIBUTION_INVALID_RUN_ID") from error
    if run_id <= 0:
        raise ValueError("STOP_RUN_CONTRIBUTION_INVALID_RUN_ID")
    return str(run_id)


def _map(state: dict[str, Any], name: str) -> dict[str, int]:
    aggregates = state.setdefault("aggregates", {})
    value = aggregates.setdefault(name, {})
    if not isinstance(value, dict):
        raise ValueError("STOP_RUN_CONTRIBUTION_BAD_AGGREGATES")
    return value


def _increment(target: dict[str, int], additions: Any) -> None:
    if not isinstance(additions, dict):
        return
    for key, value in additions.items():
        target[str(key)] = int(target.get(str(key), 0)) + int(value)


def _ledger(state: dict[str, Any]) -> list[dict[str, Any]]:
    value = state.setdefault("run_contributions", [])
    if not isinstance(value, list):
        raise ValueError("STOP_RUN_CONTRIBUTION_BAD_LEDGER")
    return value


def _payload(contribution: dict[str, Any]) -> dict[str, Any]:
    nested = contribution.get("contribution")
    return nested if isinstance(nested, dict) else contribution


def apply_contribution(state: dict[str, Any], contribution: dict[str, Any]) -> dict[str, Any]:
    """Apply one contribution exactly once, including repeated fact occurrences."""
    payload = _payload(contribution)
    run_id = _run_key(payload.get("run_id", contribution.get("run_id")))
    digest = str(contribution.get("contribution_hash") or contribution_hash(payload))
    existing = next((item for item in _ledger(state)
                     if _run_key(item.get("run_id")) == run_id), None)
    if existing is not None:
        if existing.get("contribution_hash") == digest:
            return {"status": "NO_DELTA_ALREADY_APPLIED", "run_id": int(run_id),
                    "contribution_hash": digest}
        raise ValueError(f"{STOP_CONFLICT}:{run_id}")
    record = deepcopy(payload)
    record["run_id"] = int(run_id)
    record["contribution_hash"] = digest
    record["commit_state"] = "COMMITTED_EXACT"
    _increment(_map(state, "instruction_occurrences"),
               contribution.get("instruction_occurrences"))
    _increment(_map(state, "relation_occurrences"),
               contribution.get("relation_occurrences"))
    _increment(_map(state, "terminal_occurrences"),
               contribution.get("terminal_occurrences"))
    _ledger(state).append(record)
    _ledger(state).sort(key=lambda item: int(item["run_id"]))
    return {"status": "APPLIED_ONCE", "run_id": int(run_id),
            "contribution_hash": digest}


def verify_only(state: dict[str, Any], contribution: dict[str, Any]) -> dict[str, Any]:
    """Verify a prior contribution without changing counters or the ledger."""
    payload = _payload(contribution)
    run_id = _run_key(payload.get("run_id", contribution.get("run_id")))
    digest = str(contribution.get("contribution_hash") or contribution_hash(payload))
    existing = next((item for item in _ledger(state)
                     if _run_key(item.get("run_id")) == run_id), None)
    if existing is None:
        return {"status": "NOT_COMMITTED", "run_id": int(run_id),
                "contribution_hash": digest}
    if existing.get("contribution_hash") != digest:
        raise ValueError(f"{STOP_CONFLICT}:{run_id}")
    return {"status": "COMMITTED_EXACT", "run_id": int(run_id),
            "contribution_hash": digest}


def forensic_replay(state: dict[str, Any], calculate: Callable[[], dict[str, Any]]) -> dict[str, Any]:
    """Run a contribution calculation while proving the supplied state is unchanged."""
    before = canonical_bytes(state)
    result = calculate()
    after = canonical_bytes(state)
    if before != after:
        raise ValueError("STOP_RECONCILIATION_FORENSIC_MUTATION")
    return {"status": "FORENSIC_ONLY", "contribution": result}


def mark_absorbed_exact(record: dict[str, Any], stages: dict[str, str]) -> dict[str, Any]:
    required = {"STAGE_5": {"PASS", "NO_DELTA"}, "STAGE_6": {"PASS", "NO_DELTA"},
                "STAGE_7": {"PASS", "NO_DELTA"}, "STAGE_8": {"PASS"}}
    if any(stages.get(stage) not in accepted for stage, accepted in required.items()):
        raise ValueError("STOP_ABSORPTION_PROOF_INCOMPLETE")
    value = dict(record); value["commit_state"] = "ABSORBED_EXACT"
    return value


def read_contribution_section(master_path: Path) -> dict[str, Any]:
    try:
        value = json.loads(read_master_v2_section(master_path, SECTION))
    except (OSError, ValueError, json.JSONDecodeError) as error:
        raise ValueError("STOP_MASTER_V2_RUN_CONTRIBUTIONS_UNAVAILABLE") from error
    if value.get("schema") != LEDGER_SCHEMA or not isinstance(value.get("runs"), list):
        raise ValueError("STOP_MASTER_V2_RUN_CONTRIBUTIONS_INVALID")
    return value


class MasterContributionView:
    """Read-only run contribution authority; no legacy fallback is available."""

    def __init__(self, master_path: Path):
        self.path = Path(master_path).resolve()
        self.state = read_contribution_section(self.path)

    @property
    def runs(self) -> list[dict[str, Any]]:
        return list(self.state["runs"])

    def for_run(self, run_id: int) -> dict[str, Any] | None:
        return next((item for item in self.runs if int(item.get("run_id", -1)) == int(run_id)), None)


def extend_master_v2(source: Path, output: Path, ledger: dict[str, Any],
                     section_overrides: dict[str, bytes] | None = None) -> dict[str, Any]:
    """Stream an existing verified V2 file and add one hashed run_contributions section."""
    payload = canonical_bytes(ledger)
    section_overrides = section_overrides or {}
    temporary = output.with_name(output.name + f".tmp-{os.getpid()}")
    sections: list[dict[str, Any]] = []
    try:
        with source.open("rb") as src, temporary.open("w+b") as dst:
            if src.read(len(MAGIC)) != MAGIC:
                raise ValueError("MASTER_V2_BAD_MAGIC")
            header_size = struct.unpack("<Q", src.read(8))[0]
            header = json.loads(src.read(header_size))
            source_sections = tuple(header.get("sections", ()))
            if source_sections not in (SECTION_NAMES, EXTENDED_SECTION_NAMES):
                raise ValueError("MASTER_V2_MISSING_REQUIRED_SECTION")
            new_header = canonical_bytes({"schema": SCHEMA, "sections": EXTENDED_SECTION_NAMES})
            dst.write(MAGIC); dst.write(struct.pack("<Q", len(new_header))); dst.write(new_header)
            for expected in SECTION_NAMES:
                name_size = struct.unpack("<H", src.read(2))[0]
                name = src.read(name_size).decode("ascii")
                if name != expected:
                    raise ValueError("MASTER_V2_SECTION_ORDER")
                size = struct.unpack("<Q", src.read(8))[0]
                expected_hash = src.read(32).hex()
                record_start = dst.tell(); dst.write(struct.pack("<H", len(name)))
                dst.write(name.encode("ascii")); length_at = dst.tell(); dst.write(b"\0" * 8)
                hash_at = dst.tell(); dst.write(b"\0" * 32)
                replacement = section_overrides.get(name)
                digest = hashlib.sha256(); remaining = size
                while remaining:
                    block = src.read(min(1024 * 1024, remaining))
                    if not block: raise ValueError("MASTER_V2_TRUNCATED_SECTION")
                    digest.update(block)
                    if replacement is None:
                        dst.write(block)
                    remaining -= len(block)
                source_digest = digest.hexdigest()
                if source_digest != expected_hash:
                    raise ValueError(f"MASTER_V2_SECTION_HASH:{name}")
                if replacement is not None:
                    digest = hashlib.sha256(replacement); size = len(replacement)
                    dst.write(replacement)
                end = dst.tell(); dst.seek(length_at); dst.write(struct.pack("<Q", size))
                dst.seek(hash_at); dst.write(bytes.fromhex(digest.hexdigest())); dst.seek(end)
                sections.append({"name": name, "bytes": size, "sha256": digest.hexdigest()})
            if source_sections == EXTENDED_SECTION_NAMES:
                old_name_size = struct.unpack("<H", src.read(2))[0]
                old_name = src.read(old_name_size).decode("ascii")
                old_size = struct.unpack("<Q", src.read(8))[0]
                old_hash = src.read(32).hex()
                old_digest = hashlib.sha256(); remaining = old_size
                while remaining:
                    block = src.read(min(1024 * 1024, remaining))
                    if not block: raise ValueError("MASTER_V2_TRUNCATED_SECTION")
                    old_digest.update(block); remaining -= len(block)
                if old_name != SECTION or old_digest.hexdigest() != old_hash:
                    raise ValueError("MASTER_V2_SECTION_HASH:run_contributions")
                if src.read(len(FOOTER_MAGIC)) != FOOTER_MAGIC:
                    raise ValueError("MASTER_V2_MISSING_FOOTER")
                footer_size = struct.unpack("<Q", src.read(8))[0]
                if len(src.read(footer_size)) != footer_size:
                    raise ValueError("MASTER_V2_TRUNCATED_FOOTER")
            name = SECTION; digest = hashlib.sha256(payload).hexdigest(); size = len(payload)
            dst.write(struct.pack("<H", len(name))); dst.write(name.encode("ascii"))
            dst.write(struct.pack("<Q", size)); dst.write(bytes.fromhex(digest)); dst.write(payload)
            sections.append({"name": name, "bytes": size, "sha256": digest})
            footer_core = {"schema": SCHEMA, "sections": sections}
            overall = hashlib.sha256(canonical_bytes(footer_core)).hexdigest()
            footer = canonical_bytes({**footer_core, "overall_sha256": overall})
            dst.write(FOOTER_MAGIC); dst.write(struct.pack("<Q", len(footer))); dst.write(footer)
            dst.flush(); os.fsync(dst.fileno())
        os.replace(temporary, output)
    finally:
        temporary.unlink(missing_ok=True)
    return {"path": str(output), "bytes": output.stat().st_size,
            "sections": sections, "overall_sha256": overall}


__all__ = ["LEDGER_SCHEMA", "SECTION", "STOP_CONFLICT", "apply_contribution",
           "canonical_bytes", "contribution_hash", "extend_master_v2",
           "forensic_replay", "mark_absorbed_exact", "MasterContributionView",
           "read_contribution_section", "verify_only"]
