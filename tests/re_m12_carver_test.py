"""Regression tests for the deterministic M12 Stage 1 Carver."""

import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT / "src/tools"))
from m12_carver import IntervalDB, ROM_END


def manifest():
    entries = [
        {"start": 0, "end": 0x100, "kind": "CODE_VERIFIED", "classification": "CODE_VERIFIED",
         "confidence": "CONFIRMED", "source": "AUTO", "emitted_artifact_type": "asm"},
        {"start": 0x100, "end": 0x200, "kind": "UNKNOWN", "classification": "UNKNOWN",
         "confidence": "ROM_HASH_VERIFIED", "source": "canonical_local_rom"},
        {"start": 0x200, "end": 0x300, "kind": "STRUCTURED_DATA_CONFIRMED",
         "classification": "TABLE", "confidence": "CONFIRMED", "source": "AUTO"},
        {"start": 0x300, "end": ROM_END, "kind": "UNKNOWN", "classification": "UNKNOWN",
         "confidence": "ROM_HASH_VERIFIED", "source": "canonical_local_rom"},
    ]
    return {"schema": "oasis.full-rom-split.v1", "start": 0, "end": ROM_END,
            "rom_size": ROM_END, "rom_sha256": "fixture",
            "entries": entries, "metrics": {"SOURCE_OWNED_BYTES": 0x200}}


def test_manifest_import_and_required_fields():
    db = IntervalDB.from_manifest(manifest(), "fixture.json")
    assert sum(item["end"] - item["start"] for item in db.ranges) == ROM_END
    assert db.source_owned_bytes() == 0x200
    required = {"id", "start", "end", "classification", "confidence", "evidence", "conflicts",
                "parser", "consumer", "destination", "discovered_by", "provenance_parents",
                "provenance_children", "promotion_transaction", "source_manifest_entry"}
    assert required <= set(db.ranges[0])


def test_evidence_campaigns_conflicts_and_no_promotion():
    db = IntervalDB.from_manifest(manifest())
    before = [(r["start"], r["end"], r["classification"]) for r in db.ranges]
    db.ingest({"adapter": "pointer_resource_table", "records": [
        {"id": "candidate-a", "start": 0x100, "end": 0x140, "confidence": "CANDIDATE",
         "consumer": "table-parser", "pointer_table": "table-a", "targets": [0x110]},
        {"id": "candidate-b", "start": 0x140, "end": 0x180, "confidence": "CANDIDATE",
         "consumer": "table-parser", "pointer_table": "table-a", "runtime": True},
    ]}, "fixture-pointer.json")
    db.ingest({"adapter": "runtime_pc_read", "records": [
        {"id": "cycle-a", "start": 0x300, "end": 0x310, "type": "RUNTIME_READ",
         "runtime_reader": "reader-a"},
    ], "edges": [{"source": "cycle-a", "target": "reader-a", "type": "OBSERVES"},
                 {"source": "reader-a", "target": "cycle-a", "type": "FEEDS"}]}, "fixture-runtime.json")
    gaps, campaigns = db.gap_report()
    after = [(r["start"], r["end"], r["classification"]) for r in db.ranges]
    assert before == after
    assert db.source_owned_bytes() == 0x200
    assert gaps[0]["pointers_xrefs"] and gaps[1]["runtime_reads_executions"]
    assert campaigns[0]["contexts"]
    assert db.edges and any(edge["source"] == "reader-a" and edge["target"] == "cycle-a"
                            for edge in db.edges.values())


def test_non_confirmed_parent_blocks_child_candidate():
    db = IntervalDB.from_manifest(manifest())
    db.ingest({"adapter": "structured_record_promoter", "records": [
        {"id": "child", "start": 0x100, "end": 0x110, "parents": ["range:0001:000100:000200"]},
    ]}, "fixture-child.json")
    assert db.conflicts
    assert all(edge["type"] != "DERIVES_CANDIDATE" for edge in db.edges.values())


def test_non_confirmed_explicit_candidate_edge_is_blocked():
    db = IntervalDB.from_manifest(manifest())
    db.ingest({"adapter": "structured_record_promoter", "records": [
        {"id": "child-edge", "start": 0x100, "end": 0x110,
         "confidence": "CANDIDATE"},
    ], "edges": [{"source": "range:0001:000100:000200", "target": "child-edge",
                   "type": "DERIVES_CANDIDATE", "evidence": ["child-edge"]}]},
              "fixture-edge.json")
    assert any(conflict["type"] == "NON_CONFIRMED_PROVENANCE_PARENT"
               for conflict in db.conflicts.values())
    assert all(edge["type"] != "DERIVES_CANDIDATE" for edge in db.edges.values())


def test_decoder_only_hits_are_candidates_without_false_closed_conflicts():
    db = IntervalDB.from_manifest(manifest())
    db.ingest({"adapter": "graphics_decoder_resource_scan", "records": [
        {"id": "closed-hit", "start": 0x20, "end": 0x30},
        {"id": "boundary-hit", "start": 0xF0, "end": 0x110},
    ]}, "fixture-graphics.json")
    assert db.evidence["closed-hit"]["confidence"] == "CANDIDATE"
    assert db.evidence["closed-hit"]["structural_format"] == "graphics_decoder_candidate"
    assert any(conflict["type"] == "CANDIDATE_OVERLAPS_CONFIRMED"
               for conflict in db.conflicts.values())


def test_deterministic_serialization_and_manifest_rejection():
    first = IntervalDB.from_manifest(manifest())
    second = IntervalDB.from_manifest(manifest())
    payload_a = json.dumps(first.interval_db(), sort_keys=True, separators=(",", ":"))
    payload_b = json.dumps(second.interval_db(), sort_keys=True, separators=(",", ":"))
    assert payload_a == payload_b
    invalid = manifest()
    invalid["entries"][1]["start"] += 1
    try:
        IntervalDB.from_manifest(invalid)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid manifest was accepted")


if __name__ == "__main__":
    test_manifest_import_and_required_fields()
    test_evidence_campaigns_conflicts_and_no_promotion()
    test_non_confirmed_parent_blocks_child_candidate()
    test_non_confirmed_explicit_candidate_edge_is_blocked()
    test_decoder_only_hits_are_candidates_without_false_closed_conflicts()
    test_deterministic_serialization_and_manifest_rejection()
    print("M12 Carver helper tests passed")
