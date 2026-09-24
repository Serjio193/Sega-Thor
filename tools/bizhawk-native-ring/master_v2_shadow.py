"""Build and verify a deterministic, self-contained MASTER V2 shadow file."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import struct
import sys
from typing import Any, BinaryIO

SCHEMA = "oasis.m12.master-v2-shadow.v1"
MAGIC = b"OASIS-M12-MASTER-V2\x00"
FOOTER_MAGIC = b"M12-V2-FOOTER\x00"
SECTION_NAMES = ("meta", "rolling_master", "canonical_map_master",
                 "canonical_knowledge", "stage_outcomes", "provenance",
                 "outcomes", "absorption_history")
EXTENDED_SECTION_NAMES = SECTION_NAMES + ("run_contributions",)
ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
TABLES = {
    "rolling_master": ("meta", "run", "audit", "instruction", "edge"),
    "canonical_map_master": ("map_meta", "map_import", "map_node", "map_edge",
                              "map_frontier", "map_conflict"),
    "canonical_knowledge": ("map_meta", "source_artifact", "rom_range", "rom_object",
                            "claim", "relation", "evidence_ref", "emission", "conflict",
                            "map_import", "derivation", "derivation_input", "map_proposal",
                            "map_proposal_operation"),
}
PATH_KEYS = {"path", "raw_path", "index_path", "diagnostic_path", "session_path",
             "generation_dir", "materialized", "manifest_path", "delete_manifest",
             "absorption_receipt", "output_generation_or_same", "input_generation"}
VOLATILE_KEYS = {"timestamp", "timestamp_utc", "timestamp_monotonic", "last_heartbeat",
                 "backend_pid", "backend_started_at", "elapsed_seconds", "duration_seconds",
                 "last_progress_change_time"}


@dataclass(frozen=True)
class LegacyState:
    rolling_root: Path
    canonical_root: Path
    campaign_root: Path
    run_id: int
    rolling_pointer: dict[str, Any]
    canonical_pointer: dict[str, Any]
    rolling_generation: Path
    canonical_generation: Path
    run_analysis: Path


class _Sink:
    def __init__(self, stream: BinaryIO | None = None):
        self.stream, self.digest, self.size = stream, hashlib.sha256(), 0

    def write(self, data: bytes) -> None:
        self.digest.update(data)
        self.size += len(data)
        if self.stream is not None:
            self.stream.write(data)

    def finish(self) -> tuple[str, int]:
        return self.digest.hexdigest(), self.size


def _json_bytes(value: Any) -> bytes:
    return json.dumps(value, ensure_ascii=True, sort_keys=True,
                      separators=(",", ":")).encode("utf-8")


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _semantic(value: Any, key: str = "") -> Any:
    if isinstance(value, dict):
        return {name: _semantic(item, name) for name, item in sorted(value.items())
                if name not in VOLATILE_KEYS and name not in PATH_KEYS}
    if isinstance(value, list):
        return [_semantic(item, key) for item in value]
    if key in PATH_KEYS or key.endswith("_path") or key.endswith("_dir"):
        return Path(str(value)).name
    return value


def _value(value: Any) -> Any:
    if isinstance(value, bytes):
        return {"__bytes__": value.hex()}
    return value


def _table_order(db: sqlite3.Connection, table: str) -> tuple[str, ...]:
    columns = db.execute(f'PRAGMA table_info("{table}")').fetchall()
    primary = [str(row[1]) for row in columns if int(row[5])]
    return tuple(primary or [str(row[1]) for row in columns])


def _write_table_payload(sink: _Sink, path: Path, tables: tuple[str, ...]) -> None:
    db = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        sink.write(b'{"tables":{')
        for index, table in enumerate(tables):
            if index:
                sink.write(b",")
            columns = [str(row[1]) for row in db.execute(f'PRAGMA table_info("{table}")')]
            if not columns:
                sink.write(_json_bytes(table))
                sink.write(b":[]")
                continue
            order = ",".join('"' + column.replace('"', '""') + '"' for column in _table_order(db, table))
            sink.write(_json_bytes(table.encode("utf-8").decode("utf-8")))
            sink.write(b":[")
            query = f'SELECT {",".join(chr(34)+c.replace(chr(34), chr(34)*2)+chr(34) for c in columns)} FROM "{table}" ORDER BY {order}'
            for row_index, row in enumerate(db.execute(query)):
                if row_index:
                    sink.write(b",")
                sink.write(_json_bytes([_value(item) for item in row]))
            sink.write(b"]")
        sink.write(b"}}")
    finally:
        db.close()


def _find_run_analysis(state_root: Path, run_id: int) -> Path:
    candidates = sorted((state_root / "post-run-analysis").glob(f"run-{run_id}-*"))
    if len(candidates) != 1:
        raise ValueError(f"expected one run analysis directory for {run_id}, got {len(candidates)}")
    return candidates[0]


def discover(rolling_root: Path, canonical_root: Path, campaign_root: Path,
             run_id: int) -> LegacyState:
    rolling_root, canonical_root, campaign_root = (p.resolve() for p in
                                                   (rolling_root, canonical_root, campaign_root))
    rolling_pointer = _read_json(rolling_root / "current.json")
    canonical_pointer = _read_json(canonical_root / "current.json")
    rolling_generation = (rolling_root / rolling_pointer["generation_dir"]).resolve()
    canonical_generation = (canonical_root / canonical_pointer["generation_dir"]).resolve()
    for path in (rolling_generation / "master.sqlite", canonical_generation / "master.sqlite",
                 canonical_generation / "knowledge.sqlite"):
        if not path.is_file():
            raise ValueError(f"authoritative legacy artifact missing: {path}")
    state = LegacyState(rolling_root, canonical_root, campaign_root, int(run_id),
                        rolling_pointer, canonical_pointer, rolling_generation,
                        canonical_generation, _find_run_analysis(rolling_root, int(run_id)))
    report = campaign_root / "post-run-analysis" / "report.json"
    report_value = _read_json(report) if report.is_file() else {}
    report_run_id = report_value.get("run_id", report_value.get("runtime", {}).get("run_id", -1))
    if not report.is_file() or int(report_run_id) != int(run_id):
        raise ValueError("accepted campaign report does not match requested run")
    return state


def _legacy_files(state: LegacyState) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []
    for prefix, root in (("rolling", state.rolling_generation), ("canonical", state.canonical_generation)):
        for path in sorted(root.iterdir()):
            if path.is_file():
                files.append((f"{prefix}/{path.name}", path))
    for label, path in (("rolling/current.json", state.rolling_root / "current.json"),
                        ("canonical/current.json", state.canonical_root / "current.json")):
        files.append((label, path))
    post = state.campaign_root / "post-run-analysis"
    for path in sorted(post.glob("*.json")):
        files.append((f"campaign/post-run-analysis/{path.name}", path))
    for path in sorted(state.run_analysis.rglob("*.json")):
        files.append((f"run-analysis/{path.relative_to(state.run_analysis).as_posix()}", path))
    return [(label, path) for label, path in files if path.is_file()]


def _meta_payload(state: LegacyState, file_manifest: list[dict[str, Any]],
                  generation_id: str) -> bytes:
    db = sqlite3.connect((state.rolling_generation / "master.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
    try:
        absorbed = [int(row[0]) for row in db.execute("SELECT run_id FROM run ORDER BY run_id")]
    finally:
        db.close()
    evidence_tools = Path(__file__).resolve().parents[2] / "src" / "tools" / "thor_evidence"
    if str(evidence_tools) not in sys.path:
        sys.path.insert(0, str(evidence_tools))
    try:
        from rom_knowledge_map import KnowledgeStore
        knowledge = KnowledgeStore(state.canonical_generation / "knowledge.sqlite",
                                   ROM_SHA, ROM_SIZE, read_only=True)
        try:
            logical_hashes = knowledge.hashes()
            knowledge_schema = knowledge.meta()["schema"]
        finally:
            knowledge.close()
    except (ImportError, KeyError, OSError, ValueError, sqlite3.Error) as exc:
        raise ValueError("MASTER_V2_CANONICAL_KNOWLEDGE_IDENTITY_INVALID") from exc
    value = {"schema": SCHEMA, "generation_id": generation_id,
             "parent_generation": state.rolling_pointer.get("generation_dir"),
             "parent_master_sha256": state.rolling_pointer.get("master_sha256"),
             "canonical_generation": state.canonical_pointer.get("generation_dir"),
             "canonical_master_sha256": state.canonical_pointer.get("master_sha256"),
             "canonical_knowledge_sha256": state.canonical_pointer.get("knowledge_sha256"),
             "canonical_knowledge_schema": knowledge_schema,
             "canonical_logical_hashes": logical_hashes,
             "rom": {"sha256": ROM_SHA, "size": ROM_SIZE},
             "absorbed_run_ids": absorbed, "source_owned_delta": 0,
             "legacy_input_manifest": file_manifest}
    return _json_bytes(value)


def _stages_payload(state: LegacyState) -> bytes:
    campaign_post = state.campaign_root / "post-run-analysis"
    values: dict[str, Any] = {}
    for name in ("report.json", "status.json"):
        path = campaign_post / name
        if path.is_file():
            values[name[:-5]] = _semantic(_read_json(path))
    absorbed = sorted(campaign_post.glob(f"absorbed-run-{state.run_id}.json"))
    if absorbed:
        values["absorption"] = _semantic(_read_json(absorbed[0]))
    for path in sorted(state.run_analysis.rglob("*.json")):
        values[f"run-analysis/{path.relative_to(state.run_analysis).as_posix()}"] = _semantic(_read_json(path))
    return _json_bytes(values)


def _run_json(state: LegacyState, name: str) -> dict[str, Any]:
    path = state.run_analysis / name
    return _semantic(_read_json(path)) if path.is_file() else {}


def _status(receipt: dict[str, Any], default: str = "PASS") -> str:
    value = receipt.get("status") or receipt.get("state")
    if value:
        return str(value)
    if receipt.get("stop_code") or receipt.get("stop_message"):
        return "STOP"
    return default


def legacy_provenance_projection(state: LegacyState) -> dict[str, Any]:
    """Return the structured Stage 6 facts used for the R3 shadow comparison."""
    receipt = _run_json(state, "stage6-control-provenance.json")
    return {"schema": "oasis.m12.master-v2.provenance.v1", "run_id": state.run_id,
            "stage": "CONTROL PROVENANCE", "status": _status(receipt, "NO_DELTA"),
            "input_hashes": {key: receipt.get(key) for key in
                             ("raw_flow_sha256", "canonical_generation")},
            "relations": {key: receipt.get(key) for key in
                          ("indirect_JMP_occurrences", "indirect_JSR_occurrences",
                           "resolved_jump_table_entries", "resolved_offset_chains",
                           "resolved_pointer_chains")},
            "counters": {key: receipt.get(key) for key in
                         ("consumers_total", "unresolved_consumers", "unsupported_transforms",
                          "predecessor_failures", "identity_conflicts", "records_processed",
                          "segments_processed", "segments_total")},
            "witness": receipt.get("first_offending_consumer"), "receipt": receipt}


def legacy_outcomes_projection(state: LegacyState) -> dict[str, Any]:
    """Project accepted Stage 5-9 receipts into one stable semantic object."""
    stage5 = _run_json(state, "stage5-receipt.json")
    stage6 = _run_json(state, "stage6-control-provenance.json")
    stage7 = _run_json(state, "stage7/stage7-result.json")
    stage8 = _run_json(state, "stage8-receipt.json")
    absorption = {}
    for path in sorted((state.campaign_root / "post-run-analysis").glob(
            f"absorbed-run-{state.run_id}.json")):
        absorption = _semantic(_read_json(path)); break
    report_path = state.campaign_root / "post-run-analysis" / "report.json"
    report = _semantic(_read_json(report_path)) if report_path.is_file() else {}
    for stage, receipt, default in (("STAGE_5", stage5, "PASS"),
                                    ("STAGE_6", stage6, "NO_DELTA"),
                                    ("STAGE_7", stage7, "NO_DELTA"),
                                    ("STAGE_8", stage8, "PASS"),
                                    ("STAGE_9", absorption, "PASS")):
        if receipt and "status" not in receipt and "state" not in receipt:
            receipt["status"] = _status(receipt, default)
    stages = {"STAGE_5": stage5, "STAGE_6": stage6, "STAGE_7": stage7,
              "STAGE_8": stage8, "STAGE_9": absorption}
    return {"schema": "oasis.m12.master-v2.outcomes.v1", "run_id": state.run_id,
            "stages": stages, "pipeline": report.get("pipeline_state"),
            "source_owned_delta": report.get("source_owned_delta", 0),
            "report": report}


def legacy_absorption_projection(state: LegacyState) -> dict[str, Any]:
    """Build compact absorbed-run history without depending on raw FLOW files."""
    db = sqlite3.connect((state.rolling_generation / "master.sqlite").resolve().as_uri() + "?mode=ro", uri=True)
    try:
        columns = [str(row[1]) for row in db.execute('PRAGMA table_info("run")')]
        rows = [dict(zip(columns, row)) for row in db.execute('SELECT * FROM "run" ORDER BY 1')]
    finally:
        db.close()
    receipts = []
    for path in sorted((state.campaign_root / "post-run-analysis").glob("absorbed-run-*.json")):
        receipts.append(_semantic(_read_json(path)))
    return {"schema": "oasis.m12.master-v2.absorption-history.v1", "run_id": state.run_id,
            "rom": {"sha256": ROM_SHA, "size": ROM_SIZE}, "run_rows": rows,
            "absorbed_receipts": receipts, "current_run": state.run_id}


def _section_stream(state: LegacyState, name: str, sink: _Sink, meta: bytes | None = None) -> None:
    if name == "meta":
        assert meta is not None
        sink.write(meta)
    elif name == "stage_outcomes":
        sink.write(_stages_payload(state))
    elif name == "provenance":
        sink.write(_json_bytes(legacy_provenance_projection(state)))
    elif name == "outcomes":
        sink.write(_json_bytes(legacy_outcomes_projection(state)))
    elif name == "absorption_history":
        sink.write(_json_bytes(legacy_absorption_projection(state)))
    elif name == "rolling_master":
        _write_table_payload(sink, state.rolling_generation / "master.sqlite", TABLES[name])
    elif name == "canonical_map_master":
        _write_table_payload(sink, state.canonical_generation / "master.sqlite", TABLES[name])
    elif name == "canonical_knowledge":
        _write_table_payload(sink, state.canonical_generation / "knowledge.sqlite", TABLES[name])
    else:
        raise ValueError(f"unknown MASTER V2 section: {name}")


def legacy_section_hashes(state: LegacyState, meta: bytes) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for name in SECTION_NAMES:
        sink = _Sink()
        _section_stream(state, name, sink, meta)
        digest, size = sink.finish()
        result[name] = {"sha256": digest, "bytes": size}
    return result


def _file_manifest(state: LegacyState) -> list[dict[str, Any]]:
    return [{"label": label, "bytes": path.stat().st_size, "sha256": _sha(path)}
            for label, path in _legacy_files(state)]


def write_master_v2(state: LegacyState, output: Path) -> dict[str, Any]:
    manifest = _file_manifest(state)
    parent_seed = (str(state.rolling_pointer.get("master_sha256")) +
                   str(state.canonical_pointer.get("knowledge_sha256")) + str(state.run_id)).encode()
    generation_id = "master-v2-" + hashlib.sha256(parent_seed).hexdigest()[:16]
    meta = _meta_payload(state, manifest, generation_id)
    temporary = output.with_name(output.name + f".tmp-{os.getpid()}")
    sections: list[dict[str, Any]] = []
    try:
        with temporary.open("w+b") as stream:
            stream.write(MAGIC)
            header = _json_bytes({"schema": SCHEMA, "sections": SECTION_NAMES})
            stream.write(struct.pack("<Q", len(header))); stream.write(header)
            for name in SECTION_NAMES:
                encoded_name = name.encode("ascii")
                stream.write(struct.pack("<H", len(encoded_name))); stream.write(encoded_name)
                length_offset = stream.tell(); stream.write(b"\x00" * 8); hash_offset = stream.tell(); stream.write(b"\x00" * 32)
                sink = _Sink(stream)
                _section_stream(state, name, sink, meta)
                digest, size = sink.finish()
                end = stream.tell(); stream.seek(length_offset); stream.write(struct.pack("<Q", size)); stream.seek(hash_offset); stream.write(bytes.fromhex(digest)); stream.seek(end)
                sections.append({"name": name, "bytes": size, "sha256": digest})
            overall = hashlib.sha256(_json_bytes({"schema": SCHEMA, "sections": sections})).hexdigest()
            footer = _json_bytes({"schema": SCHEMA, "sections": sections, "overall_sha256": overall})
            stream.write(FOOTER_MAGIC); stream.write(struct.pack("<Q", len(footer))); stream.write(footer)
            stream.flush(); os.fsync(stream.fileno())
        os.replace(temporary, output)
    finally:
        temporary.unlink(missing_ok=True)
    return {"schema": SCHEMA, "generation_id": generation_id, "path": str(output),
            "bytes": output.stat().st_size, "sections": sections, "overall_sha256": overall,
            "legacy_persistent_bytes": sum(item["bytes"] for item in manifest),
            "legacy_files": len(manifest)}


def decode_master_v2(path: Path, materialize: bool = False) -> dict[str, Any]:
    with path.open("rb") as stream:
        if stream.read(len(MAGIC)) != MAGIC:
            raise ValueError("MASTER_V2_BAD_MAGIC")
        header_size = struct.unpack("<Q", stream.read(8))[0]
        header = json.loads(stream.read(header_size))
        header_sections = tuple(header.get("sections", ()))
        if header.get("schema") != SCHEMA or header_sections not in (SECTION_NAMES, EXTENDED_SECTION_NAMES):
            raise ValueError("MASTER_V2_MISSING_REQUIRED_SECTION")
        sections: list[dict[str, Any]] = []; payloads: dict[str, Any] = {}
        for expected in header_sections:
            name_size = struct.unpack("<H", stream.read(2))[0]; name = stream.read(name_size).decode("ascii")
            if name != expected:
                raise ValueError("MASTER_V2_SECTION_ORDER")
            size = struct.unpack("<Q", stream.read(8))[0]; expected_hash = stream.read(32).hex(); offset = stream.tell()
            digest = hashlib.sha256(); remaining = size; chunks: list[bytes] = []
            while remaining:
                block = stream.read(min(1024 * 1024, remaining));
                if not block: raise ValueError("MASTER_V2_TRUNCATED_SECTION")
                digest.update(block); remaining -= len(block)
                if materialize: chunks.append(block)
            actual = digest.hexdigest()
            if actual != expected_hash: raise ValueError(f"MASTER_V2_SECTION_HASH:{name}")
            item = {"name": name, "offset": offset, "bytes": size, "sha256": actual}; sections.append(item)
            if materialize: payloads[name] = json.loads(b"".join(chunks))
        if stream.read(len(FOOTER_MAGIC)) != FOOTER_MAGIC: raise ValueError("MASTER_V2_MISSING_FOOTER")
        footer_size = struct.unpack("<Q", stream.read(8))[0]; footer = json.loads(stream.read(footer_size))
        if footer.get("sections") != [{k: item[k] for k in ("name", "bytes", "sha256")} for item in sections]:
            raise ValueError("MASTER_V2_FOOTER_MISMATCH")
        overall = hashlib.sha256(_json_bytes({"schema": SCHEMA, "sections": footer["sections"]})).hexdigest()
        if overall != footer.get("overall_sha256"): raise ValueError("MASTER_V2_OVERALL_HASH")
        return {"schema": SCHEMA, "sections": sections, "overall_sha256": overall, "payloads": payloads}


def read_master_v2_section(path: Path, section_name: str) -> bytes:
    """Read one verified section without materializing the other sections."""
    with path.open("rb") as stream:
        if stream.read(len(MAGIC)) != MAGIC:
            raise ValueError("MASTER_V2_BAD_MAGIC")
        header_size = struct.unpack("<Q", stream.read(8))[0]
        header = json.loads(stream.read(header_size))
        header_sections = tuple(header.get("sections", ()))
        if header.get("schema") != SCHEMA or header_sections not in (SECTION_NAMES, EXTENDED_SECTION_NAMES) or section_name not in header_sections:
            raise ValueError("MASTER_V2_MISSING_REQUIRED_SECTION")
        found: bytes | None = None
        section_records: list[dict[str, Any]] = []
        for expected in header["sections"]:
            name_size = struct.unpack("<H", stream.read(2))[0]
            name = stream.read(name_size).decode("ascii")
            size = struct.unpack("<Q", stream.read(8))[0]
            expected_hash = stream.read(32).hex()
            digest = hashlib.sha256()
            chunks = bytearray() if name == section_name else None
            remaining = size
            while remaining:
                block = stream.read(min(1024 * 1024, remaining))
                if not block:
                    raise ValueError("MASTER_V2_TRUNCATED_SECTION")
                digest.update(block)
                if chunks is not None:
                    chunks.extend(block)
                remaining -= len(block)
            if digest.hexdigest() != expected_hash:
                raise ValueError(f"MASTER_V2_SECTION_HASH:{name}")
            section_records.append({"name": name, "bytes": size, "sha256": expected_hash})
            if name == section_name:
                found = bytes(chunks)
        if stream.read(len(FOOTER_MAGIC)) != FOOTER_MAGIC:
            raise ValueError("MASTER_V2_MISSING_FOOTER")
        footer_size = struct.unpack("<Q", stream.read(8))[0]
        footer = json.loads(stream.read(footer_size))
        if footer.get("sections") != section_records:
            raise ValueError("MASTER_V2_FOOTER_MISMATCH")
        overall = hashlib.sha256(_json_bytes({"schema": SCHEMA, "sections": section_records})).hexdigest()
        if overall != footer.get("overall_sha256"):
            raise ValueError("MASTER_V2_OVERALL_HASH")
        if found is None:
            raise ValueError(f"MASTER_V2_MISSING_REQUIRED_SECTION:{section_name}")
        return found


__all__ = ["LegacyState", "SCHEMA", "decode_master_v2", "discover", "legacy_section_hashes",
           "read_master_v2_section", "write_master_v2"]


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rolling-root", type=Path, required=True)
    parser.add_argument("--canonical-root", type=Path, required=True)
    parser.add_argument("--campaign-root", type=Path, required=True)
    parser.add_argument("--run-id", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    state = discover(args.rolling_root, args.canonical_root, args.campaign_root, args.run_id)
    result = write_master_v2(state, args.output)
    decoded = decode_master_v2(args.output)
    expected = legacy_section_hashes(
        state, _meta_payload(state, _file_manifest(state), result["generation_id"]))
    actual = {item["name"]: {key: item[key] for key in ("sha256", "bytes")}
              for item in decoded["sections"]}
    if expected != actual:
        raise SystemExit("STOP_MASTER_V2_SEMANTIC_SECTION_MISMATCH")
    result.update({"semantic_equivalence": "PASS", "decoded_overall_sha256": decoded["overall_sha256"],
                   "section_hashes_match_legacy": True,
                   "ratio_v2_to_legacy": result["bytes"] / result["legacy_persistent_bytes"],
                   "section_sizes_mib": {item["name"]: item["bytes"] / 1048576
                                         for item in result["sections"]}})
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))
