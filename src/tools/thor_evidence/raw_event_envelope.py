"""Lossless gzip JSONL envelope for fixed-width FLOW_V1 and W3 V2 records."""

from __future__ import annotations

import gzip
import hashlib
import io
import json
import os
import argparse
from pathlib import Path
import re
import sqlite3
import struct
import tempfile
from contextlib import closing
from typing import Any, Iterator

try:
    from .raw_elimination import canonical, event_accounting, file_sha256
except ImportError:
    from raw_elimination import canonical, event_accounting, file_sha256


FORMAT = struct.Struct("<QQQIIIHBBHHI")
FIELDS = ("stream_sequence", "instruction_sequence", "master_time", "pc",
          "address", "value", "kind_flags", "cpu_id", "length_or_width",
          "domain", "reserved", "auxiliary")
KNOWN_FLAGS = 0xB7FF
FAULTED = 0x0004
EVENT = 0x8000
EVENT_SUBTYPE_MASK = 0x3800


def _spans(raw_path: Path, index_path: Path | None) -> Iterator[tuple[int, int, dict[str, Any]]]:
    size = raw_path.stat().st_size
    if size % FORMAT.size:
        raise ValueError("STOP_RAW_RECORD_STREAM_TRUNCATED")
    if index_path is None:
        yield 0, size, {"source": raw_path.name, "window_id": file_sha256(raw_path)}
        return
    cursor = 0
    with index_path.open("r", encoding="utf-8") as index:
        for line_no, line in enumerate(index, 1):
            item = json.loads(line)
            start, length = int(item["raw_offset"]), int(item["raw_length"])
            segment = item.get("segment", {})
            if start != cursor or length <= 0 or length % FORMAT.size:
                raise ValueError(f"STOP_RAW_INDEX_SPAN_INVALID:{line_no}")
            if not isinstance(segment, dict):
                raise ValueError(f"STOP_RAW_INDEX_SEGMENT_INVALID:{line_no}")
            yield start, length, {"source": raw_path.name,
                "raw_offset": start, "raw_length": length, "segment": segment}
            cursor += length
    if cursor != size:
        raise ValueError("STOP_RAW_INDEX_COVERAGE_MISMATCH")


def _segment_sha(raw_path: Path, start: int, length: int) -> str:
    digest = hashlib.sha256()
    with raw_path.open("rb") as source:
        source.seek(start)
        remaining = length
        while remaining:
            chunk = source.read(min(1024 * 1024, remaining))
            if not chunk:
                raise ValueError("STOP_RAW_SEGMENT_TRUNCATED")
            digest.update(chunk)
            remaining -= len(chunk)
    return digest.hexdigest()


def _classification(row: tuple[int, ...]) -> tuple[str, str | None]:
    flags = row[6]
    if flags & FAULTED:
        return "REJECTED", "FAULTED_RECORD"
    # EVENT_SUBTYPE bits are payload when FLAG_EVENT is set, including the
    # BUS_READ bit at 0x0800. They are not independent unknown flags.
    unknown_flags = flags & ~(KNOWN_FLAGS | (EVENT_SUBTYPE_MASK if flags & EVENT else 0))
    if flags == 0 or unknown_flags:
        return "UNRESOLVED", "UNKNOWN_OR_UNTYPED_FLAGS"
    if row[7] not in (0, 1, 255):
        return "UNRESOLVED", "UNKNOWN_CPU_ID"
    if flags & EVENT and ((flags & EVENT_SUBTYPE_MASK) >> 11) not in (1, 2, 3, 4):
        return "UNRESOLVED", "UNKNOWN_EVENT_SUBTYPE"
    return "ACCEPTED", None


def normalize_records(raw_path: Path, output_path: Path, format_name: str,
                      index_path: Path | None = None) -> dict[str, Any]:
    """Publish a complete envelope atomically and never overwrite a prior one."""
    output_path = Path(output_path)
    if output_path.exists():
        raise FileExistsError("refusing to overwrite normalized evidence")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, staging_name = tempfile.mkstemp(
        prefix=output_path.name + ".", suffix=".partial", dir=output_path.parent)
    os.close(descriptor)
    staging_path = Path(staging_name)
    staging_path.unlink()
    try:
        receipt = _normalize_records(raw_path, staging_path, format_name, index_path)
        staging_path.replace(output_path)
    except Exception:
        staging_path.unlink(missing_ok=True)
        raise
    receipt["normalized_artifact"]["name"] = output_path.name
    return receipt


def _normalize_records(raw_path: Path, output_path: Path, format_name: str,
                       index_path: Path | None = None) -> dict[str, Any]:
    """Normalize every record and retain all 12 fields, locator and disposition."""
    if format_name not in {"FLOW_V1", "W3_V2"}:
        raise ValueError("STOP_RAW_FORMAT_UNSUPPORTED")
    raw_path, output_path = Path(raw_path), Path(output_path)
    raw_bytes = raw_path.stat().st_size
    raw_sha = file_sha256(raw_path)
    if raw_bytes == 0 or raw_bytes % FORMAT.size:
        raise ValueError("STOP_RAW_RECORD_STREAM_TRUNCATED")
    counts = {"input_events": 0, "accepted_events": 0, "unresolved_events": 0,
              "rejected_events": 0, "duplicate_events": 0}
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="raw-envelope-") as temp_dir:
        with closing(sqlite3.connect(Path(temp_dir) / "identities.sqlite")) as identities:
            identities.execute("CREATE TABLE seen(identity TEXT PRIMARY KEY,payload TEXT NOT NULL)")
            with raw_path.open("rb") as source, output_path.open("wb") as binary:
                with gzip.GzipFile(fileobj=binary, mode="wb", mtime=0) as compressed:
                    with io.TextIOWrapper(compressed, encoding="utf-8", newline="\n") as text:
                        for start, length, window in _spans(raw_path, index_path):
                            segment = window.get("segment", {})
                            expected_hash = segment.get("records_sha256")
                            if expected_hash and _segment_sha(raw_path, start, length) != expected_hash:
                                raise ValueError("STOP_RAW_INDEX_SEGMENT_HASH_MISMATCH")
                            source.seek(start)
                            for local_offset in range(0, length, FORMAT.size):
                                blob = source.read(FORMAT.size)
                                if len(blob) != FORMAT.size:
                                    raise ValueError("STOP_RAW_RECORD_STREAM_TRUNCATED")
                                row = FORMAT.unpack(blob)
                                state, reason = _classification(row)
                                absolute = start + local_offset
                                identity_scope = (segment.get("run_id"), segment.get("epoch"))
                                if identity_scope[0] is not None and identity_scope[1] is not None:
                                    event_identity = canonical([identity_scope, row[7], row[0], row[1], row[6]])
                                    payload_identity = hashlib.sha256(blob).hexdigest()
                                    previous = identities.execute(
                                        "SELECT payload FROM seen WHERE identity=?",
                                        (event_identity,)).fetchone()
                                    if previous and previous[0] == payload_identity:
                                        state, reason = "DUPLICATE", "OVERLAPPING_NATIVE_OCCURRENCE"
                                    elif previous:
                                        state, reason = "UNRESOLVED", "NATIVE_IDENTITY_PAYLOAD_CONFLICT"
                                    else:
                                        identities.execute("INSERT INTO seen VALUES (?,?)",
                                                           (event_identity, payload_identity))
                                else:
                                    event_identity = canonical([raw_sha, absolute])
                                item = {"schema": "oasis.runtime-event-envelope.v1",
                                    "format": format_name, "event_id": hashlib.sha256(
                                        event_identity).hexdigest(),
                                    "source_sha256": raw_sha, "source_offset": absolute,
                                    "window": window, "disposition": state,
                                    "reason": reason, "fields": dict(zip(FIELDS, row)),
                                    "record_hex": blob.hex()}
                                text.write(canonical(item).decode("utf-8") + "\n")
                                counts["input_events"] += 1
                                key = {"ACCEPTED": "accepted_events", "UNRESOLVED": "unresolved_events",
                                       "REJECTED": "rejected_events", "DUPLICATE": "duplicate_events"}[state]
                                counts[key] += 1
            identities.commit()
    accounting = event_accounting(counts)
    if accounting["unaccounted_events"] != 0:
        raise ValueError("STOP_RAW_EVENT_ACCOUNTING_MISMATCH")
    module_hash = file_sha256(Path(__file__))
    return {"schema": "oasis.raw-elimination.envelope.v1", "format": format_name,
        "raw_file": {"name": raw_path.name, "bytes": raw_bytes, "sha256": raw_sha},
        "index_sha256": file_sha256(index_path) if index_path else None,
        "normalized_artifact": {"name": output_path.name,
            "bytes": output_path.stat().st_size, "sha256": file_sha256(output_path)},
        "normalizer": {"version": "m14.7b-record-envelope-v1", "sha256": module_hash},
        "event_counts": accounting, "all_fields_known_to_format_accounted": True,
        "record_fields": list(FIELDS), "lossless_record_hex_retained": True}


def replay_envelope(path: Path, expected_raw_sha256: str | None = None) -> dict[str, Any]:
    """Rebuild raw bytes and independently recheck identities/accounting."""
    raw_hash = hashlib.sha256()
    event_hash = hashlib.sha256()
    source_sha: str | None = None
    expected_offset = 0
    counts = {"input_events": 0, "accepted_events": 0, "unresolved_events": 0,
              "rejected_events": 0, "duplicate_events": 0}
    with tempfile.TemporaryDirectory(prefix="raw-replay-") as temp_dir:
        with closing(sqlite3.connect(Path(temp_dir) / "identities.sqlite")) as identities:
            identities.execute("CREATE TABLE seen(identity TEXT PRIMARY KEY,payload TEXT NOT NULL)")
            with gzip.open(path, "rt", encoding="utf-8") as source:
                for line in source:
                    item = json.loads(line)
                    if item.get("schema") != "oasis.runtime-event-envelope.v1" or \
                            item.get("record_hex") is None:
                        raise ValueError("STOP_NORMALIZED_EVENT_INVALID")
                    blob = bytes.fromhex(item["record_hex"])
                    row = FORMAT.unpack(blob)
                    if dict(zip(FIELDS, row)) != item.get("fields"):
                        raise ValueError("STOP_NORMALIZED_EVENT_FIELD_MISMATCH")
                    if item.get("source_offset") != expected_offset or \
                            (source_sha is not None and item.get("source_sha256") != source_sha):
                        raise ValueError("STOP_NORMALIZED_EVENT_LOCATOR_MISMATCH")
                    source_sha = item["source_sha256"]
                    segment = item.get("window", {}).get("segment", {})
                    run_id, epoch = segment.get("run_id"), segment.get("epoch")
                    expected_state, expected_reason = _classification(row)
                    if run_id is not None and epoch is not None:
                        identity = canonical([[run_id, epoch], row[7], row[0], row[1], row[6]])
                        payload = hashlib.sha256(blob).hexdigest()
                        prior = identities.execute("SELECT payload FROM seen WHERE identity=?",
                                                   (identity,)).fetchone()
                        if prior and prior[0] == payload:
                            expected_state, expected_reason = "DUPLICATE", "OVERLAPPING_NATIVE_OCCURRENCE"
                        elif prior:
                            expected_state, expected_reason = "UNRESOLVED", "NATIVE_IDENTITY_PAYLOAD_CONFLICT"
                        else:
                            identities.execute("INSERT INTO seen VALUES (?,?)", (identity, payload))
                    else:
                        identity = canonical([item["source_sha256"], expected_offset])
                    event_id = hashlib.sha256(identity).hexdigest()
                    if item.get("event_id") != event_id or \
                            item.get("disposition") != expected_state or \
                            item.get("reason") != expected_reason:
                        raise ValueError("STOP_NORMALIZED_EVENT_IDENTITY_OR_CLASSIFICATION_MISMATCH")
                    raw_hash.update(blob)
                    event_hash.update(canonical([event_id, expected_state, row]) + b"\n")
                    expected_offset += FORMAT.size
                    counts["input_events"] += 1
                    category = {"ACCEPTED": "accepted_events", "UNRESOLVED": "unresolved_events",
                                "REJECTED": "rejected_events", "DUPLICATE": "duplicate_events"}[
                                    expected_state]
                    counts[category] += 1
            identities.commit()
    accounting = event_accounting(counts)
    if accounting["input_events"] == 0 or source_sha is None:
        raise ValueError("STOP_NORMALIZED_EVENT_STREAM_EMPTY")
    actual_raw_sha = raw_hash.hexdigest()
    if expected_raw_sha256 and actual_raw_sha != expected_raw_sha256:
        raise ValueError("STOP_RAW_FREE_REPLAY_RAW_HASH_MISMATCH")
    return {"status": "PASS_RAW_FREE_ENVELOPE_REPLAY", "raw_sha256": actual_raw_sha,
        "event_stream_sha256": event_hash.hexdigest(), "event_counts": accounting,
        "accounted_events": accounting["accounted_events"],
        "unaccounted_events": accounting["unaccounted_events"]}


def normalize_capsule(raw_path: Path, output_path: Path) -> dict[str, Any]:
    """Retain AUTO67 O67C/O67V headers, lease identity and every capsule row."""
    try:
        from .auto67_capsule_codec import decode_capsule
    except ImportError:
        from auto67_capsule_codec import decode_capsule
    raw_path, output_path = Path(raw_path), Path(output_path)
    if output_path.exists():
        raise FileExistsError("refusing to overwrite normalized evidence")
    decoded = decode_capsule(raw_path)
    raw = raw_path.read_bytes()
    header_bytes = 20 if decoded.format_version == 1 else 24
    record_bytes = 16 if decoded.format_version == 1 else 20
    raw_sha = hashlib.sha256(raw).hexdigest()
    version_name = "O67C_V1" if decoded.format_version == 1 else "O67V_V2"
    header = {"schema": "oasis.runtime-event-envelope.v1", "record_type": "HEADER",
        "format": version_name, "source_sha256": raw_sha, "source_name": raw_path.name,
        "header_hex": raw[:header_bytes].hex(), "capsule_id": decoded.capsule_id,
        "lease_id": decoded.lease_id, "start_frame": decoded.start_frame,
        "logical_bytes_used": decoded.logical_bytes_used,
        "event_count": decoded.event_count, "record_bytes": record_bytes}
    counts = {"input_events": decoded.event_count, "accepted_events": 0,
        "unresolved_events": 0, "rejected_events": 0, "duplicate_events": 0}
    output_path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=output_path.name + ".",
        suffix=".partial", dir=output_path.parent)
    os.close(descriptor)
    temporary_path = Path(temporary_name)
    try:
        with temporary_path.open("wb") as binary:
            with gzip.GzipFile(fileobj=binary, mode="wb", mtime=0) as compressed:
                with io.TextIOWrapper(compressed, encoding="utf-8", newline="\n") as text:
                    text.write(canonical(header).decode("utf-8") + "\n")
                    for index, observation in enumerate(decoded.observations):
                        offset = header_bytes + index * record_bytes
                        blob = raw[offset:offset + record_bytes]
                        state = "ACCEPTED" if observation.kind is not None else "UNRESOLVED"
                        reason = None if observation.kind is not None else "CAPSULE_KIND_UNKNOWN"
                        item = {"schema": "oasis.runtime-event-envelope.v1",
                            "record_type": "EVENT", "format": version_name,
                            "event_id": hashlib.sha256(canonical(
                                [raw_sha, index, observation.sequence, observation.frame])).hexdigest(),
                            "source_sha256": raw_sha, "source_offset": offset,
                            "disposition": state, "reason": reason,
                            "fields": {"sequence": observation.sequence,
                                "frame": observation.frame, "address": observation.address,
                                "pc": observation.pc, "kind": observation.kind,
                                "kind_code": observation.kind_code},
                            "record_hex": blob.hex()}
                        text.write(canonical(item).decode("utf-8") + "\n")
                        counts["accepted_events" if state == "ACCEPTED" else "unresolved_events"] += 1
        temporary_path.replace(output_path)
    except Exception:
        temporary_path.unlink(missing_ok=True)
        raise
    accounting = event_accounting(counts)
    if accounting["unaccounted_events"] != 0:
        raise ValueError("STOP_CAPSULE_EVENT_ACCOUNTING_MISMATCH")
    return {"schema": "oasis.raw-elimination.envelope.v1", "format": version_name,
        "raw_file": {"name": raw_path.name, "bytes": len(raw), "sha256": raw_sha},
        "normalized_artifact": {"name": output_path.name,
            "bytes": output_path.stat().st_size, "sha256": file_sha256(output_path)},
        "event_counts": accounting, "all_fields_known_to_format_accounted": True,
        "lease_identity_retained": True, "frame_identity_retained": True}


def replay_capsule_envelope(path: Path, expected_raw_sha256: str | None = None) -> dict[str, Any]:
    """Rebuild the exact capsule file from its retained header and record bytes."""
    digest = hashlib.sha256()
    event_hash = hashlib.sha256()
    counts = {"input_events": 0, "accepted_events": 0, "unresolved_events": 0,
              "rejected_events": 0, "duplicate_events": 0}
    with gzip.open(path, "rt", encoding="utf-8") as source:
        header = json.loads(next(source))
        if header.get("record_type") != "HEADER" or header.get("format") not in {
                "O67C_V1", "O67V_V2"}:
            raise ValueError("STOP_CAPSULE_ENVELOPE_HEADER_INVALID")
        header_bytes = bytes.fromhex(header["header_hex"])
        if header["format"] == "O67C_V1":
            if len(header_bytes) != 20 or header_bytes[:4] != b"O67C":
                raise ValueError("STOP_CAPSULE_ENVELOPE_HEADER_INVALID")
            capsule_id, start_frame, logical_bytes, declared_count = struct.unpack_from(
                "<IIII", header_bytes, 4)
            record_size = 16
        else:
            if len(header_bytes) != 24 or header_bytes[:4] != b"O67V":
                raise ValueError("STOP_CAPSULE_ENVELOPE_HEADER_INVALID")
            version, capsule_id, start_frame, logical_bytes, declared_count = struct.unpack_from(
                "<IIIII", header_bytes, 4)
            if version != 2:
                raise ValueError("STOP_CAPSULE_ENVELOPE_HEADER_INVALID")
            record_size = 20
        name_match = re.fullmatch(r"capsule-(\d{2})-([A-Za-z0-9_-]+)\.bin",
                                  str(header.get("source_name", "")))
        if not name_match or int(name_match.group(1)) != capsule_id or \
                name_match.group(2) != header.get("lease_id") or \
                header.get("capsule_id") != capsule_id or \
                header.get("start_frame") != start_frame or \
                header.get("logical_bytes_used") != logical_bytes or \
                header.get("event_count") != declared_count or \
                header.get("record_bytes") != record_size:
            raise ValueError("STOP_CAPSULE_ENVELOPE_HEADER_IDENTITY_MISMATCH")
        digest.update(header_bytes)
        kind_names = {1: "BUS_WRITE_PC", 2: "BUS_EXEC_PC"}
        index = 0
        for line in source:
            item = json.loads(line)
            if item.get("record_type") != "EVENT" or \
                    item.get("source_sha256") != header.get("source_sha256"):
                raise ValueError("STOP_CAPSULE_ENVELOPE_EVENT_INVALID")
            blob = bytes.fromhex(item["record_hex"])
            if len(blob) != record_size or item.get("source_offset") != \
                    len(header_bytes) + index * record_size:
                raise ValueError("STOP_CAPSULE_ENVELOPE_RECORD_SIZE_INVALID")
            if header["format"] == "O67V_V2":
                sequence, frame, address, pc, kind_code = struct.unpack("<IIIII", blob)
                kind = kind_names.get(kind_code)
            else:
                sequence, frame, address, pc = struct.unpack("<IIII", blob)
                kind, kind_code = None, None
            fields = {"sequence": sequence, "frame": frame, "address": address,
                      "pc": pc, "kind": kind, "kind_code": kind_code}
            expected_state = "ACCEPTED" if kind is not None else "UNRESOLVED"
            expected_reason = None if kind is not None else "CAPSULE_KIND_UNKNOWN"
            event_id = hashlib.sha256(canonical(
                [header["source_sha256"], index, sequence, frame])).hexdigest()
            if item.get("fields") != fields or item.get("event_id") != event_id or \
                    item.get("disposition") != expected_state or \
                    item.get("reason") != expected_reason:
                raise ValueError("STOP_CAPSULE_ENVELOPE_EVENT_SEMANTICS_MISMATCH")
            digest.update(blob)
            event_hash.update(canonical([event_id, expected_state, fields]) + b"\n")
            counts["input_events"] += 1
            key = {"ACCEPTED": "accepted_events", "UNRESOLVED": "unresolved_events",
                   "REJECTED": "rejected_events", "DUPLICATE": "duplicate_events"}.get(
                       item.get("disposition"))
            if key is None:
                raise ValueError("STOP_CAPSULE_ENVELOPE_DISPOSITION_INVALID")
            counts[key] += 1
            index += 1
        if index != declared_count or logical_bytes != 64 + index * record_size:
            raise ValueError("STOP_CAPSULE_ENVELOPE_ACCOUNTING_MISMATCH")
    accounting = event_accounting(counts)
    actual = digest.hexdigest()
    if expected_raw_sha256 and actual != expected_raw_sha256:
        raise ValueError("STOP_CAPSULE_RAW_FREE_REPLAY_HASH_MISMATCH")
    return {"status": "PASS_RAW_FREE_CAPSULE_REPLAY", "raw_sha256": actual,
        "event_stream_sha256": event_hash.hexdigest(), "event_counts": accounting,
        "unaccounted_events": accounting["unaccounted_events"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--normalize", type=Path,
                      help="raw fixed-width record file to normalize")
    mode.add_argument("--normalize-capsule", type=Path,
                      help="AUTO67 O67C/O67V capsule to normalize")
    mode.add_argument("--replay", type=Path,
                      help="normalized envelope to replay without raw input")
    mode.add_argument("--replay-capsule", type=Path,
                      help="normalized AUTO67 capsule envelope to replay")
    parser.add_argument("--format", choices=("FLOW_V1", "W3_V2"))
    parser.add_argument("--index", type=Path)
    parser.add_argument("--expected-raw-sha256")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--receipt", type=Path,
                        help="normalization receipt output (required with --normalize)")
    args = parser.parse_args()
    if args.normalize_capsule:
        if args.receipt is None:
            parser.error("--normalize-capsule requires --receipt")
        if args.receipt.exists():
            raise FileExistsError("refusing to overwrite normalization receipt")
        result = normalize_capsule(args.normalize_capsule, args.output)
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    elif args.normalize:
        if args.format is None or args.receipt is None:
            parser.error("--normalize requires --format and --receipt")
        if args.receipt.exists():
            raise FileExistsError("refusing to overwrite normalization receipt")
        result = normalize_records(args.normalize, args.output, args.format, args.index)
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    else:
        if args.output.exists():
            raise FileExistsError("refusing to overwrite replay receipt")
        result = replay_capsule_envelope(args.replay_capsule, args.expected_raw_sha256) \
            if args.replay_capsule else replay_envelope(args.replay, args.expected_raw_sha256)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
