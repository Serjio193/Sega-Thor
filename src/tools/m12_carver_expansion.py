"""Deterministic graph-guided Stage 2 evidence expansion for M12 Carver.

This module consumes existing table and graphics evidence.  It never edits the
manifest and never promotes a candidate.  The raw-resource size rule is
intentionally a hypothesis record: the existing M12 consumer contract does
not prove that field six is a byte length for every table row.
"""

from copy import deepcopy
import hashlib
import json
from pathlib import Path
from collections import defaultdict
import zlib

from m12_carver import IntervalDB, canonical
from re_m12_record_stream_promote import TABLE_COUNT, TABLE_START, TABLE_STRIDE
from re_m12_record_stream_promote import parse_table


RAW_SIZE_ROWS = (4, 5, 6, 7, 8)
RAW_SIZE_MAX = 0x10000
RAW_SIZE_ALIGNMENT = 0x20
EXPANSION_SCHEMA = "oasis.m68k.m12-carver.stage2-expansion.v1"


def manifest_range(db, start, end):
    matches = [item for item in db.ranges
               if item["start"] == start and item["end"] == end]
    if len(matches) != 1:
        raise ValueError(f"confirmed manifest range is not unique: 0x{start:06X}..0x{end:06X}")
    if not matches[0]["source_owned"]:
        raise ValueError("graph seed must be SOURCE_OWNED")
    return matches[0]


def load_census(paths):
    result = {}
    for path in sorted((Path(value).resolve() for value in paths), key=str):
        payload = json.loads(path.read_text())
        rows = payload.get("records", payload.get("streams", []))
        for item in rows:
            if not isinstance(item, dict) or "start" not in item or "end" not in item:
                continue
            start, end = int(item["start"]), int(item["end"])
            key = (start, end)
            previous = result.get(key)
            if previous is not None and canonical(previous) != canonical(item):
                raise ValueError(f"conflicting census record at 0x{start:06X}")
            result[key] = deepcopy(item)
    return result


def census_by_start(census):
    result = {}
    for (start, end), item in sorted(census.items()):
        previous = result.get(start)
        if previous is not None and previous["end"] != end:
            raise ValueError(f"conflicting census boundary at 0x{start:06X}")
        result[start] = {"end": end, "record": item}
    return result


def pointer_records(table, parent_id):
    records = []
    edges = []
    nodes = []
    for item in table["records"]:
        pointer = item["fields"][1]
        if not pointer:
            continue
        address = item["address"] + 4
        evidence_id = f"m12-field1-pointer:{item['index']:02d}"
        records.append({
            "id": evidence_id,
            "start": address,
            "end": address + 4,
            "type": "POINTER_TABLE_FIELD1",
            "producer": "re_m12_record_stream_promote",
            "confidence": "CONFIRMED",
            "parser": "m12_record_table_field1",
            "pointer_table": parent_id,
            "targets": [pointer],
            "source_ref": "existing:99-row-field1-contract",
            "details": {"table_row": item["index"], "target": pointer,
                        "table_start": TABLE_START, "stride": TABLE_STRIDE},
        })
        edges.append({"source": parent_id, "target": evidence_id,
                      "type": "REFERENCES", "evidence": [evidence_id]})
        resource_id = f"resource:{pointer:06X}"
        nodes.append({"id": resource_id, "type": "resource",
                      "label": f"ROM resource 0x{pointer:06X}"})
        edges.append({"source": evidence_id, "target": resource_id,
                      "type": "POINTS_TO", "evidence": [evidence_id]})
    return records, edges, nodes


def raw_size_hypotheses(table, parent_id):
    result = []
    by_row = {item["index"]: item for item in table["records"]}
    for row in RAW_SIZE_ROWS:
        item = by_row.get(row)
        if item is None:
            continue
        pointer = item["fields"][1]
        size = item["fields"][6]
        if not pointer or not (0 < size <= RAW_SIZE_MAX) or size % RAW_SIZE_ALIGNMENT:
            continue
        result.append({
            "id": f"m12-raw-field1-size-hypothesis:{row:02d}",
            "start": pointer,
            "end": pointer + size,
            "type": "RAW_FIELD1_SIZE_HYPOTHESIS",
            "producer": "re_m12_record_stream_promote",
            "confidence": "CANDIDATE",
            "parser": "m12_record_table_field1",
            "pointer_table": parent_id,
            "consumer": None,
            "provenance_parents": [parent_id],
            "source_ref": "existing:field6-shape-only",
            "details": {
                "table_row": row,
                "size_field_offset": 24,
                "size_field_value": size,
                "boundary_status": "HYPOTHESIS_ONLY",
                "rejection_reason": "field6 has no closed universal raw-resource consumer contract",
            },
        })
    return result


def graphics_closure(table, census, db):
    by_start = census_by_start(census)
    full = []
    partial = []
    absent = []
    for item in table["records"]:
        pointer = item["fields"][1]
        if not pointer:
            continue
        hit = by_start.get(pointer)
        if hit is None:
            absent.append(item["index"])
            continue
        end = hit["end"]
        overlaps = db.find_ranges(pointer, end)
        if any(r["source_owned"] for r in overlaps):
            wholly_owned = any(r["source_owned"] and r["start"] <= pointer and end <= r["end"]
                               for r in overlaps)
            (full if wholly_owned else partial).append({
                "row": item["index"], "start": pointer, "end": end,
                "decompressed_bytes": hit["record"].get("decompressed_bytes"),
            })
        else:
            full.append({"row": item["index"], "start": pointer, "end": end,
                         "decompressed_bytes": hit["record"].get("decompressed_bytes")})
    return {"census_matches": len(full) + len(partial), "already_closed": full,
            "blocked_by_partial_overlap": partial, "no_boundary": absent}


def compression_family_report(report):
    campaigns = []
    for gap in report["gaps"]:
        hits = [item for item in gap["evidence"]
                if item["type"] == "graphics_decoder_resource_scan"]
        if not hits:
            continue
        spans = [(max(gap["start"], item["start"]),
                  min(gap["end"], item["end"])) for item in hits]
        spans = [span for span in spans if span[0] < span[1]]
        campaigns.append({
            "gap_id": gap["id"],
            "start": gap["start"],
            "end": gap["end"],
            "candidate_hits": len(hits),
            "candidate_span_bytes": sum(end - start for start, end in spans),
            "largest_candidate_bytes": max((end - start for start, end in spans),
                                             default=0),
            "promotion": "REJECTED",
            "reason": "decoder validity lacks confirmed consumer/table boundary",
        })
    campaigns.sort(key=lambda item: (-item["candidate_span_bytes"],
                                     -item["candidate_hits"], item["start"]))
    return {"campaigns": campaigns,
            "candidate_hits": sum(item["candidate_hits"] for item in campaigns),
            "candidate_span_bytes": sum(item["candidate_span_bytes"] for item in campaigns),
            "promotion": "REJECTED_NO_CONSUMER_PROVENANCE"}


def ownership_totals(db):
    totals = defaultdict(int)
    for item in db.ranges:
        if item["source_owned"]:
            totals[item["classification"]] += item["end"] - item["start"]
    return dict(sorted(totals.items()))


def rom_identity(rom):
    return {"size": len(rom), "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rom).hexdigest(),
            "sha256": hashlib.sha256(rom).hexdigest()}


def expand(db, rom, census):
    table = parse_table(rom)
    parent = manifest_range(db, TABLE_START, TABLE_START + TABLE_STRIDE * TABLE_COUNT)
    pointer, edges, nodes = pointer_records(table, parent["id"])
    db.ingest({"adapter": "pointer_resource_table", "records": pointer,
               "edges": edges, "nodes": nodes}, "existing:99-row-field1-graph")
    hypotheses = raw_size_hypotheses(table, parent["id"])
    db.ingest({"adapter": "structured_record_promoter", "records": hypotheses},
              "existing:field6-size-hypotheses")
    closure = graphics_closure(table, census, db)
    return {
        "seed": {"range_id": parent["id"], "start": TABLE_START,
                 "end": TABLE_START + TABLE_STRIDE * TABLE_COUNT,
                 "confirmed": True},
        "table_field1": {"rows": TABLE_COUNT, "nonzero_pointers": len(pointer),
                         "evidence_records": len(pointer),
                         "typed_edges": len(edges)},
        "graphics_decoder_closure": closure,
        "raw_size_hypotheses": {
            "count": len(hypotheses),
            "bytes": sum(item["end"] - item["start"] for item in hypotheses),
            "candidate_ids": [item["id"] for item in hypotheses],
            "promotion": "REJECTED",
            "reason": "no exact field6 raw-resource consumer and boundary contract",
        },
    }


def run(manifest_path, rom_path, evidence_paths, census_paths, output):
    manifest_path = Path(manifest_path).resolve()
    rom_path = Path(rom_path).resolve()
    output = Path(output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    manifest = json.loads(manifest_path.read_text())
    rom = rom_path.read_bytes()
    if len(rom) != int(manifest["rom_size"]):
        raise ValueError("ROM size does not match manifest")
    if hashlib.sha256(rom).hexdigest() != manifest.get("rom_sha256"):
        raise ValueError("ROM SHA-256 does not match manifest")
    db = IntervalDB.from_manifest(manifest, manifest_path)
    for path in sorted((Path(value).resolve() for value in evidence_paths), key=str):
        db.ingest(json.loads(path.read_text()), str(path))
    baseline_owned = db.source_owned_bytes()
    expansion = expand(db, rom, load_census(census_paths))
    report = db.report()
    db_payload = db.interval_db()
    compression = compression_family_report(report)
    expansion.update({
        "schema": EXPANSION_SCHEMA,
        "deterministic": True,
        "baseline_source_owned_bytes": baseline_owned,
        "final_source_owned_bytes": report["source_owned_bytes"],
        "gained_source_owned_bytes": report["source_owned_bytes"] - baseline_owned,
        "baseline_source_owned_percent": 100.0 * baseline_owned / len(rom),
        "final_source_owned_percent": 100.0 * report["source_owned_bytes"] / len(rom),
        "ownership_by_classification": ownership_totals(db),
        "canonical_rom": rom_identity(rom),
        "deterministic_db_sha256": hashlib.sha256(canonical(db_payload).encode()).hexdigest(),
        "remaining_unknown_ranked": sorted(
            report["gaps"], key=lambda item: (-item["size"], item["start"])),
        "compression_family_closure": compression,
        "fixed_point": report["fixed_point"],
        "campaigns_attempted": [
            {"method": "provenance_closure.field1", "gain_bytes": 0,
             "status": "EVIDENCE_ADDED_NO_PROMOTION"},
            {"method": "graphics_decoder_closure.field1", "gain_bytes": 0,
             "status": "ALREADY_CLOSED_OR_PARTIAL_OVERLAP"},
            {"method": "raw_field1_size_hypothesis", "gain_bytes": 0,
             "status": "REJECTED_NO_EXACT_CONSUMER_BOUNDARY"},
            {"method": "compression_family_closure", "gain_bytes": 0,
             "candidate_span_bytes": compression["candidate_span_bytes"],
             "status": "REJECTED_NO_CONSUMER_PROVENANCE"},
        ],
        "stop_reason": "stable_fixed_point_no_safe_growth_ge_16KiB",
        "gap_report": report,
    })
    output.mkdir(parents=True)
    (output / "interval_db.json").write_text(
        json.dumps(db.interval_db(), indent=2, sort_keys=True) + "\n")
    (output / "gap_report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n")
    (output / "expansion_report.json").write_text(
        json.dumps(expansion, indent=2, sort_keys=True) + "\n")
    return expansion
