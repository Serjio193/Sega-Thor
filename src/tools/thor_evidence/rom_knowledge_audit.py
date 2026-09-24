"""Independent, read-only verifier for the canonical ROM knowledge map."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from pathlib import Path, PurePosixPath
from typing import Any

try:
    from .identity import ROM_SHA, ROM_SIZE
    from .rom_knowledge_audit_records import audit_record_ids
except ImportError:
    from identity import ROM_SHA, ROM_SIZE
    from rom_knowledge_audit_records import audit_record_ids


UNKNOWN = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}
UNCONFIRMED = {"PROBABLE", "CANDIDATE", "UNVERIFIED"}
GRAPHICS = {"GRAPHICS_COMPRESSED_STREAM", "CONSUMER_BACKED_GRAPHICS_STREAM",
    "TABLE_SELECTED_GRAPHICS_STREAM", "DIRECT_GRAPHICS_STREAM", "GRAPHICS_DIRECT_LOADER_STREAM",
    "DIRECT_MENU_GRAPHICS_STREAM", "RUNTIME_CORRELATED_GRAPHICS_STREAM", "EXACT_3820_GRAPHICS_STREAM"}
POINTERS = {"ABSOLUTE_STATE_DISPATCH_POINTER_TABLE", "STATIC_DISPATCH_POINTER_TABLE",
    "STATIC_MULTI_DISPATCH_POINTER_TABLE", "SCREEN_GROUP_POINTER_TABLE", "GROUP_POINTER_TABLE_32X32",
    "COMPRESSED_RESOURCE_POINTER_TABLE", "CCB0_RELATIVE_TARGET_TABLE_256X16",
    "SIGNED_RELATIVE_EVENT_DISPATCH_TABLE"}


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _hash_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _id(prefix: str, value: Any) -> str:
    return prefix + ":" + _hash_bytes(_canonical(value).encode("utf-8"))


def _object_type(entry: dict[str, Any]) -> str:
    kind = str(entry.get("kind", "UNKNOWN"))
    label = str(entry.get("classification", kind))
    if kind in UNKNOWN:
        return "UNKNOWN"
    if label in GRAPHICS:
        return "GRAPHICS_STREAM"
    if label == "SOUND_DATA_CONTAINER_CONFIRMED":
        return "AUDIO_DATA"
    if label == "Z80_ASM_SOURCE_OWNED":
        return "Z80_PROGRAM"
    if label in POINTERS:
        return "POINTER_TABLE"
    if kind in {"STRUCTURED_DATA_CONFIRMED", "DATA_KNOWN", "PADDING_ALIGNMENT_CONFIRMED", "HEADER_VECTOR_ASM"}:
        return "ROM_DATA"
    return "ROM_RANGE"


def _emission_type(entry: dict[str, Any]) -> str:
    artifact, kind = str(entry.get("emitted_artifact_type", "")), str(entry.get("kind", "UNKNOWN"))
    if artifact == "asm":
        return "ASM"
    if artifact == "blob":
        return "INCBIN"
    if artifact == "rom_asset" and kind == "LOCAL_ROM_DERIVED_ASSET":
        return "ASSET"
    if artifact == "rom_asset" and kind in {"STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED"}:
        return "DATA"
    raise ValueError("STOP_EMISSION_TYPE_UNSUPPORTED")


def _owned(entry: dict[str, Any]) -> bool:
    return entry.get("kind") not in UNKNOWN and str(entry.get("confidence", "")).upper() not in UNCONFIRMED


def _owned_intervals(entries: list[dict[str, Any]]) -> list[tuple[int, int]]:
    output = []
    for entry in entries:
        if _owned(entry):
            interval = (int(entry["start"]), int(entry["end"]))
            if output and interval[0] == output[-1][1]:
                output[-1] = (output[-1][0], interval[1])
            else:
                output.append(interval)
    return output


def _subtract_ranges(old: list[tuple[int, int]], new: list[tuple[int, int]]) -> list[tuple[int, int]]:
    mask = bytearray(ROM_SIZE)
    for start, end in new:
        mask[start:end] = b"\1" * (end - start)
    for start, end in old:
        mask[start:end] = b"\0" * (end - start)
    result = []
    pos = 0
    while pos < ROM_SIZE:
        if not mask[pos]:
            pos += 1
            continue
        start = pos
        while pos < ROM_SIZE and mask[pos]:
            pos += 1
        result.append((start, pos))
    return result


def _source_owned_mask(entries: list[dict[str, Any]]) -> bytearray:
    mask = bytearray(ROM_SIZE)
    for entry in entries:
        if _owned(entry):
            start, end = int(entry["start"]), int(entry["end"])
            mask[start:end] = b"\1" * (end - start)
    return mask


def _reconcile(paths: dict[str, Path], receipt: dict[str, Any]) -> dict[str, Any]:
    loaded: dict[str, tuple[list[dict[str, Any]], str, bytearray]] = {}
    expected_by_name = {r["name"]: r for r in receipt["sources"]}
    for name, path in paths.items():
        data = json.loads(path.read_text(encoding="utf-8"))
        entries = data.get("entries", [])
        if data.get("schema") != "oasis.full-rom-split.v1" or data.get("rom_sha256") != ROM_SHA:
            raise ValueError("STOP_SOURCE_OWNED_MANIFEST_IDENTITY_MISMATCH")
        mask = _source_owned_mask(entries)
        count = sum(mask)
        digest = _hash_file(path)
        expected = expected_by_name[name]
        if count != int(data.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)) or \
                count != int(expected["source_owned_bytes"]) or digest != expected["source_sha256"]:
            raise ValueError("STOP_SOURCE_OWNED_IMPORT_MISMATCH")
        loaded[name] = (entries, digest, mask)
    base, current = loaded["AUTO60"], loaded["AUTO61"]
    added = _subtract_ranges(_owned_intervals(base[0]), _owned_intervals(current[0]))
    removed = _subtract_ranges(_owned_intervals(current[0]), _owned_intervals(base[0]))
    declared = [(int(r["start"]), int(r["end"])) for r in receipt["added_ranges"]]
    if added != declared or removed:
        raise ValueError("STOP_SOURCE_OWNED_ACCOUNTING_UNRECONCILED")
    conflicts = 0
    old_entries, new_entries = base[0], current[0]
    i = j = 0
    while i < len(old_entries) and j < len(new_entries):
        old, new = old_entries[i], new_entries[j]
        start = max(int(old["start"]), int(new["start"]))
        end = min(int(old["end"]), int(new["end"]))
        if start < end and _owned(old) and _owned(new):
            old_label = (old.get("kind"), old.get("classification", old.get("kind")), old.get("confidence"))
            new_label = (new.get("kind"), new.get("classification", new.get("kind")), new.get("confidence"))
            if old_label != new_label:
                conflicts += end - start
        if int(old["end"]) <= int(new["end"]):
            i += 1
        else:
            j += 1
    if conflicts != int(receipt["shared_owned_classification_difference_bytes"]):
        raise ValueError("STOP_SOURCE_OWNED_SHARED_CLASSIFICATION_CONFLICT")
    return {"status": "PASS_SOURCE_OWNED_ACCOUNTING_RECONCILED",
            "source_owned_bytes": sum(loaded["AUTO61"][2]), "added_bytes": sum(e-s for s,e in added),
            "added_ranges": len(added), "removed_ranges": len(removed),
            "shared_classification_conflict_bytes": conflicts}


def audit_database(db_path: Path, rom_path: Path, manifest_path: Path,
                   execution_path: Path, session_path: Path, carver_report_path: Path,
                   carver_interval_path: Path, reconciliation: dict[str, Any],
                   idempotence: dict[str, Any], manifest_paths: dict[str, Path] | None = None,
                   declared_artifact_paths: list[Path] | None = None) -> dict[str, Any]:
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or _hash_bytes(rom) != ROM_SHA:
        raise ValueError("STOP_ROM_IDENTITY_MISMATCH")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = manifest.get("entries", [])
    if manifest.get("rom_sha256") != ROM_SHA:
        raise ValueError("STOP_SOURCE_OWNED_MANIFEST_IDENTITY_MISMATCH")
    cursor = 0
    owned_bytes = 0
    artifact_base = manifest_path.parent.resolve()
    expected_emission = []
    for entry in entries:
        start, end = int(entry["start"]), int(entry["end"])
        if start != cursor or end <= start or end > ROM_SIZE:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        cursor = end
        owned = _owned(entry)
        owned_bytes += end - start if owned else 0
        artifact = PurePosixPath(str(entry.get("artifact", "")))
        artifact_path = (artifact_base / Path(*artifact.parts)).resolve()
        try:
            artifact_path.relative_to(artifact_base)
        except ValueError as exc:
            raise ValueError("STOP_EMISSION_ARTIFACT_PATH_INVALID") from exc
        if not artifact_path.is_file():
            raise ValueError("STOP_EMISSION_ARTIFACT_MISSING")
        if entry.get("emitted_artifact_type") in {"blob", "rom_asset"}:
            if artifact_path.read_bytes() != rom[start:end]:
                raise ValueError("STOP_EMISSION_BYTES_MISMATCH")
        expected_emission.append((start, end, _emission_type(entry),
            str(entry.get("classification", entry.get("kind", "UNKNOWN"))),
            str(entry.get("kind", "UNKNOWN")), int(owned),
            str(entry.get("emitted_artifact_type", "")), artifact.as_posix()))
    if cursor != ROM_SIZE or owned_bytes != int(manifest.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)):
        raise ValueError("STOP_SOURCE_OWNED_IMPORT_MISMATCH")
    if _hash_file(rom_path) != ROM_SHA:
        raise ValueError("STOP_EMISSION_CANONICAL_ROM_HASH_MISMATCH")
    if manifest_paths is not None:
        reconciliation_check = _reconcile(manifest_paths, reconciliation)
    else:
        reconciliation_check = {"status": "NOT_RECOMPUTED"}

    uri = "file:" + db_path.resolve().as_posix() + "?mode=ro&immutable=1"
    db = sqlite3.connect(uri, uri=True)
    db.row_factory = sqlite3.Row
    if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
        raise ValueError("STOP_KNOWLEDGE_DATABASE_INTEGRITY")
    meta = {r[0]: r[1] for r in db.execute("SELECT key,value FROM map_meta")}
    if meta.get("schema") not in {"oasis.m12.canonical-rom-knowledge.v1",
                                   "oasis.m14.canonical-rom-knowledge.v2"} or \
            meta.get("rom_sha256") != ROM_SHA or int(meta.get("rom_size", -1)) != ROM_SIZE:
        raise ValueError("STOP_KNOWLEDGE_DATABASE_IDENTITY")
    emission = [tuple(r) for r in db.execute("""SELECT start,end,emission_type,classification,
        source_kind,source_owned,artifact_type,artifact FROM emission ORDER BY start,end""")]
    if emission != expected_emission:
        raise ValueError("STOP_EMISSION_PARTITION_MISMATCH")
    if len(emission) != len(entries) or sum(r[1] - r[0] for r in emission) != ROM_SIZE:
        raise ValueError("STOP_EMISSION_COVERAGE_INCOMPLETE")

    source_owned_mask = _source_owned_mask(entries)
    object_rows = list(db.execute("SELECT object_id,range_id,object_type,attributes_json FROM rom_object"))
    range_rows = list(db.execute("SELECT range_id,rom_sha256,start,end FROM rom_range"))
    range_by_id = {str(r[0]): (str(r[1]), int(r[2]), int(r[3])) for r in range_rows}
    object_by_id = {}
    for row in object_rows:
        oid, rid, typ, attrs_text = str(row[0]), str(row[1]), str(row[2]), str(row[3])
        if rid not in range_by_id:
            raise ValueError("STOP_OBJECT_RANGE_MISSING")
        rom_sha, start, end = range_by_id[rid]
        expected_rid = _id("range", {"rom_sha256": ROM_SHA, "start": start, "end": end})
        expected_oid = _id("object", {"rom_sha256": ROM_SHA, "start": start,
                                       "end": end, "object_type": typ})
        if rom_sha != ROM_SHA or expected_rid != rid or expected_oid != oid or end > ROM_SIZE:
            raise ValueError("STOP_OBJECT_IDENTITY_CONFLICT")
        attrs = json.loads(attrs_text)
        owned_count = attrs.get("source_owned_bytes")
        if "bytes_hex" in attrs or not isinstance(owned_count, int) or \
                owned_count < 0 or owned_count > end - start or \
                owned_count != sum(source_owned_mask[start:end]):
            raise ValueError("STOP_OBJECT_ATTRIBUTES_INVALID")
        object_by_id[oid] = (typ, start, end, attrs)
    if len(object_by_id) != len(object_rows) or len(range_by_id) != len(range_rows):
        raise ValueError("STOP_OBJECT_DEDUPLICATION_FAILURE")

    claim_by_id, relation_ids, evidence_ids = audit_record_ids(
        db, entries, object_by_id, source_owned_mask, ROM_SHA)

    execution = json.loads(execution_path.read_text(encoding="utf-8"))
    if execution.get("rom_sha256") != ROM_SHA or len(execution.get("ranges", [])) != 330:
        raise ValueError("STOP_2B_RUNTIME_IMPORT_MISMATCH")
    expected_instruction_ids = set()
    unique_intervals = []
    occurrences = 0
    for r in execution["ranges"]:
        start, end = int(r["start_offset"]), int(r["end_offset_exclusive"])
        bytes_value = bytes.fromhex(r["bytes_hex"])
        if end - start != len(bytes_value) or rom[start:end] != bytes_value or \
                _hash_bytes(bytes_value) != r["bytes_sha256"] or int(r["opcode"]) != int.from_bytes(bytes_value[:2], "big"):
            raise ValueError("STOP_2B_RUNTIME_BYTES_MISMATCH")
        oid = _id("object", {"rom_sha256": ROM_SHA, "start": start,
                             "end": end, "object_type": "M68K_INSTRUCTION"})
        if oid not in object_by_id or object_by_id[oid][0] != "M68K_INSTRUCTION":
            raise ValueError("STOP_2B_RUNTIME_OBJECT_MISSING")
        attrs = object_by_id[oid][3]
        owned_count = sum(source_owned_mask[start:end])
        if attrs.get("bytes_sha256") != r["bytes_sha256"] or attrs.get("opcode") != int(r["opcode"]) or \
                attrs.get("length") != end - start or attrs.get("source_owned_bytes") != owned_count:
            raise ValueError("STOP_2B_RUNTIME_OBJECT_ATTRIBUTES_MISMATCH")
        expected_instruction_ids.add(oid)
        unique_intervals.append((start, end))
        occurrences += int(r["evidence_count"])
    if occurrences != 199630 or _union_bytes(unique_intervals) != 1244:
        raise ValueError("STOP_2B_RUNTIME_OCCURRENCE_COUNT_MISMATCH")
    instruction_objects = {oid for oid, item in object_by_id.items() if item[0] == "M68K_INSTRUCTION"}
    if instruction_objects != expected_instruction_ids:
        raise ValueError("STOP_2B_RUNTIME_OBJECT_DUPLICATION")
    owned_claims_on_instructions = int(db.execute("""SELECT COUNT(*) FROM claim c JOIN rom_object o USING(object_id)
        WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='SOURCE_OWNED'""").fetchone()[0])
    if owned_claims_on_instructions:
        raise ValueError("STOP_RUNTIME_PROMOTED_SOURCE_OWNED")

    session_uri = "file:" + session_path.resolve().as_posix() + "?mode=ro&immutable=1"
    source = sqlite3.connect(session_uri, uri=True)
    source.row_factory = sqlite3.Row
    source_meta = {r[0]: r[1] for r in source.execute("SELECT key,value FROM map_meta")}
    if source_meta.get("rom_sha256") != ROM_SHA or source_meta.get("live_forward_session_state") != "CLOSED":
        raise ValueError("STOP_2B_RUNTIME_SESSION_IDENTITY_MISMATCH")
    run_id = str(source_meta.get("live_forward_run_id", ""))
    nodes = {str(r["node_id"]): dict(r) for r in source.execute(
        "SELECT node_id,kind,node_key,body FROM map_node")}
    exported_ranges = {str(r["range_node_id"]): r for r in execution["ranges"]}
    source_to_object: dict[str, str] = {}
    export_source_hash = str(db.execute("SELECT source_sha256 FROM source_artifact WHERE artifact_name=?",
                                        ("rom_execution_ranges.json",)).fetchone()[0])
    session_source_hash = str(db.execute("SELECT source_sha256 FROM source_artifact WHERE artifact_name=?",
                                         ("session-rom-link.sqlite",)).fetchone()[0])
    for edge in source.execute("""SELECT edge_id,source_id,target_id,json_array_length(lineage) n
        FROM map_edge WHERE relation='EXECUTED_FROM_ROM' ORDER BY edge_id"""):
        row = exported_ranges.get(str(edge["target_id"]))
        node = nodes.get(str(edge["source_id"]))
        if row is None or node is None or node["kind"] != "M68K_INSTRUCTION" or \
                int(edge["n"]) != int(row["evidence_count"]):
            raise ValueError("STOP_2B_RUNTIME_RANGE_LINEAGE_MISMATCH")
        start, end = int(row["start_offset"]), int(row["end_offset_exclusive"])
        pc_text, opcode_text = str(node["node_key"]).split(":", 1)
        if int(pc_text, 16) != start or int(opcode_text, 16) != int(row["opcode"]):
            raise ValueError("STOP_2B_RUNTIME_RANGE_PC_MISMATCH")
        oid = _id("object", {"rom_sha256": ROM_SHA, "start": start,
                             "end": end, "object_type": "M68K_INSTRUCTION"})
        if oid not in expected_instruction_ids or str(edge["source_id"]) in source_to_object:
            raise ValueError("STOP_2B_RUNTIME_RANGE_DEDUP_MISMATCH")
        source_to_object[str(edge["source_id"])] = oid
        claim_id = _id("claim", {"object_id": oid, "claim_type": "EXECUTED_FROM_ROM",
                                  "value": True, "status": "OBSERVED_RUNTIME"})
        expected_refs = [
            ("RUNTIME_INSTRUCTION_RANGE", 1, {"range_node_id": str(row["range_node_id"]),
                                                "start": start, "end": end},
             export_source_hash),
            ("RUNTIME_INSTRUCTION_OCCURRENCE", int(edge["n"]),
             {"table": "map_edge", "edge_id": str(edge["edge_id"]), "run_id": run_id},
             session_source_hash),
        ]
        for kind, count, locator, source_hash in expected_refs:
            payload = {"subject_type": "CLAIM", "subject_id": claim_id,
                       "source_sha256": source_hash, "fact_kind": kind,
                       "fact_count": count, "locator": locator}
            if _id("evidence", payload) not in evidence_ids:
                raise ValueError("STOP_2B_RUNTIME_EVIDENCE_LINEAGE_MISMATCH")
    if len(source_to_object) != 330:
        raise ValueError("STOP_2B_RUNTIME_RANGE_DEDUP_MISMATCH")
    relations = list(source.execute("""SELECT relation,COUNT(*),SUM(json_array_length(lineage))
        FROM map_edge WHERE relation IN ('EXECUTED_FROM_ROM','EXECUTED_NEXT','OBSERVED_NEXT_PC')
        GROUP BY relation ORDER BY relation"""))
    source_relations = {str(r[0]): (int(r[1]), int(r[2] or 0)) for r in relations}
    if source_relations != {"EXECUTED_FROM_ROM": (330, 199630),
                            "EXECUTED_NEXT": (385, 197520),
                            "OBSERVED_NEXT_PC": (26, 1600)}:
        raise ValueError("STOP_2B_RUNTIME_RELATION_COUNTS_MISMATCH")
    relation_shapes = {(str(r[0]), str(r[1]), str(r[2])): (int(r[3]), int(r[4] or 0))
        for r in source.execute("""SELECT e.relation,s.kind,t.kind,COUNT(*),
            SUM(json_array_length(e.lineage)) FROM map_edge e
            JOIN map_node s ON s.node_id=e.source_id JOIN map_node t ON t.node_id=e.target_id
            WHERE e.relation IN ('EXECUTED_NEXT','OBSERVED_NEXT_PC')
            GROUP BY e.relation,s.kind,t.kind ORDER BY e.relation,s.kind,t.kind""")}
    expected_shapes = {
        ("EXECUTED_NEXT", "M68K_INSTRUCTION", "M68K_INSTRUCTION"): (338, 195794),
        ("EXECUTED_NEXT", "M68K_EXCEPTION_EVENT", "M68K_INSTRUCTION"): (23, 863),
        ("EXECUTED_NEXT", "M68K_INSTRUCTION", "M68K_EXCEPTION_EVENT"): (24, 863),
        ("OBSERVED_NEXT_PC", "M68K_INSTRUCTION", "M68K_TARGET_ADDRESS"): (26, 1600),
    }
    if relation_shapes != expected_shapes:
        raise ValueError("STOP_2B_RUNTIME_RELATION_ENDPOINT_CLASSIFICATION_MISMATCH")
    expected_runtime_relation_ids = set()
    flow_edges = source.execute("""SELECT edge_id,source_id,target_id,relation,
        json_array_length(lineage) n FROM map_edge
        WHERE relation IN ('EXECUTED_NEXT','OBSERVED_NEXT_PC') ORDER BY edge_id""")
    for edge in flow_edges:
        relation_type = str(edge["relation"])
        source_id, target_id = str(edge["source_id"]), str(edge["target_id"])
        source_node, target_node = nodes.get(source_id), nodes.get(target_id)
        count = int(edge["n"] or 0)
        if source_node is None or target_node is None or count <= 0:
            raise ValueError("STOP_2B_RUNTIME_RELATION_ENDPOINT_MISSING")
        if relation_type == "EXECUTED_NEXT":
            if source_node["kind"] != "M68K_INSTRUCTION" or target_node["kind"] != "M68K_INSTRUCTION":
                continue
            source_object, target_object, target_address = source_to_object.get(source_id), source_to_object.get(target_id), None
            fact_kind = "RUNTIME_NEXT_OCCURRENCE"
        else:
            if source_node["kind"] != "M68K_INSTRUCTION" or target_node["kind"] != "M68K_TARGET_ADDRESS":
                raise ValueError("STOP_2B_TERMINAL_TARGET_PROMOTED")
            source_object, target_object = source_to_object.get(source_id), None
            target_address = int(json.loads(str(target_node["body"]))["attributes"]["raw_next_pc"])
            fact_kind = "RUNTIME_TERMINAL_FACT"
        if source_object is None or (relation_type == "EXECUTED_NEXT" and target_object is None):
            raise ValueError("STOP_2B_RUNTIME_RELATION_UNCAPTURED_INSTRUCTION")
        payload = {"relation_type": relation_type, "source_object_id": source_object,
                   "target_object_id": target_object, "target_address": target_address,
                   "status": "OBSERVED_RUNTIME"}
        relation_id = _id("relation", payload)
        expected_runtime_relation_ids.add(relation_id)
        locator = {"table": "map_edge", "edge_id": str(edge["edge_id"]), "run_id": run_id}
        evidence_payload = {"subject_type": "RELATION", "subject_id": relation_id,
            "source_sha256": session_source_hash, "fact_kind": fact_kind,
            "fact_count": count, "locator": locator}
        if _id("evidence", evidence_payload) not in evidence_ids:
            raise ValueError("STOP_2B_RUNTIME_RELATION_EVIDENCE_MISMATCH")
    if expected_runtime_relation_ids != relation_ids:
        raise ValueError("STOP_2B_RUNTIME_RELATION_IMPORT_MISMATCH")
    imported_relations = {str(r[0]): int(r[1]) for r in db.execute(
        "SELECT relation_type,COUNT(*) FROM relation GROUP BY relation_type")}
    if imported_relations != {"EXECUTED_NEXT": 338, "OBSERVED_NEXT_PC": 26}:
        raise ValueError("STOP_2B_RUNTIME_RELATION_IMPORT_MISMATCH")
    ref_facts = {str(r[0]): int(r[1]) for r in db.execute(
        "SELECT fact_kind,SUM(fact_count) FROM evidence_ref GROUP BY fact_kind")}
    if ref_facts.get("RUNTIME_INSTRUCTION_OCCURRENCE") != 199630 or \
            ref_facts.get("RUNTIME_NEXT_OCCURRENCE") != 195794 or \
            ref_facts.get("RUNTIME_TERMINAL_FACT") != 1600:
        raise ValueError("STOP_EVIDENCE_REFERENCE_COUNT_MISMATCH")

    carver = json.loads(carver_report_path.read_text(encoding="utf-8"))
    interval = json.loads(carver_interval_path.read_text(encoding="utf-8"))
    candidates = carver.get("formats", {}).get("candidates", [])
    if len(candidates) != 1015 or carver.get("final", {}).get("promoted_bytes") != 0 or \
            interval.get("conflicts") != []:
        raise ValueError("STOP_CARVER_HYPOTHESIS_IMPORT_MISMATCH")
    hclaims = int(db.execute("SELECT COUNT(*) FROM claim WHERE status='HYPOTHESIS'").fetchone()[0])
    if hclaims <= 0:
        raise ValueError("STOP_CARVER_HYPOTHESIS_IMPORT_MISSING")

    artifacts = {str(r[0]): (str(r[1]), str(r[2]), str(r[3])) for r in db.execute(
        "SELECT source_sha256,checkpoint,artifact_name,artifact_type FROM source_artifact")}
    for (source_hash,) in db.execute("SELECT DISTINCT source_sha256 FROM evidence_ref"):
        if str(source_hash) not in artifacts or len(str(source_hash)) != 64:
            raise ValueError("STOP_EVIDENCE_REFERENCE_UNRESOLVED")
    artifact_paths = declared_artifact_paths or [rom_path, manifest_path, execution_path,
        session_path, carver_report_path, carver_interval_path]
    actual_hashes = {_hash_file(path) for path in artifact_paths if path.is_file()}
    if any(not path.is_file() for path in artifact_paths):
        raise ValueError("STOP_EVIDENCE_SOURCE_MISSING")
    if actual_hashes != set(artifacts):
        raise ValueError("STOP_EVIDENCE_SOURCE_HASH_UNRESOLVED")
    if idempotence.get("status") != "PASS_IDEMPOTENT_REIMPORT" or \
            idempotence.get("first_hashes") != idempotence.get("reimport_hashes") or \
            idempotence.get("first_counts") != idempotence.get("reimport_counts"):
        raise ValueError("STOP_NONDETERMINISTIC_CANONICAL_MAP")

    conflicts = int(db.execute("SELECT COUNT(*) FROM conflict").fetchone()[0])
    source_owned_db = int(db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0] or 0)
    if source_owned_db != owned_bytes or conflicts != 0:
        raise ValueError("STOP_SOURCE_OWNED_IMPORT_MISMATCH")
    map_hashes = _database_hashes(db)
    db.close()
    source.close()
    return {"status": "PASS_INDEPENDENT_CANONICAL_ROM_KNOWLEDGE_AUDIT",
        "rom_sha256": ROM_SHA, "rom_bytes": len(rom),
        "emission_partition": {"rows": len(expected_emission), "coverage_bytes": ROM_SIZE,
                               "gaps": 0, "overlaps": 0, "out_of_bounds": 0,
                               "artifacts_resolved": True},
        "source_owned": {"bytes": owned_bytes, "manifest_bytes": int(manifest["metrics"]["SOURCE_OWNED_BYTES"]),
                         "delta_from_manifest": 0, "runtime_claims_promoted": 0},
        "ownership_reconciliation": reconciliation_check,
        "runtime_2b": {"instruction_objects": len(instruction_objects),
            "instruction_occurrences": occurrences,
            "unique_instruction_bytes": _union_bytes(unique_intervals),
            "executed_next_relations": 338, "executed_next_occurrences": 195794,
            "excluded_exception_flow_edges": 47, "excluded_exception_flow_occurrences": 1726,
            "terminal_relation_objects": 26, "terminal_facts": 1600,
            "range_bytes_reconciled": 330, "unresolved": 0, "unsupported": 0},
        "carver_hypotheses": {"candidate_records": len(candidates),
                              "unique_hypothesis_claims": hclaims,
                              "promoted_bytes": 0, "interval_db_conflicts": 0},
        "source_artifacts": len(artifacts), "evidence_refs_resolved": True,
        "database_hashes": map_hashes}


def _union_bytes(intervals: list[tuple[int, int]]) -> int:
    total, cursor = 0, -1
    for start, end in sorted(intervals):
        if end > cursor:
            total += end - max(cursor, start)
            cursor = end
    return total


def _database_hashes(db: sqlite3.Connection) -> dict[str, str]:
    cols = {
        "rom_range": "range_id,rom_sha256,start,end",
        "rom_object": "object_id,range_id,object_type,attributes_json",
        "claim": "claim_id,object_id,claim_type,value_json,status",
        "relation": "relation_id,relation_type,source_object_id,target_object_id,target_address,status,attributes_json",
        "conflict": "conflict_id,start,end,conflict_type,detail_json",
        "source_artifact": "source_sha256,checkpoint,artifact_name,artifact_type",
        "evidence_ref": "ref_id,subject_type,subject_id,source_sha256,fact_kind,fact_count,locator_json",
        "emission": "start,end,emission_type,classification,source_kind,source_owned,artifact_type,artifact",
    }
    def rows(table: str) -> list[tuple[Any, ...]]:
        return [tuple(r) for r in db.execute(f"SELECT {cols[table]} FROM {table} ORDER BY {cols[table]}")]
    structure = {table: rows(table) for table in ("rom_range", "rom_object", "claim", "relation", "conflict")}
    evidence = {table: rows(table) for table in ("source_artifact", "evidence_ref")}
    emission = rows("emission")
    sh = _hash_bytes(_canonical(structure).encode())
    eh = _hash_bytes(_canonical(evidence).encode())
    mh = _hash_bytes(_canonical(emission).encode())
    return {"structure_hash": sh, "evidence_index_hash": eh, "emission_hash": mh, "map_hash": _hash_bytes((sh + eh + mh).encode())}
