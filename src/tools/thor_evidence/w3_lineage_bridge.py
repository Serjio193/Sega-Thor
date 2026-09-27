"""Reconstruct and validate W3 wave windows from the deterministic producer contract."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct
from typing import Any, Iterable


RECORD = struct.Struct("<QQQIIIHBBHHI")
SCHEMA = "oasis.m14.w3-lineage-index.v1"
CONTRACT = "live-forward-scaling-round-worker-order-v1"


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        while chunk := source.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def _canonical(value: Any) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


def _audit_sha(path: Path) -> str:
    return _sha(path)


def build_wave_index(wave_path: Path, audit_rows: Iterable[dict[str, Any]], *,
                     run_id: int, wave_index: int, worker_count: int,
                     audit_sha256: str) -> dict[str, Any]:
    """Bind a whole W3 wave to one audited segment per worker, in producer order."""
    wave_path = Path(wave_path)
    if run_id <= 0 or wave_index <= 0 or worker_count <= 0:
        raise ValueError("STOP_W3_LINEAGE_INVALID_RUN_OR_WAVE_ID")
    if len(audit_sha256) != 64:
        raise ValueError("STOP_W3_LINEAGE_AUDIT_HASH_INVALID")
    size = wave_path.stat().st_size
    if size == 0 or size % RECORD.size:
        raise ValueError("STOP_W3_LINEAGE_RAW_SIZE_INVALID")
    by_worker: dict[int, dict[str, Any]] = {}
    capture_ids: set[int] = set()
    for row in audit_rows:
        if row.get("run_id") != run_id or row.get("cycle") != wave_index:
            continue
        worker = row.get("worker_id")
        if not isinstance(worker, int) or worker in by_worker:
            raise ValueError("STOP_W3_LINEAGE_DUPLICATE_OR_INVALID_WORKER")
        required = ("epoch", "capture_id", "generation", "entry_stream_sequence",
                    "exit_stream_sequence", "entry_instruction_sequence",
                    "exit_instruction_sequence", "record_count", "configured_depth")
        if row.get("valid") is not True or any(type(row.get(key)) is not int or
                row[key] < 0 for key in required):
            raise ValueError("STOP_W3_LINEAGE_SEGMENT_IDENTITY_INVALID")
        if row["capture_id"] <= 0 or row["generation"] <= 0 or row["epoch"] <= 0 or \
                row["capture_id"] in capture_ids:
            raise ValueError("STOP_W3_LINEAGE_DUPLICATE_CAPTURE_IDENTITY")
        for key in ("records_sha256", "segment_sha256"):
            value = row.get(key)
            if not isinstance(value, str) or len(value) != 64 or any(
                    char not in "0123456789abcdef" for char in value):
                raise ValueError("STOP_W3_LINEAGE_SEGMENT_HASH_INVALID")
        capture_ids.add(row["capture_id"])
        by_worker[worker] = row
    if set(by_worker) != set(range(worker_count)):
        raise ValueError("STOP_W3_LINEAGE_WORKER_COVERAGE_MISMATCH")

    segments: list[dict[str, Any]] = []
    offset = 0
    with wave_path.open("rb") as source:
        for worker in range(worker_count):
            row = by_worker[worker]
            count = row.get("record_count")
            expected_sha = row.get("records_sha256")
            if not isinstance(count, int) or count <= 0 or not isinstance(expected_sha, str):
                raise ValueError("STOP_W3_LINEAGE_SEGMENT_METADATA_INVALID")
            length = count * RECORD.size
            source.seek(offset)
            digest = hashlib.sha256()
            remaining = length
            first = last = None
            while remaining:
                chunk = source.read(min(1024 * 1024, remaining))
                if not chunk:
                    raise ValueError("STOP_W3_LINEAGE_WINDOW_TRUNCATED")
                digest.update(chunk)
                if first is None:
                    first = RECORD.unpack_from(chunk)
                last = RECORD.unpack_from(chunk, len(chunk) - RECORD.size)
                remaining -= len(chunk)
            if digest.hexdigest() != expected_sha:
                raise ValueError(f"STOP_W3_LINEAGE_SEGMENT_HASH_MISMATCH:{worker}")
            if first[0] != row["entry_stream_sequence"] or \
                    last[0] + 1 != row["exit_stream_sequence"] or \
                    first[1] != row["entry_instruction_sequence"] or \
                    last[1] + 1 != row["exit_instruction_sequence"]:
                raise ValueError(f"STOP_W3_LINEAGE_SEQUENCE_BOUNDS_MISMATCH:{worker}")
            segment = dict(row)
            segment.update({"ready_for_cartographer": True,
                            "raw_offset": offset, "raw_length": length})
            segments.append(segment)
            offset += length
    if offset != size:
        raise ValueError("STOP_W3_LINEAGE_RAW_COVERAGE_MISMATCH")
    return {"schema": SCHEMA, "producer_contract": CONTRACT,
            "run_id": run_id, "wave_index": wave_index,
            "worker_count": worker_count, "raw_file": wave_path.name,
            "raw_bytes": size, "raw_sha256": _sha(wave_path),
            "record_count": size // RECORD.size,
            "segment_audit_sha256": audit_sha256, "segments": segments}


def write_wave_index(wave_path: Path, audit_rows: Iterable[dict[str, Any]], *,
                     run_id: int, wave_index: int, worker_count: int,
                     audit_sha256: str, output_path: Path) -> dict[str, Any]:
    rows = list(audit_rows)
    index = build_wave_index(wave_path, rows, run_id=run_id,
        wave_index=wave_index, worker_count=worker_count,
        audit_sha256=audit_sha256)
    encoded = _canonical(index)
    output_path = Path(output_path)
    output_path.write_bytes(encoded)
    return {"path": str(output_path.resolve()), "bytes": len(encoded),
            "sha256": hashlib.sha256(encoded).hexdigest(), **index}


def validate_wave_index(wave_path: Path, index_path: Path, *,
                        expected_index_sha256: str,
                        expected_audit_sha256: str) -> dict[str, Any]:
    wave_path, index_path = Path(wave_path), Path(index_path)
    if _sha(index_path) != expected_index_sha256:
        raise ValueError("STOP_W3_LINEAGE_INDEX_HASH_MISMATCH")
    index = json.loads(index_path.read_text(encoding="utf-8"))
    if index.get("schema") != SCHEMA or index.get("producer_contract") != CONTRACT:
        raise ValueError("STOP_W3_LINEAGE_INDEX_SCHEMA_UNSUPPORTED")
    if _sha(wave_path) != index.get("raw_sha256") or \
            wave_path.stat().st_size != index.get("raw_bytes"):
        raise ValueError("STOP_W3_LINEAGE_RAW_HASH_MISMATCH")
    if index_path.name and index.get("raw_file") != wave_path.name:
        raise ValueError("STOP_W3_LINEAGE_RAW_NAME_MISMATCH")
    if index.get("segment_audit_sha256") != expected_audit_sha256:
        raise ValueError("STOP_W3_LINEAGE_AUDIT_BINDING_MISMATCH")
    offset = 0
    with wave_path.open("rb") as source:
        for segment in index.get("segments", []):
            if segment.get("raw_offset") != offset or \
                    segment.get("raw_length") != segment.get("record_count", 0) * RECORD.size:
                raise ValueError("STOP_W3_LINEAGE_INDEX_SPAN_INVALID")
            source.seek(offset)
            data = source.read(segment["raw_length"])
            if len(data) != segment["raw_length"] or \
                    hashlib.sha256(data).hexdigest() != segment.get("records_sha256"):
                raise ValueError("STOP_W3_LINEAGE_INDEX_SEGMENT_HASH_MISMATCH")
            offset += len(data)
    if offset != wave_path.stat().st_size or \
            index.get("record_count") != offset // RECORD.size or \
            len(index.get("segments", [])) != index.get("worker_count"):
        raise ValueError("STOP_W3_LINEAGE_INDEX_COVERAGE_MISMATCH")
    return {"status": "PASS_W3_LINEAGE_INDEX", "raw_sha256": index["raw_sha256"],
            "index_sha256": _sha(index_path), "record_count": index["record_count"],
            "segments": len(index["segments"]), "unmapped_records": 0}


def write_event_window_index(index_path: Path, output_path: Path, *,
                             expected_index_sha256: str) -> str:
    """Adapt a verified bridge sidecar to the envelope reader's JSONL index."""
    index_path, output_path = Path(index_path), Path(output_path)
    if _sha(index_path) != expected_index_sha256:
        raise ValueError("STOP_W3_LINEAGE_INDEX_HASH_MISMATCH")
    index = json.loads(index_path.read_text(encoding="utf-8"))
    if index.get("schema") != SCHEMA:
        raise ValueError("STOP_W3_LINEAGE_INDEX_SCHEMA_UNSUPPORTED")
    cursor, encoded = 0, []
    for segment in index.get("segments", []):
        if segment.get("raw_offset") != cursor or \
                segment.get("raw_length") != segment.get("record_count", 0) * RECORD.size:
            raise ValueError("STOP_W3_LINEAGE_INDEX_SPAN_INVALID")
        value = {"raw_offset": cursor, "raw_length": segment["raw_length"],
                 "segment": segment}
        encoded.append(_canonical(value).decode("utf-8"))
        cursor += segment["raw_length"]
    if cursor != index.get("raw_bytes"):
        raise ValueError("STOP_W3_LINEAGE_INDEX_COVERAGE_MISMATCH")
    with output_path.open("x", encoding="utf-8", newline="\n") as output:
        output.writelines(encoded)
    return _sha(output_path)


def admit_wave(session: Any, wave_path: Path, index_path: Path, *,
               expected_index_sha256: str,
               expected_audit_sha256: str) -> list[dict[str, Any]]:
    """Feed only fully verified windows into the existing RAM Cartographer."""
    validate_wave_index(wave_path, index_path,
        expected_index_sha256=expected_index_sha256,
        expected_audit_sha256=expected_audit_sha256)
    index = json.loads(Path(index_path).read_text(encoding="utf-8"))
    raw_sha, index_sha = index["raw_sha256"], _sha(Path(index_path))
    results = []
    with Path(wave_path).open("rb") as source:
        for segment in index["segments"]:
            source.seek(segment["raw_offset"])
            data = source.read(segment["raw_length"])
            ready = dict(segment)
            ready.update({"source_raw_sha256": raw_sha,
                          "source_index_sha256": index_sha})
            rows = list(RECORD.iter_unpack(data))
            results.append(session.admit(ready, rows, data))
    return results


def load_audit(path: Path, expected_sha256: str) -> list[dict[str, Any]]:
    path = Path(path)
    if _audit_sha(path) != expected_sha256:
        raise ValueError("STOP_W3_LINEAGE_AUDIT_HASH_MISMATCH")
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()]


def publish_run_indexes(output_dir: Path, audit_path: Path, *, run_id: int,
                        worker_count: int, audit_sha256: str,
                        chunk_prefix: str) -> list[dict[str, Any]]:
    """Create cryptographically bound sidecars after the producer has closed."""
    output_dir, audit_path = Path(output_dir), Path(audit_path)
    rows = load_audit(audit_path, audit_sha256)
    pattern = re.compile(rf"{re.escape(chunk_prefix)}-(\d+)\.bin\Z")
    waves = []
    for path in output_dir.glob(f"{chunk_prefix}-*.bin"):
        match = pattern.fullmatch(path.name)
        if not match:
            raise ValueError("STOP_W3_LINEAGE_WAVE_FILENAME_INVALID")
        waves.append((int(match.group(1)), path))
    waves.sort()
    if not waves or [index for index, _ in waves] != list(range(1, len(waves) + 1)):
        raise ValueError("STOP_W3_LINEAGE_WAVE_SEQUENCE_GAP")
    result = []
    for index, path in waves:
        sidecar = path.with_suffix(path.suffix + ".lineage.json")
        if sidecar.exists():
            raise FileExistsError(f"refusing to overwrite W3 sidecar: {sidecar}")
        published = write_wave_index(path, rows, run_id=run_id,
            wave_index=index, worker_count=worker_count,
            audit_sha256=audit_sha256, output_path=sidecar)
        result.append({"path": published["path"], "bytes": published["bytes"],
            "sha256": published["sha256"], "raw_sha256": published["raw_sha256"],
            "record_count": published["record_count"],
            "segment_count": len(published["segments"])})
    return result
