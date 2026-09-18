"""Build the compact 2D ROM knowledge map from accepted local evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

_TOOL_ROOT = str(Path(__file__).resolve().parents[1])
if _TOOL_ROOT not in sys.path:
    sys.path.insert(0, _TOOL_ROOT)

try:
    from .identity import ROM_SHA, ROM_SIZE
    from .rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS, canonical, sha256_bytes
    from .rom_knowledge_sources import (manifest_rows, read_2b, reconcile_manifests,
        import_carver_hypotheses, sha256_file)
    from .rom_knowledge_audit import audit_database
except ImportError:
    from identity import ROM_SHA, ROM_SIZE
    from rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS, canonical, sha256_bytes
    from rom_knowledge_sources import (manifest_rows, read_2b, reconcile_manifests,
        import_carver_hypotheses, sha256_file)
    from rom_knowledge_audit import audit_database


def _root_default() -> Path:
    return Path(__file__).resolve().parents[3]


def _paths(root: Path) -> dict[str, Path]:
    return {
        "rom": root / "build/m12-auto61-child-tables-c/materialized/rebuilt.rom",
        "AUTO60": root / "build/m12-auto60-contiguous-islands-a/materialized/manifest.json",
        "GFX2": root / "build/m12-gfx2-caller-closure-d/materialized/manifest.json",
        "GFXMAX": root / "build/m12-gfxmax-screen-descriptor-candidate-a/materialized/manifest.json",
        "AUTO61": root / "build/m12-auto61-child-tables-c/materialized/manifest.json",
        "GFXMAX_ROOT_C": root / "build/m12-gfxmax-screen-root-c/materialized/manifest.json",
        "2B_REPORT": root / "docs/reports/THOR_M12_ROM_RANGE_LINKAGE_2B.json",
        "2B_EXPORT": root / "build/thor-evidence/live-forward-rom-link-2b/recovered-runtime/rom-link-evidence/rom_execution_ranges.json",
        "2B_SESSION": root / "build/thor-evidence/live-forward-rom-link-2b/recovered-runtime/session-rom-link.sqlite",
        "CARVER_REPORT": root / "build/m12-carver-m12c5-format-reconstruction-e/format_reconstruction_report.json",
        "CARVER_INTERVAL": root / "build/m12-carver-m12c5-format-reconstruction-e/interval_db.json",
        "db": root / "build/thor-evidence/canonical-rom-knowledge-2d/knowledge.sqlite",
        "receipt": root / "docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json",
        "report": root / "docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.md",
    }


def _args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=_root_default())
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--db", type=Path)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--report", type=Path)
    source_args = [("AUTO60", "auto60"), ("GFX2", "gfx2"), ("GFXMAX", "gfxmax"),
        ("AUTO61", "auto61"), ("GFXMAX_ROOT_C", "gfxmax_root_c"),
        ("2B_REPORT", "two_b_report"), ("2B_EXPORT", "two_b_export"),
        ("2B_SESSION", "two_b_session"), ("CARVER_REPORT", "carver_report"),
        ("CARVER_INTERVAL", "carver_interval")]
    for key, dest in source_args:
        parser.add_argument("--" + key.lower().replace("_", "-"), dest=dest, type=Path)
    args = parser.parse_args()
    defaults = _paths(args.root.resolve())
    for key, attr in (("rom", "rom"), ("db", "db"), ("receipt", "receipt"), ("report", "report")):
        if getattr(args, attr) is None:
            setattr(args, attr, defaults[key])
    for key, attr in source_args:
        if getattr(args, attr) is None:
            setattr(args, attr, defaults[key])
    return args


def _artifact(bundle: dict[str, list[dict[str, Any]]], path: Path,
              checkpoint: str, name: str, artifact_type: str) -> str:
    sha = sha256_file(path)
    bundle["source_artifact"].append({"source_sha256": sha, "checkpoint": checkpoint,
        "artifact_name": name, "artifact_type": artifact_type})
    return sha


def _bundle() -> dict[str, list[dict[str, Any]]]:
    return {name: [] for name in TABLE_COLUMNS}


def _import(args: argparse.Namespace) -> tuple[dict[str, Any], dict[str, Any]]:
    source_paths = {name: getattr(args, name.lower()) for name in
                    ("AUTO60", "GFX2", "GFXMAX", "AUTO61", "GFXMAX_ROOT_C")}
    rom = args.rom.read_bytes()
    if len(rom) != ROM_SIZE or sha256_bytes(rom) != ROM_SHA:
        raise ValueError("STOP_ROM_IDENTITY_MISMATCH")
    reconciliation, manifests = reconcile_manifests(source_paths, rom)
    final_manifest, final_manifest_sha = manifests["AUTO61"]
    report2b = json.loads(args.two_b_report.read_text(encoding="utf-8"))
    execution, export_sha = (json.loads(args.two_b_export.read_text(encoding="utf-8")),
                             sha256_file(args.two_b_export))
    if report2b.get("checkpoint") != "M12-ROM-RANGE-LINKAGE-2B" or \
            report2b.get("status") != "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" or \
            report2b.get("rom_projection", {}).get("rom_sha256") != ROM_SHA or \
            int(report2b.get("runtime", {}).get("run_id", -1)) != 1789714283:
        raise ValueError("STOP_2B_REPORT_IDENTITY_MISMATCH")
    carver_report = json.loads(args.carver_report.read_text(encoding="utf-8"))
    carver_interval = json.loads(args.carver_interval.read_text(encoding="utf-8"))
    if carver_interval.get("schema") != "oasis.m68k.m12-carver.interval-db.v1" or \
            carver_interval.get("rom_sha256") != ROM_SHA or len(carver_interval.get("conflicts", [])) != 0:
        raise ValueError("STOP_CARVER_INTERVAL_SOURCE_INVALID")

    bundle = _bundle()
    source_shas: dict[str, str] = {}
    source_shas["canonical_rom"] = _artifact(bundle, args.rom, "M12-AUTO61",
                                              "canonical-rom-verified-by-sha256", "ROM_IDENTITY")
    for name in ("AUTO60", "GFX2", "GFXMAX", "AUTO61", "GFXMAX_ROOT_C"):
        source_shas[name] = _artifact(bundle, source_paths[name], name,
                                      "materialized/manifest.json", "SOURCE_OWNERSHIP_MANIFEST")
    source_shas["2b_report"] = _artifact(bundle, args.two_b_report, "M12-ROM-RANGE-LINKAGE-2B",
                                          "checkpoint-receipt.json", "CHECKPOINT_RECEIPT")
    source_shas["2b_export"] = _artifact(bundle, args.two_b_export, "M12-ROM-RANGE-LINKAGE-2B",
                                          "rom_execution_ranges.json", "COMPACT_RANGE_EXPORT")
    source_shas["2b_session"] = _artifact(bundle, args.two_b_session, "M12-ROM-RANGE-LINKAGE-2B",
                                           "session-rom-link.sqlite", "LOCAL_EVIDENCE_DATABASE")
    raw_flow = args.two_b_session.parent / "rom-link-evidence/flow-v1-records.bin"
    segment_index = args.two_b_session.parent / "rom-link-evidence/flow-v1-segments.jsonl"
    source_shas["2b_flow"] = _artifact(bundle, raw_flow, "M12-ROM-RANGE-LINKAGE-2B",
                                        "flow-v1-records.bin", "LOCAL_RAW_EVIDENCE")
    source_shas["2b_segments"] = _artifact(bundle, segment_index, "M12-ROM-RANGE-LINKAGE-2B",
                                            "flow-v1-segments.jsonl", "LOCAL_RAW_EVIDENCE_INDEX")
    source_shas["carver_report"] = _artifact(bundle, args.carver_report, "M12-CARVER-5",
                                               "format_reconstruction_report.json", "HYPOTHESIS_REPORT")
    source_shas["carver_interval"] = _artifact(bundle, args.carver_interval, "M12-CARVER-5",
                                                "interval_db.json", "LOCAL_EVIDENCE_DATABASE_EXPORT")

    manifest_rows(bundle, final_manifest, final_manifest_sha)
    ownership_runs = [(int(e["start"]), int(e["end"])) for e in final_manifest["entries"] if
                      e.get("kind") not in {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"} and
                      str(e.get("confidence", "")).upper() not in {"PROBABLE", "CANDIDATE", "UNVERIFIED"}]
    hypothesis_count = import_carver_hypotheses(bundle, carver_report,
        source_shas["carver_report"], source_shas["carver_interval"], ownership_runs)
    runtime = read_2b(bundle, rom, execution, source_shas["2b_export"], args.two_b_session,
                      source_shas["2b_session"], str(report2b["runtime"]["run_id"]))
    source_keys = {"rom_sha256": ROM_SHA, "manifest_sha256": final_manifest_sha,
                   "2b_export_sha256": source_shas["2b_export"],
                   "2b_session_sha256": source_shas["2b_session"],
                   "carver_report_sha256": source_shas["carver_report"]}
    input_hash = sha256_bytes(canonical({"source_keys": source_keys,
        "reconciliation": reconciliation,
        "records": {k: len(v) for k, v in bundle.items()}}).encode("utf-8"))
    bundle["map_import"].append({"import_key": "M12-CANONICAL-ROM-KNOWLEDGE-MAP-2D",
                                  "input_hash": input_hash})
    return {"bundle": bundle, "reconciliation": reconciliation, "runtime": runtime,
            "hypothesis_count": hypothesis_count, "source_keys": source_keys,
            "input_hash": input_hash, "rom": rom, "manifest_path": args.auto61,
            "execution_path": args.two_b_export, "session_path": args.two_b_session,
            "carver_report_path": args.carver_report, "carver_interval_path": args.carver_interval}, source_keys


def _write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, sort_keys=True, separators=(",", ":"),
                                ensure_ascii=True) + "\n",
                    encoding="utf-8", newline="\n")


def _markdown(receipt: dict[str, Any]) -> str:
    m, rec, hashes = receipt["metrics"], receipt["ownership_reconciliation"], receipt["hashes"]
    lines = [
        "# M12 Canonical ROM Knowledge Map 2D", "",
        f"Status: `{receipt['status']}`.", "",
        "The map imports the accepted AUTO61 ownership manifest, exact 2B runtime linkage, and",
        "Carver-5 format candidates as hypotheses. It keeps unknown classifications, runtime",
        "observations, source ownership, and emission intervals as separate records. No new runtime",
        "campaign or instrumentation was used. Raw FLOW/session data stays under ignored `build/`.", "",
        "## SOURCE_OWNED reconciliation", "",
        "AUTO60 and AUTO61 are different accepted checkpoints, not competing snapshots of the same",
        "state. Exact intervening promotions explain the full delta. The old Carver-1..5 corpus is",
        "still a valid historical snapshot of AUTO60; it is stale as the current ownership base.", "",
        "| Checkpoint | SOURCE_OWNED bytes | Maximal intervals | Manifest SHA-256 |",
        "| --- | ---: | ---: | --- |",
    ]
    for source in rec["sources"]:
        lines.append(f"| {source['name']} | {source['source_owned_bytes']:,} | {source['owned_interval_count']} | `{source['source_sha256']}` |")
    lines.extend(["", f"The AUTO60→AUTO61 delta is **{rec['bytes_delta']:,} bytes** in **{rec['added_range_count']}** new",
        f"disjoint intervals; there are {rec['removed_bytes']:,} AUTO60-only bytes and the net maximal-interval count delta is {rec['maximal_owned_interval_count_delta']}.",
        f"The manifests share {rec['overlap_bytes']:,} owned bytes with identical source kind, classification, and confidence; both endpoint manifests report zero conflict bytes.", "",
        "| Transition | Delta | New intervals |", "| --- | ---: | --- |"])
    for step in rec["steps"]:
        ranges = ", ".join(f"`[{x['start']:#08x},{x['end']:#08x})` ({x['bytes']:,})" for x in step["added_ranges"])
        lines.append(f"| {step['from']} → {step['to']} | +{step['bytes_delta']:,} | {ranges} |")
    stale = rec["stale_root_c"]
    lines.extend(["", f"The older GFX-MAX screen-root-c manifest is rejected as current input: it is missing",
        f"`{stale['missing_bytes']}` bytes from the descriptor-candidate checkpoint, at",
        ", ".join(f"`[{x['start']:#08x},{x['end']:#08x})`" for x in stale["missing_ranges"]) + ".", "",
        "## Canonical map", "",
        f"ROM identity is `{receipt['rom']['sha256']}` ({receipt['rom']['bytes']:,} bytes). The emission partition has",
        f"{m['emission_ranges']:,} contiguous intervals with exact coverage, no gaps or overlaps, and no out-of-bounds rows.",
        f"It contains {m['semantic_objects']:,} canonical objects, {m['unknown_bytes']:,} UNKNOWN bytes, and",
        f"{m['source_owned_bytes']:,} SOURCE_OWNED bytes ({m['source_owned_percent']}%). Runtime observations do not change that ownership.", "",
        "| Measure | Value |", "| --- | ---: |",
        f"| Executed M68K instruction objects | {m['executed_instruction_objects']:,} |",
        f"| Unique executed ROM bytes | {m['executed_unique_rom_bytes']:,} |",
        f"| Runtime occurrences referenced | {m['runtime_occurrences_referenced']:,} |",
        f"| Executed instructions not fully SOURCE_OWNED | {m['executed_not_fully_owned_objects']:,} |",
        f"| SOURCE_OWNED code ranges never observed / bytes | {m['source_owned_code_ranges_unobserved']:,} / {m['source_owned_code_bytes_unobserved']:,} |",
        f"| Typed data / graphics / audio / Z80 objects | {m['typed_data_objects']:,} / {m['graphics_objects']:,} / {m['audio_objects']:,} / {m['z80_objects']:,} |",
        f"| EXECUTED_NEXT unique relations / occurrence refs | {m['relations_by_type'].get('EXECUTED_NEXT',0):,} / {receipt['runtime_import']['executed_next_occurrences']:,} |",
        f"| OBSERVED_NEXT_PC unique relations / terminal facts | {m['relations_by_type'].get('OBSERVED_NEXT_PC',0):,} / {receipt['runtime_import']['terminal_facts']:,} |",
        f"| Exception-event next edges excluded from EXECUTED_NEXT | {receipt['runtime_import']['excluded_exception_flow_edges']:,} / {receipt['runtime_import']['excluded_exception_flow_occurrences']:,} |",
        f"| Carver-5 hypotheses | {m['hypothesis_claims']:,} |",
        f"| Conflicts imported | {m['conflicts']:,} |",
        f"| Evidence references | {m['evidence_refs']:,} |",
        f"| Emission ASM / DATA / ASSET / INCBIN bytes | {m['asm_bytes']:,} / {m['data_bytes']:,} / {m['asset_bytes']:,} / {m['incbin_bytes']:,} |", "",
        "Objects by canonical type: " + ", ".join(
            f"`{kind}` {count:,}" for kind, count in sorted(m["objects_by_type"].items())) + ".", "",
        "Claims by status: " + ", ".join(
            f"`{status}` {count:,}" for status, count in sorted(m["claims_by_status"].items())) + ".", "",
        "Byte coverage by claim status is a union per status; semantic and hypothesis intervals can",
        "overlap, so those status totals are not additive. The receipt also contains exact byte counts",
        "for every classification, source kind, status, and emission type.", "",
        "The Carver-5 report’s 1,015 bounded format candidates remain HYPOTHESIS evidence; duplicate",
        f"ranges share canonical claims (stored unique hypothesis claims: {m['hypothesis_claims']:,}). Its Stage-4",
        "`168` blocker count is not imported as claim conflict: the exact Stage-5 IntervalDB conflict",
        "collection is empty, and the blockers describe unresolved candidate overlaps rather than",
        "contradictory accepted classifications. No `CALLS`, `READS`, `WRITES`, or `POINTS_TO` relation",
        "is inferred. Of 385 2B `EXECUTED_NEXT` graph edges, only 338 have M68K instruction records",
        "at both endpoints and are imported (195,794 occurrence refs); the remaining 47 connect",
        "exception-event records to instructions and are excluded (1,726 occurrence refs). Terminal",
        "facts remain address-only `OBSERVED_NEXT_PC` relations.", "",
        "## Determinism and audit", "",
        f"Structure hash: `{hashes['structure_hash']}`.",
        f"Evidence-index hash: `{hashes['evidence_index_hash']}`.",
        f"Emission hash: `{hashes['emission_hash']}`.",
        f"Combined map hash: `{hashes['map_hash']}`.",
        f"The independent auditor returned `{receipt['independent_audit']['status']}`. Reimport counts and all three component hashes were unchanged.", "",
        "The compact JSON receipt contains the exact interval/object/claim/relation/evidence-reference",
        "index and source SHA-256 values without ROM bytes or raw runtime lineage. The 979 MB 2B",
        "session database, FLOW stream, segment index, and materialized ROM remain local.", "",
        "Validation results are recorded in `docs/WORKLOG.md`; this checkpoint does not modify the",
        "production AUTO67 runner, predecessor logic, Worker/FLOW runtime, or the ownership manifest.", ""])
    return "\n".join(lines)


def main() -> int:
    args = _args()
    for path in (args.rom, args.auto60, args.gfx2, args.gfxmax, args.auto61,
                 args.gfxmax_root_c, args.two_b_report, args.two_b_export, args.two_b_session,
                 args.carver_report, args.carver_interval):
        if not path.is_file():
            raise FileNotFoundError(f"STOP_REQUIRED_SOURCE_MISSING:{path.name}")
    prepared, _ = _import(args)
    store = KnowledgeStore(args.db, ROM_SHA, ROM_SIZE)
    bundle = prepared["bundle"]
    store.db.execute("BEGIN IMMEDIATE")
    for table in TABLE_COLUMNS:
        store.insert_rows(table, bundle[table])
    store.db.commit()
    first_hashes, first_counts = store.hashes(), store.counts()
    store.db.execute("BEGIN IMMEDIATE")
    for table in TABLE_COLUMNS:
        store.insert_rows(table, bundle[table])
    store.db.commit()
    reimport_hashes, reimport_counts = store.hashes(), store.counts()
    idempotence = {"status": "PASS_IDEMPOTENT_REIMPORT" if first_hashes == reimport_hashes and
                   first_counts == reimport_counts else "STOP_NONDETERMINISTIC_CANONICAL_MAP",
                   "first_hashes": first_hashes, "reimport_hashes": reimport_hashes,
                   "first_counts": first_counts, "reimport_counts": reimport_counts,
                   "counts_unchanged": first_counts == reimport_counts}
    if idempotence["status"] != "PASS_IDEMPOTENT_REIMPORT":
        raise ValueError(idempotence["status"])
    raw_flow = args.two_b_session.parent / "rom-link-evidence/flow-v1-records.bin"
    segment_index = args.two_b_session.parent / "rom-link-evidence/flow-v1-segments.jsonl"
    audit = audit_database(args.db, args.rom, args.auto61, args.two_b_export,
        args.two_b_session, args.carver_report, args.carver_interval,
        prepared["reconciliation"], idempotence,
        {name: getattr(args, attr) for name, attr in (("AUTO60", "auto60"),
          ("GFX2", "gfx2"), ("GFXMAX", "gfxmax"), ("AUTO61", "auto61"),
          ("GFXMAX_ROOT_C", "gfxmax_root_c"))},
        [args.rom, args.auto60, args.gfx2, args.gfxmax, args.auto61, args.gfxmax_root_c,
         args.two_b_report, args.two_b_export, args.two_b_session, raw_flow, segment_index,
         args.carver_report, args.carver_interval])
    receipt = store.export(prepared["reconciliation"], audit, idempotence)
    receipt["status"] = "PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1"
    receipt["runtime_import"] = prepared["runtime"]
    receipt["carver_hypothesis_count"] = prepared["hypothesis_count"]
    store.close()
    _write_json(args.receipt, receipt)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(_markdown(receipt), encoding="utf-8", newline="\n")
    print(json.dumps({"status": receipt["status"], "hashes": receipt["hashes"],
                      "metrics": receipt["metrics"], "receipt": args.receipt.as_posix()},
                     indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
