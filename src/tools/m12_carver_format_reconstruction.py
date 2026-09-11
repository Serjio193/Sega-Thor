"""Whole-ROM deterministic format and container reconstruction for M12 Carver."""

from collections import Counter, defaultdict
import hashlib
import json
from math import log2
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from m12_carver import IntervalDB, canonical, parse_int
from m12_carver_bo_decoder import decoder_spans


ROM_END = 0x300000
UNKNOWN_KINDS = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}
STATIC_REPORT_SCHEMA = "oasis.m68k.m12-carver.static-recovery.v1"
REPORT_SCHEMA = "oasis.m68k.m12-carver.format-container-reconstruction.v1"


def _word_values(data, width, endian):
    return [int.from_bytes(data[index:index + width], endian)
            for index in range(0, len(data) - width + 1, width)]


def _top_words(data, width, endian):
    counts = Counter(_word_values(data, width, endian))
    return [{"value": value, "count": count}
            for value, count in sorted(counts.items(), key=lambda item: (-item[1], item[0]))[:8]]


def _max_run(data, value):
    longest = current = 0
    for item in data:
        current = current + 1 if item == value else 0
        longest = max(longest, current)
    return longest


def _entropy(data):
    if not data:
        return 0.0
    counts = Counter(data)
    size = len(data)
    return round(-sum((count / size) * log2(count / size)
                      for count in counts.values()), 6)


def _monotonic_run(values):
    longest = current = 1 if values else 0
    for left, right in zip(values, values[1:]):
        if right >= left:
            current += 1
        else:
            current = 1
        longest = max(longest, current)
    return longest


def _pointer_count(values, start, end):
    return sum(start <= value < end for value in values)


def _offset_shapes(data, start, end):
    result = []
    for width in (2, 4):
        for endian in ("big", "little"):
            values = _word_values(data, width, endian)
            if len(values) < 3:
                continue
            absolute = _pointer_count(values, start, end)
            relative = _pointer_count([start + value for value in values], start, end)
            monotonic = _monotonic_run(values)
            if monotonic >= 3 or absolute >= 3 or relative >= 3:
                result.append({"width": width, "endian": endian,
                               "absolute_targets": absolute,
                               "relative_targets": relative,
                               "monotonic_run": monotonic})
    return sorted(result, key=canonical)


def _repeat_blocks(data):
    result = []
    for width in (8, 16, 32):
        if len(data) < width * 2:
            continue
        counts = Counter(data[index:index + width]
                         for index in range(0, len(data) - width + 1, width))
        repeated = sum(count - 1 for count in counts.values() if count > 1)
        if repeated:
            result.append({"block_size": width, "distinct": len(counts),
                           "repeated_blocks": repeated,
                           "max_count": max(counts.values())})
    return result


def fingerprint(rom, start, end):
    """Return a bounded structural fingerprint; never emits source bytes."""
    data = rom[start:end]
    words_be = _word_values(data, 2, "big")
    words_le = _word_values(data, 2, "little")
    long_be = _word_values(data, 4, "big")
    shape = {
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "alignment": {"even_start": start % 2 == 0, "even_end": end % 2 == 0,
                       "four_start": start % 4 == 0, "four_end": end % 4 == 0},
        "bank": {"start": start // 0x80000, "end": (end - 1) // 0x80000,
                 "crosses": start // 0x80000 != (end - 1) // 0x80000},
        "zero_ratio": round(data.count(0) / len(data), 6) if data else 0.0,
        "ff_ratio": round(data.count(0xFF) / len(data), 6) if data else 0.0,
        "longest_zero_run": _max_run(data, 0),
        "longest_ff_run": _max_run(data, 0xFF),
        "entropy": _entropy(data),
        "top_words_be": _top_words(data, 2, "big"),
        "top_words_le": _top_words(data, 2, "little"),
        "pointer_like_be32": _pointer_count(long_be, start, end),
        "offset_shapes": _offset_shapes(data, start, end),
        "repeated_blocks": _repeat_blocks(data),
        "tail_sentinels": {"zero": data.endswith(b"\x00"),
                           "ff": data.endswith(b"\xFF"),
                           "word_zero": data.endswith(b"\x00\x00"),
                           "word_ff": data.endswith(b"\xFF\xFF")},
    }
    shape["signature"] = hashlib.sha256(canonical({
        key: value for key, value in shape.items() if key not in ("sha256",)
    }).encode("utf-8")).hexdigest()[:20]
    return shape


def boundary_candidates(rom, start, end, shape):
    """Find structural candidates. Candidates are never ownership by themselves."""
    data = rom[start:end]
    candidates = []
    for width in (2, 4):
        if len(data) < width:
            continue
        for endian in ("big", "little"):
            count = int.from_bytes(data[:width], endian)
            for stride in range(1, 257):
                if count >= 2 and width + count * stride == len(data):
                    candidates.append({"kind": "count_times_stride", "width": width,
                                       "count": count, "stride": stride,
                                       "start": start, "end": end,
                                       "exact_boundary": True})
            values = _word_values(data, width, endian)
            if len(values) >= 3 and all(left <= right for left, right in zip(values, values[1:])):
                if values[0] in (0, width) and values[-1] in (len(data), len(data) - width):
                    candidates.append({"kind": "offset_table_difference", "width": width,
                                       "endian": endian, "entries": len(values),
                                       "start": start, "end": end,
                                       "exact_boundary": True})
    if data.endswith(b"\x00\x00") or data.endswith(b"\xFF\xFF"):
        candidates.append({"kind": "sentinel_termination", "start": start, "end": end,
                           "exact_boundary": False})
    if data and (data == b"\x00" * len(data) or data == b"\xFF" * len(data)):
        candidates.append({"kind": "uniform_alignment_padding", "start": start,
                           "end": end, "exact_boundary": False})
    for item in shape["offset_shapes"]:
        if item["absolute_targets"] >= 4 and item["monotonic_run"] >= 4:
            candidates.append({"kind": "monotonic_pointer_family", "start": start,
                               "end": end, "exact_boundary": False,
                               "shape": item})
    unique = {canonical(item): item for item in candidates}
    return [unique[key] for key in sorted(unique)]


def _unknown_entries(manifest):
    return [entry for entry in manifest["entries"] if entry.get("kind") in UNKNOWN_KINDS]


def _static_seed(path):
    if not path:
        return {}
    report = json.loads(Path(path).read_text(encoding="utf-8"))
    if report.get("schema") != STATIC_REPORT_SCHEMA:
        raise ValueError("unsupported static consumer report schema")
    return {item["id"]: item for item in report.get("remaining_unknown", {}).get("ranges", [])}


def _confirmed_hashes(rom, manifest):
    result = defaultdict(list)
    for entry in manifest["entries"]:
        if entry.get("kind") in UNKNOWN_KINDS:
            continue
        start, end = parse_int(entry["start"]), parse_int(entry["end"])
        result[(end - start, hashlib.sha256(rom[start:end]).hexdigest())].append(
            {"start": start, "end": end, "kind": entry.get("kind")})
    return result


def _blocker_counts(items):
    counts = defaultdict(lambda: {"ranges": 0, "bytes": 0})
    for item in items:
        bucket = counts[item["blocker"]]
        bucket["ranges"] += 1
        bucket["bytes"] += item["size"]
    return {key: counts[key] for key in sorted(counts)}


def _write_json(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2, ensure_ascii=True) + "\n",
                    encoding="utf-8")


def run(manifest_path, rom_path, output, static_report=None):
    manifest_path = Path(manifest_path).resolve()
    rom_path = Path(rom_path).resolve()
    output = Path(output).resolve()
    if output.exists():
        raise ValueError("output directory must not already exist")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    rom = rom_path.read_bytes()
    if len(rom) != ROM_END:
        raise ValueError("canonical ROM must be exactly 0x300000 bytes")
    actual_sha = hashlib.sha256(rom).hexdigest()
    if manifest.get("rom_sha256") and manifest["rom_sha256"] != actual_sha:
        raise ValueError("manifest and canonical ROM hashes differ")
    unknown = _unknown_entries(manifest)
    static = _static_seed(static_report)
    db = IntervalDB.from_manifest(manifest, manifest_path)
    confirmed_hashes = _confirmed_hashes(rom, manifest)
    records = []
    candidate_records = []
    duplicate_records = []
    blocker_records = []
    for entry in unknown:
        start, end = parse_int(entry["start"]), parse_int(entry["end"])
        rid = next(item["id"] for item in db.ranges
                   if item["start"] == start and item["end"] == end)
        shape = fingerprint(rom, start, end)
        decoder = decoder_spans(rom, start, end)
        static_item = static.get(rid, {})
        structural = {
            "range_id": rid, "start": start, "end": end, "size": end - start,
            "fingerprint": shape,
            "left_neighbor": next((item.get("kind") for item in manifest["entries"]
                                    if parse_int(item["end"]) == start), None),
            "right_neighbor": next((item.get("kind") for item in manifest["entries"]
                                     if parse_int(item["start"]) == end), None),
            "static_references": static_item.get("static_references", 0),
            "static_family": static_item.get("family_id"),
            "decoder": decoder,
        }
        records.append(structural)
        db.add_evidence({"id": f"evidence:format:{start:06X}:{end:06X}",
                         "type": "STRUCTURAL_FINGERPRINT", "producer": "m12_carver_format_scanner",
                         "confidence": "EVIDENCE_ONLY", "start": start, "end": end,
                         "details": {"fingerprint": shape, "static_family": static_item.get("family_id")},
                         "resource_block": static_item.get("family_id")})
        if static_item.get("static_references"):
            db.add_evidence({"id": f"evidence:format-static:{start:06X}:{end:06X}",
                             "type": "STATIC_CONSUMER_CONTEXT", "producer": "m12_carver_static_seed",
                             "confidence": "CONFIRMED", "start": start, "end": end,
                             "consumer": static_item.get("family_id"),
                             "details": {"static_references": static_item["static_references"],
                                         "prior_blocker": static_item.get("blocker")}})
        candidates = boundary_candidates(rom, start, end, shape)
        for hit in decoder["spans"]:
            candidate_records.append({"kind": "decoder_eos", "range_id": rid,
                                      "start": hit["start"], "end": hit["end"],
                                      "consumed": hit["consumed"],
                                      "output_size": hit["output_size"], "mode": hit["mode"],
                                      "exact_boundary": True, "promotable": False,
                                      "rejection": "decoder validity is not an ownership contract"})
            db.add_evidence({"id": f"evidence:decoder:{hit['start']:06X}:"
                                    f"{hit['end']:06X}:{hit['output_size']}:{hit['mode']}",
                             "type": "DECODER_GRAMMAR_HIT", "producer": "m12_carver_bo_decoder_scan",
                             "confidence": "CANDIDATE", "start": hit["start"], "end": hit["end"],
                             "parser": "existing_bo_graphics_decompressor",
                             "structural_format": "bo_graphics_stream", "details": hit})
        for candidate in candidates:
            candidate = dict(candidate)
            candidate["range_id"] = rid
            candidate["promotable"] = False
            candidate["rejection"] = "no independently proven reusable sibling grammar"
            candidate_records.append(candidate)
            candidate_id = hashlib.sha256(canonical(candidate).encode("utf-8")).hexdigest()[:12]
            db.add_evidence({"id": f"evidence:format-candidate:{start:06X}:{end:06X}:"
                                    f"{candidate['kind']}:{candidate_id}",
                             "type": "STRUCTURAL_FORMAT_CANDIDATE", "producer": "m12_carver_format_scanner",
                             "confidence": "CANDIDATE", "start": start, "end": end,
                             "parser": static_item.get("family_id"),
                             "structural_format": candidate["kind"], "details": candidate})
        key = (end - start, shape["sha256"])
        siblings = confirmed_hashes.get(key, [])
        if siblings:
            duplicate = {"range_id": rid, "start": start, "end": end,
                         "siblings": siblings, "promotable": False,
                         "rejection": "duplicate bytes lack a closed typed source representation"}
            duplicate_records.append(duplicate)
            db.add_evidence({"id": f"evidence:format-duplicate:{start:06X}:{end:06X}",
                             "type": "EXACT_DUPLICATE_PROVENANCE", "producer": "m12_carver_format_scanner",
                             "confidence": "CANDIDATE", "start": start, "end": end,
                             "details": duplicate})
        blocker = static_item.get("blocker", "G")
        status = "FORMAT_HEURISTIC_ONLY" if candidates else "NO_CLOSED_GRAMMAR"
        if static_item.get("blocker") == "F":
            status = "CANDIDATE_OVERLAP_REQUIRES_SPLIT"
        elif static_item.get("blocker") == "B" and not candidates:
            status = "KNOWN_CONSUMER_UNKNOWN_CONTAINER"
        blocker_records.append({"range_id": rid, "start": start, "end": end,
                                "size": end - start, "blocker": blocker,
                                "structural_status": status,
                                "static_family": static_item.get("family_id"),
                                "static_references": static_item.get("static_references", 0),
                                "candidate_count": len(candidates)})
    fixed = db.fixed_point()
    db_json = db.interval_db()
    report = {
        "schema": REPORT_SCHEMA,
        "deterministic": True,
        "baseline": {"manifest": str(manifest_path), "source_owned_bytes": db.manifest_source_owned_bytes,
                      "source_owned_percent": db.manifest_source_owned_bytes / ROM_END * 100,
                      "unknown_ranges": len(unknown),
                      "unknown_bytes": sum(parse_int(item["end"]) - parse_int(item["start"]) for item in unknown)},
        "final": {"source_owned_bytes": db.source_owned_bytes(),
                  "source_owned_unchanged": db.source_owned_bytes() == db.manifest_source_owned_bytes,
                  "promoted_ranges": 0, "promoted_bytes": 0,
                  "unknown_ranges": len(unknown),
                  "unknown_bytes": sum(item["size"] for item in blocker_records)},
        "structural_fingerprints": records,
        "formats": {"candidate_count": len(candidate_records), "candidates": candidate_records,
                    "promoted": [], "rejected": candidate_records},
        "containers_banks": {"global_clusters": sorted(Counter(item["fingerprint"]["signature"]
                                                                  for item in records).items()),
                              "closed": [], "candidate_ranges": []},
        "tables_grammars": {"reusable_confirmed_grammars": [], "candidate_contracts": sorted(
            Counter(item["kind"] for item in candidate_records).items())},
        "compression": {"decoder_scanned": True,
                         "starts_scanned": sum(item["decoder"]["starts_scanned"] for item in records),
                         "valid_hits": sum(item["decoder"]["valid_hits"] for item in records),
                         "retained_spans": sum(len(item["decoder"]["spans"]) for item in records),
                         "reason": "Existing BO grammar was scanned globally; decoder-only hits remain candidates."},
        "duplicates": {"records": duplicate_records, "promoted_bytes": 0},
        "code_data_separation": {"new_code_ranges": [], "conflicts": [],
                                  "reason": "No executable bytes were claimed by format analysis."},
        "blocker_map": {"by_blocker": _blocker_counts(blocker_records),
                        "ranges": blocker_records,
                        "largest_families": sorted(
                            [{"family": key, "ranges": sum(item["static_family"] == key for item in blocker_records),
                              "bytes": sum(item["size"] for item in blocker_records
                                            if item["static_family"] == key)}
                             for key in sorted({item["static_family"] for item in records
                                                if item["static_family"]})],
                            key=lambda item: (-item["bytes"], item["family"]))[:25]},
        "carver": {"fixed_point": fixed, "evidence_records": len(db.evidence),
                   "provenance_nodes": len(db.nodes), "provenance_edges": len(db.edges),
                   "conflicts": len(db.conflicts), "interval_db_schema": db_json["schema"]},
        "coverage": {"rom_bytes": ROM_END, "source_owned_bytes": db.source_owned_bytes(),
                     "unknown_never_runtime_swept_bytes": sum(item["size"] for item in blocker_records),
                     "runtime_sweep_repeated": False,
                     "note": "This pass consumes the completed Carver-3 runtime result and does not repeat it."},
        "hashes": {"canonical_rom_sha256": actual_sha},
    }
    output.mkdir(parents=True)
    _write_json(output / "interval_db.json", db_json)
    report["hashes"]["interval_db_sha256"] = hashlib.sha256(
        canonical(db_json).encode("utf-8")).hexdigest()
    report["hashes"]["evidence_state_sha256"] = hashlib.sha256(
        canonical({"evidence": db_json["evidence_records"], "nodes": db_json["provenance"]["nodes"],
                   "edges": db_json["provenance"]["edges"], "conflicts": db_json["conflicts"]}).encode("utf-8")).hexdigest()
    report["hashes"]["report_sha256"] = hashlib.sha256(canonical(report).encode("utf-8")).hexdigest()
    _write_json(output / "format_reconstruction_report.json", report)
    return report
