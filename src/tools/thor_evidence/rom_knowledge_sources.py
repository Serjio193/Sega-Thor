"""Read-only adapters for accepted ownership, Carver and FLOW_V1 evidence."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from pathlib import Path, PurePosixPath
from typing import Any

try:
    from .identity import ROM_SHA, ROM_SIZE
    from .rom_knowledge_map import object_id, range_id
except ImportError:
    from identity import ROM_SHA, ROM_SIZE
    from rom_knowledge_map import object_id, range_id


UNKNOWN_KINDS = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}
UNCONFIRMED = {"PROBABLE", "CANDIDATE", "UNVERIFIED"}
GRAPHICS_STREAMS = {
    "GRAPHICS_COMPRESSED_STREAM", "CONSUMER_BACKED_GRAPHICS_STREAM",
    "TABLE_SELECTED_GRAPHICS_STREAM", "DIRECT_GRAPHICS_STREAM",
    "GRAPHICS_DIRECT_LOADER_STREAM", "DIRECT_MENU_GRAPHICS_STREAM",
    "RUNTIME_CORRELATED_GRAPHICS_STREAM", "EXACT_3820_GRAPHICS_STREAM",
}
POINTER_TABLES = {
    "ABSOLUTE_STATE_DISPATCH_POINTER_TABLE", "STATIC_DISPATCH_POINTER_TABLE",
    "STATIC_MULTI_DISPATCH_POINTER_TABLE", "SCREEN_GROUP_POINTER_TABLE",
    "GROUP_POINTER_TABLE_32X32", "COMPRESSED_RESOURCE_POINTER_TABLE",
    "CCB0_RELATIVE_TARGET_TABLE_256X16", "SIGNED_RELATIVE_EVENT_DISPATCH_TABLE",
}
EXPECTED_OWNED = {
    "AUTO60": 1_427_873, "GFX2": 1_475_262,
    "GFXMAX": 1_475_368, "AUTO61": 1_475_600,
    "GFXMAX_ROOT_C": 1_475_346,
}
EXPECTED_DELTAS = {
    ("AUTO60", "GFX2"): [(0x18F252, 0x190EE7), (0x191F8E, 0x194BDA),
                           (0x194C52, 0x196289), (0x196B87, 0x19908F),
                           (0x199D35, 0x19D232)],
    ("GFX2", "GFXMAX"): [(0x00C92C, 0x00C980), (0x02E1D8, 0x02E1EE)],
    ("GFXMAX", "AUTO61"): [(0x03B95E, 0x03BA46)],
    ("GFXMAX_ROOT_C", "GFXMAX"): [(0x02E1D8, 0x02E1EE)],
}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _json_file(path: Path) -> tuple[dict[str, Any], str]:
    return json.loads(path.read_text(encoding="utf-8")), sha256_file(path)


def _owned(entry: dict[str, Any]) -> bool:
    return entry.get("kind") not in UNKNOWN_KINDS and \
        str(entry.get("confidence", "")).upper() not in UNCONFIRMED


def _manifest(path: Path, rom: bytes) -> tuple[dict[str, Any], str, bytearray]:
    data, source_hash = _json_file(path)
    if data.get("schema") != "oasis.full-rom-split.v1" or \
            data.get("rom_sha256") != ROM_SHA or len(rom) != ROM_SIZE or \
            int(data.get("start", -1)) != 0 or int(data.get("end", -1)) != ROM_SIZE or \
            int(data.get("rom_size", -1)) != ROM_SIZE:
        raise ValueError("STOP_SOURCE_OWNED_MANIFEST_IDENTITY_MISMATCH")
    cursor = 0
    mask = bytearray(ROM_SIZE)
    owned_bytes = 0
    for entry in data.get("entries", []):
        start, end = int(entry["start"]), int(entry["end"])
        if start != cursor or end <= start or end > ROM_SIZE or \
                int(entry.get("size", end - start)) != end - start:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        cursor = end
        if _owned(entry):
            owned_bytes += end - start
            mask[start:end] = b"\1" * (end - start)
    if cursor != ROM_SIZE or int(data.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)) != owned_bytes:
        raise ValueError("STOP_SOURCE_OWNED_IMPORT_MISMATCH")
    if int(data.get("metrics", {}).get("CONFLICT_BYTES", 0)) != 0:
        raise ValueError("STOP_SOURCE_OWNED_MANIFEST_CONFLICT")
    return data, source_hash, mask


def _runs(mask: bytearray) -> list[tuple[int, int]]:
    output: list[tuple[int, int]] = []
    index = 0
    while index < len(mask):
        if not mask[index]:
            index += 1
            continue
        start = index
        while index < len(mask) and mask[index]:
            index += 1
        output.append((start, index))
    return output


def reconcile_manifests(paths: dict[str, Path], rom: bytes) -> tuple[dict[str, Any], dict[str, tuple[dict[str, Any], str]]]:
    loaded: dict[str, tuple[dict[str, Any], str, bytearray]] = {}
    for name, path in paths.items():
        loaded[name] = _manifest(path, rom)
        if int(loaded[name][0]["metrics"]["SOURCE_OWNED_BYTES"]) != EXPECTED_OWNED[name]:
            raise ValueError(f"STOP_SOURCE_OWNED_ACCOUNTING_UNRECONCILED:{name}")
    steps = [("AUTO60", "GFX2"), ("GFX2", "GFXMAX"), ("GFXMAX", "AUTO61"),
             ("GFXMAX_ROOT_C", "GFXMAX")]
    deltas: list[dict[str, Any]] = []
    for old_name, new_name in steps:
        old_mask, new_mask = loaded[old_name][2], loaded[new_name][2]
        only_new = bytearray(1 if new_mask[i] and not old_mask[i] else 0 for i in range(ROM_SIZE))
        only_old = bytearray(1 if old_mask[i] and not new_mask[i] else 0 for i in range(ROM_SIZE))
        added, removed = _runs(only_new), _runs(only_old)
        expected = EXPECTED_DELTAS[(old_name, new_name)]
        if added != expected or removed:
            raise ValueError(f"STOP_SOURCE_OWNED_ACCOUNTING_UNRECONCILED:{old_name}:{new_name}")
        if old_name == "AUTO60" and new_name == "AUTO61":
            continue
        deltas.append({"from": old_name, "to": new_name,
                       "bytes_delta": sum(end - start for start, end in added),
                       "added_ranges": [{"start": s, "end": e, "bytes": e - s} for s, e in added],
                       "removed_ranges": []})
    old_mask, new_mask = loaded["AUTO60"][2], loaded["AUTO61"][2]
    only_new = bytearray(1 if new_mask[i] and not old_mask[i] else 0 for i in range(ROM_SIZE))
    only_old = bytearray(1 if old_mask[i] and not new_mask[i] else 0 for i in range(ROM_SIZE))
    a, b = loaded["AUTO60"][0], loaded["AUTO61"][0]
    shared_class_diff = 0
    for entry in a["entries"]:
        if not _owned(entry):
            continue
        label = (str(entry.get("kind", "UNKNOWN")),
                 str(entry.get("classification", entry.get("kind", "UNKNOWN"))),
                 str(entry.get("confidence", "UNKNOWN")))
        # The labels arrays use local interning. Compare values directly by walking
        # the relatively small source-owned interval intersections below.
        for newer in b["entries"]:
            if not _owned(newer):
                continue
            start, end = max(int(entry["start"]), int(newer["start"])), min(int(entry["end"]), int(newer["end"]))
            newer_label = (str(newer.get("kind", "UNKNOWN")),
                           str(newer.get("classification", newer.get("kind", "UNKNOWN"))),
                           str(newer.get("confidence", "UNKNOWN")))
            if start < end and label != newer_label:
                shared_class_diff += end - start
    if shared_class_diff or _runs(only_old):
        raise ValueError("STOP_SOURCE_OWNED_ACCOUNTING_UNRECONCILED:shared-classification")
    additions = _runs(only_new)
    if sum(e - s for s, e in additions) != EXPECTED_OWNED["AUTO61"] - EXPECTED_OWNED["AUTO60"]:
        raise ValueError("STOP_SOURCE_OWNED_ACCOUNTING_UNRECONCILED:delta")
    summary = {
        "status": "PASS_SOURCE_OWNED_ACCOUNTING_RECONCILED",
        "rom_sha256": ROM_SHA,
        "sources": [{"name": name, "checkpoint": name, "source_sha256": loaded[name][1],
                     "source_owned_bytes": int(loaded[name][0]["metrics"]["SOURCE_OWNED_BYTES"]),
                     "owned_interval_count": len(_runs(loaded[name][2])),
                     "manifest_entries": len(loaded[name][0]["entries"])}
                    for name in ("AUTO60", "GFX2", "GFXMAX", "AUTO61", "GFXMAX_ROOT_C")],
        "baseline": "AUTO60", "authoritative": "AUTO61",
        "bytes_delta": sum(e - s for s, e in additions),
        "maximal_owned_interval_count_delta": len(_runs(loaded["AUTO61"][2])) - len(_runs(loaded["AUTO60"][2])),
        "added_range_count": len(additions), "removed_range_count": len(_runs(only_old)),
        "removed_bytes": sum(end - start for start, end in _runs(only_old)),
        "added_ranges": [{"start": s, "end": e, "bytes": e - s} for s, e in additions],
        "overlap_bytes": sum(1 for i in range(ROM_SIZE) if old_mask[i] and new_mask[i]),
        "shared_owned_classification_difference_bytes": shared_class_diff,
        "manifest_conflict_bytes": {name: int(loaded[name][0]["metrics"].get("CONFLICT_BYTES", 0))
                                    for name in ("AUTO60", "AUTO61")},
        "stale_root_c": {"status": "STALE_OMITS_DESCRIPTOR_CANDIDATE",
                         "missing_bytes": sum(e - s for s, e in EXPECTED_DELTAS[("GFXMAX_ROOT_C", "GFXMAX")]),
                         "missing_ranges": [{"start": s, "end": e, "bytes": e - s}
                                            for s, e in EXPECTED_DELTAS[("GFXMAX_ROOT_C", "GFXMAX")]]},
        "steps": deltas,
        "authoritative_checkpoint_commit": "e1e27e084d7beb600fdb47c2df0c0fb3462b4788",
    }
    meta = {name: (loaded[name][0], loaded[name][1]) for name in loaded}
    return summary, meta


def object_type_for(entry: dict[str, Any]) -> str:
    kind = str(entry.get("kind", "UNKNOWN"))
    classification = str(entry.get("classification", kind))
    if kind in UNKNOWN_KINDS:
        return "UNKNOWN"
    if classification in GRAPHICS_STREAMS:
        return "GRAPHICS_STREAM"
    if classification == "SOUND_DATA_CONTAINER_CONFIRMED":
        return "AUDIO_DATA"
    if classification == "Z80_ASM_SOURCE_OWNED":
        return "Z80_PROGRAM"
    if classification in POINTER_TABLES:
        return "POINTER_TABLE"
    if kind in {"STRUCTURED_DATA_CONFIRMED", "DATA_KNOWN", "PADDING_ALIGNMENT_CONFIRMED",
                "HEADER_VECTOR_ASM"}:
        return "ROM_DATA"
    return "ROM_RANGE"


def emission_type_for(entry: dict[str, Any]) -> str:
    artifact_type = str(entry.get("emitted_artifact_type", ""))
    kind = str(entry.get("kind", "UNKNOWN"))
    if artifact_type == "asm":
        return "ASM"
    if artifact_type == "blob":
        return "INCBIN"
    if artifact_type == "rom_asset" and kind == "LOCAL_ROM_DERIVED_ASSET":
        return "ASSET"
    if artifact_type == "rom_asset" and kind in {
            "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED"}:
        return "DATA"
    raise ValueError(f"STOP_EMISSION_TYPE_UNSUPPORTED:{artifact_type}:{kind}")


def _add_object(bundle: dict[str, list[dict[str, Any]]], rom_sha: str, start: int, end: int,
                object_type: str, source_owned_bytes: int, attributes: dict[str, Any] | None = None) -> str:
    rid, oid = range_id(rom_sha, start, end), object_id(rom_sha, start, end, object_type)
    bundle["rom_range"].append({"range_id": rid, "rom_sha256": rom_sha, "start": start, "end": end})
    merged = {"source_owned_bytes": source_owned_bytes}
    if attributes:
        merged.update(attributes)
    bundle["rom_object"].append({"object_id": oid, "range_id": rid,
                                  "object_type": object_type,
                                  "attributes_json": json.dumps(merged, sort_keys=True,
                                                               separators=(",", ":"))})
    return oid


def _claim(bundle: dict[str, list[dict[str, Any]]], oid: str, claim_type: str,
           value: Any, status: str) -> str:
    value_json = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    cid = hashlib.sha256(json.dumps({"object_id": oid, "claim_type": claim_type,
        "value": value, "status": status}, sort_keys=True, separators=(",", ":"),
        ensure_ascii=True).encode()).hexdigest()
    claim_id = f"claim:{cid}"
    bundle["claim"].append({"claim_id": claim_id, "object_id": oid,
                            "claim_type": claim_type, "value_json": value_json, "status": status})
    return claim_id


def _evidence(bundle: dict[str, list[dict[str, Any]]], subject_type: str, subject_id: str,
              source_sha: str, fact_kind: str, count: int, locator: dict[str, Any]) -> None:
    locator_json = json.dumps(locator, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    raw = json.dumps({"subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": source_sha, "fact_kind": fact_kind,
        "fact_count": count, "locator": locator}, sort_keys=True,
        separators=(",", ":"), ensure_ascii=True).encode()
    bundle["evidence_ref"].append({"ref_id": "evidence:" + hashlib.sha256(raw).hexdigest(),
        "subject_type": subject_type, "subject_id": subject_id, "source_sha256": source_sha,
        "fact_kind": fact_kind, "fact_count": count, "locator_json": locator_json})


def manifest_rows(bundle: dict[str, list[dict[str, Any]]], data: dict[str, Any],
                  manifest_sha: str) -> None:
    for index, entry in enumerate(data["entries"]):
        start, end = int(entry["start"]), int(entry["end"])
        owned = _owned(entry)
        kind = str(entry.get("kind", "UNKNOWN"))
        classification = str(entry.get("classification", kind))
        oid = _add_object(bundle, ROM_SHA, start, end, object_type_for(entry),
                          end - start if owned else 0)
        source_class = {"source_kind": kind, "classification": classification,
                        "confidence": str(entry.get("confidence", "UNKNOWN"))}
        status = "STATIC_VERIFIED" if owned else "DERIVED_EXACT"
        _evidence(bundle, "CLAIM", _claim(bundle, oid, "SOURCE_CLASS", source_class, status),
                  manifest_sha, "SOURCE_MANIFEST_CLASSIFICATION", 1,
                  {"manifest_index": index, "start": start, "end": end})
        owned_status = "STATIC_VERIFIED" if owned else "DERIVED_EXACT"
        _evidence(bundle, "CLAIM", _claim(bundle, oid, "SOURCE_OWNED", owned, owned_status),
                  manifest_sha, "SOURCE_OWNERSHIP_DECISION", 1,
                  {"manifest_index": index, "start": start, "end": end, "bytes": end - start})
        artifact = PurePosixPath(str(entry.get("artifact", ""))).as_posix()
        if not artifact or artifact.startswith("/") or ".." in PurePosixPath(artifact).parts:
            raise ValueError("STOP_EMISSION_ARTIFACT_PATH_INVALID")
        bundle["emission"].append({"start": start, "end": end,
            "emission_type": emission_type_for(entry), "classification": classification,
            "source_kind": kind, "source_owned": int(owned),
            "artifact_type": str(entry["emitted_artifact_type"]), "artifact": artifact})


def import_carver_hypotheses(bundle: dict[str, list[dict[str, Any]]], report: dict[str, Any],
                             report_sha: str, interval_sha: str, owned_ranges: list[tuple[int, int]]) -> int:
    candidates = report.get("formats", {}).get("candidates", [])
    if report.get("schema") != "oasis.m68k.m12-carver.format-container-reconstruction.v1" or \
            not report.get("deterministic") or int(report.get("baseline", {}).get("source_owned_bytes", -1)) != 1_427_873 or \
            int(report.get("final", {}).get("promoted_bytes", -1)) != 0:
        raise ValueError("STOP_CARVER_HYPOTHESIS_SOURCE_INVALID")
    owned_prefix: list[tuple[int, int]] = owned_ranges
    for index, item in enumerate(candidates):
        start, end = int(item["start"]), int(item["end"])
        if start < 0 or end <= start or end > ROM_SIZE:
            raise ValueError("STOP_CARVER_HYPOTHESIS_BOUNDS")
        owned_count = sum(max(0, min(end, hi) - max(start, lo))
                          for lo, hi in owned_prefix if lo < end and start < hi)
        oid = _add_object(bundle, ROM_SHA, start, end, "ROM_RANGE", owned_count)
        claim_value = {"candidate_kind": str(item.get("kind", "UNKNOWN")),
                       "exact_boundary": bool(item.get("exact_boundary", False)),
                       "promotable": bool(item.get("promotable", False)),
                       "rejection": str(item.get("rejection", ""))}
        cid = _claim(bundle, oid, "FORMAT_HYPOTHESIS", claim_value, "HYPOTHESIS")
        locator = {"json_pointer": f"/formats/candidates/{index}",
                   "range_id": str(item.get("range_id", "")), "start": start, "end": end}
        _evidence(bundle, "CLAIM", cid, report_sha, "CARVER_FORMAT_CANDIDATE", 1, locator)
        _evidence(bundle, "CLAIM", cid, interval_sha, "CARVER_INTERVAL_CONTEXT", 1,
                  {"range_id": str(item.get("range_id", ""))})
    return len(candidates)


def read_2b(bundle: dict[str, list[dict[str, Any]]], rom: bytes, export: dict[str, Any],
            export_sha: str, session_db_path: Path, session_sha: str,
            run_id: str) -> dict[str, int]:
    if export.get("schema") != "oasis.m12.live-forward-rom-ranges.v1" or \
            export.get("rom_sha256") != ROM_SHA or \
            export.get("status") != "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE":
        raise ValueError("STOP_2B_RUNTIME_IMPORT_MISMATCH")
    metrics = export.get("metrics", {})
    expected = {"instruction_occurrences": 199630, "unique_instruction_ranges": 330,
                "unique_instruction_bytes": 1244, "terminal_next_pc_facts": 1600,
                "unresolved_instruction_occurrences": 0, "unsupported_decode_occurrences": 0}
    if any(int(metrics.get(key, -1)) != value for key, value in expected.items()):
        raise ValueError("STOP_2B_RUNTIME_IMPORT_MISMATCH")
    uri = "file:" + session_db_path.resolve().as_posix() + "?mode=ro&immutable=1"
    db = sqlite3.connect(uri, uri=True)
    db.row_factory = sqlite3.Row
    meta = {r[0]: r[1] for r in db.execute("SELECT key,value FROM map_meta")}
    if meta.get("rom_sha256") != ROM_SHA or meta.get("live_forward_session_state") != "CLOSED" or \
            meta.get("live_forward_run_id") != str(run_id) or meta.get("source_owned_bytes") != "0":
        raise ValueError("STOP_2B_RUNTIME_SESSION_IDENTITY_MISMATCH")
    try:
        range_edges = list(db.execute("""SELECT edge_id,source_id,target_id,
            json_array_length(lineage) AS fact_count FROM map_edge WHERE relation='EXECUTED_FROM_ROM'
            ORDER BY edge_id"""))
        flow_edges = list(db.execute("""SELECT edge_id,source_id,target_id,relation,
            json_array_length(lineage) AS fact_count FROM map_edge
            WHERE relation IN ('EXECUTED_NEXT','OBSERVED_NEXT_PC') ORDER BY relation,edge_id"""))
    except sqlite3.DatabaseError as exc:
        raise ValueError("STOP_2B_RUNTIME_LINEAGE_UNREADABLE") from exc
    nodes = {r["node_id"]: dict(r) for r in db.execute(
        "SELECT node_id,kind,node_key,scope,status,body FROM map_node")}
    range_by_id = {str(r["range_node_id"]): r for r in export.get("ranges", [])}
    if len(range_by_id) != 330 or len(range_edges) != 330:
        raise ValueError("STOP_2B_RUNTIME_RANGE_DEDUP_MISMATCH")
    source_to_object: dict[str, str] = {}
    summed_occurrences = 0
    for edge in range_edges:
        row = range_by_id.get(str(edge["target_id"]))
        source_node = nodes.get(str(edge["source_id"]))
        target_node = nodes.get(str(edge["target_id"]))
        if row is None or source_node is None or target_node is None or \
                source_node["kind"] != "M68K_INSTRUCTION" or target_node["kind"] != "ROM_INSTRUCTION_RANGE":
            raise ValueError("STOP_2B_RUNTIME_RANGE_IDENTITY_MISMATCH")
        start, end = int(row["start_offset"]), int(row["end_offset_exclusive"])
        data = bytes.fromhex(str(row["bytes_hex"]))
        if start < 0 or end <= start or end > len(rom) or len(data) != end - start or \
                rom[start:end] != data or hashlib.sha256(data).hexdigest() != row["bytes_sha256"] or \
                int(row["opcode"]) != int.from_bytes(data[:2], "big") or \
                int(edge["fact_count"]) != int(row["evidence_count"]):
            raise ValueError("STOP_2B_RUNTIME_BYTES_OR_OCCURRENCES_MISMATCH")
        start_pc, opcode_text = str(source_node["node_key"]).split(":", 1)
        if int(start_pc, 16) != start or int(opcode_text, 16) != int(row["opcode"]):
            raise ValueError("STOP_2B_RUNTIME_PC_IDENTITY_MISMATCH")
        count = int(row["evidence_count"])
        oid = _add_object(bundle, ROM_SHA, start, end, "M68K_INSTRUCTION",
                          sum(max(0, min(end, hi) - max(start, lo)) for lo, hi in
                              _owned_runs_from_emission(bundle)))
        attrs = {"opcode": int(row["opcode"]), "length": end - start,
                 "bytes_sha256": str(row["bytes_sha256"]),
                 "source_owned_bytes": sum(max(0, min(end, hi) - max(start, lo))
                    for lo, hi in _owned_runs_from_emission(bundle))}
        object_row = next(r for r in bundle["rom_object"] if r["object_id"] == oid)
        object_row["attributes_json"] = json.dumps(attrs, sort_keys=True, separators=(",", ":"))
        cid = _claim(bundle, oid, "EXECUTED_FROM_ROM", True, "OBSERVED_RUNTIME")
        _evidence(bundle, "CLAIM", cid, export_sha, "RUNTIME_INSTRUCTION_RANGE", 1,
                  {"range_node_id": str(row["range_node_id"]), "start": start, "end": end})
        _evidence(bundle, "CLAIM", cid, session_sha, "RUNTIME_INSTRUCTION_OCCURRENCE", count,
                  {"table": "map_edge", "edge_id": str(edge["edge_id"]), "run_id": run_id})
        source_to_object[str(edge["source_id"])] = oid
        summed_occurrences += count
    if summed_occurrences != expected["instruction_occurrences"]:
        raise ValueError("STOP_2B_RUNTIME_OCCURRENCE_COUNT_MISMATCH")
    next_count = terminal_count = next_relations = 0
    excluded_event_edges = excluded_event_occurrences = 0
    for edge in flow_edges:
        source_id, target_id = str(edge["source_id"]), str(edge["target_id"])
        source_object = source_to_object.get(source_id)
        source_node, target_node = nodes.get(source_id), nodes.get(target_id)
        count = int(edge["fact_count"] or 0)
        if source_node is None or target_node is None or count <= 0:
            raise ValueError("STOP_2B_RUNTIME_RELATION_ENDPOINT_MISSING")
        relation_type = str(edge["relation"])
        target_object = target_address = None
        if relation_type == "EXECUTED_NEXT":
            if source_node["kind"] != "M68K_INSTRUCTION" or target_node["kind"] != "M68K_INSTRUCTION":
                if "EXCEPTION_EVENT" not in source_node["kind"] + target_node["kind"]:
                    raise ValueError("STOP_2B_EXECUTED_NEXT_ENDPOINT_TYPE_INVALID")
                excluded_event_edges += 1
                excluded_event_occurrences += count
                continue
            if source_object is None:
                raise ValueError("STOP_2B_EXECUTED_NEXT_SOURCE_UNCAPTURED")
            target_object = source_to_object.get(target_id)
            if target_object is None:
                raise ValueError("STOP_2B_EXECUTED_NEXT_UNCAPTURED_TARGET")
            next_count += count
            next_relations += 1
        else:
            if source_object is None or source_node["kind"] != "M68K_INSTRUCTION" or \
                    target_node["kind"] != "M68K_TARGET_ADDRESS":
                raise ValueError("STOP_2B_TERMINAL_TARGET_PROMOTED")
            body = json.loads(target_node["body"])
            target_address = int(body["attributes"]["raw_next_pc"])
            terminal_count += count
        rid = hashlib.sha256(json.dumps({"relation_type": relation_type,
            "source_object_id": source_object, "target_object_id": target_object,
            "target_address": target_address, "status": "OBSERVED_RUNTIME"},
            sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        relation_id = "relation:" + rid
        bundle["relation"].append({"relation_id": relation_id, "relation_type": relation_type,
            "source_object_id": source_object, "target_object_id": target_object,
            "target_address": target_address, "status": "OBSERVED_RUNTIME",
            "attributes_json": "{}"})
        _evidence(bundle, "RELATION", relation_id, session_sha,
                  "RUNTIME_NEXT_OCCURRENCE" if relation_type == "EXECUTED_NEXT" else "RUNTIME_TERMINAL_FACT",
                  count, {"table": "map_edge", "edge_id": str(edge["edge_id"]), "run_id": run_id})
    if next_count != 195794 or next_relations != 338 or \
            excluded_event_edges != 47 or excluded_event_occurrences != 1726 or \
            terminal_count != expected["terminal_next_pc_facts"]:
        raise ValueError("STOP_2B_RUNTIME_TERMINAL_ACCOUNTING_MISMATCH")
    db.close()
    return {"instruction_objects": len(range_edges), "instruction_occurrences": summed_occurrences,
            "executed_next_relations": next_relations,
            "executed_next_occurrences": next_count,
            "excluded_exception_flow_edges": excluded_event_edges,
            "excluded_exception_flow_occurrences": excluded_event_occurrences,
            "terminal_relation_objects": sum(1 for r in flow_edges if r["relation"] == "OBSERVED_NEXT_PC"),
            "terminal_facts": terminal_count}


def _owned_runs_from_emission(bundle: dict[str, list[dict[str, Any]]]) -> list[tuple[int, int]]:
    return [(int(e["start"]), int(e["end"])) for e in bundle["emission"] if int(e["source_owned"])]
