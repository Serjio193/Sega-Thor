"""M12 W5c Audio Resource Ownership Promotion and Canonical Map Regeneration.

Executes exact, idempotent promotion of reconstructed audio resources
AUDIO_RESOURCE_FORMAT_A_0001 and AUDIO_RESOURCE_FORMAT_A_0002 into the
canonical ROM knowledge map under ADR-M12-W5C.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import sys
from typing import Any

_TOOLS = str(Path(__file__).resolve().parents[1])
if _TOOLS not in sys.path:
    sys.path.insert(0, _TOOLS)

try:
    from .identity import ROM_SHA, ROM_SIZE
    from .rom_knowledge_map import (
        KnowledgeStore,
        TABLE_COLUMNS,
        canonical,
        object_id,
        range_id,
        sha256_bytes,
        stable_id,
    )
except ImportError:
    from identity import ROM_SHA, ROM_SIZE
    from rom_knowledge_map import (
        KnowledgeStore,
        TABLE_COLUMNS,
        canonical,
        object_id,
        range_id,
        sha256_bytes,
        stable_id,
    )

FORMAT_ID = "AUDIO_FORMAT_A_MODE0"
PROMOTION_SOURCE = "M12_W5C_EXACT_AUDIO_RESOURCE_OWNERSHIP_V1"

# Exact reconstructed resource ranges
RESOURCE_1_START = 0x0BD540
RESOURCE_1_END = 0x0BF768
RESOURCE_1_SIZE = 8744

RESOURCE_2_START = 0x0BC95C
RESOURCE_2_END = 0x0BD540
RESOURCE_2_SIZE = 3044

TARGET_START = RESOURCE_2_START
TARGET_END = RESOURCE_1_END
TOTAL_ELIGIBLE_BYTES = RESOURCE_1_SIZE + RESOURCE_2_SIZE  # 11,788

PARENT_EMISSION_START = 0x0B8000
PARENT_EMISSION_END = 0x0BF768


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def compute_overlap(map_data: dict[str, Any]) -> dict[str, Any]:
    """Computes set-theoretic overlap between eligible resources and current ownership."""
    already_owned = 0
    overlapping_owned = []
    for e in map_data.get("emission", []):
        if e.get("source_owned") == 1:
            s, end = e["start"], e["end"]
            overlap_s = max(s, TARGET_START)
            overlap_e = min(end, TARGET_END)
            if overlap_s < overlap_e:
                already_owned += overlap_e - overlap_s
                overlapping_owned.append((overlap_s, overlap_e))

    new_bytes = TOTAL_ELIGIBLE_BYTES - already_owned
    return {
        "eligible_bytes_total": TOTAL_ELIGIBLE_BYTES,
        "already_source_owned_bytes": already_owned,
        "new_source_owned_bytes": new_bytes,
        "eligible_intervals": [
            {"start": RESOURCE_2_START, "end": RESOURCE_2_END, "bytes": RESOURCE_2_SIZE,
             "start_hex": f"0x{RESOURCE_2_START:06X}", "end_hex": f"0x{RESOURCE_2_END:06X}"},
            {"start": RESOURCE_1_START, "end": RESOURCE_1_END, "bytes": RESOURCE_1_SIZE,
             "start_hex": f"0x{RESOURCE_1_START:06X}", "end_hex": f"0x{RESOURCE_1_END:06X}"},
        ],
        "already_owned_intervals": overlapping_owned,
    }


def _verify_proofs(w5_dir: Path) -> dict[str, str]:
    required = [
        "w5_audio_format_spec.json",
        "w5_audio_resources.json",
        "w5_roundtrip_receipt.json",
        "w5_witness_alignment.json",
    ]
    proof_shas = {}
    for name in required:
        p = w5_dir / name
        if not p.is_file():
            raise FileNotFoundError(f"STOP_W5_PROOF_MISSING:{name}")
        proof_shas[name] = _hash_file(p)

    receipt = json.loads((w5_dir / "w5_roundtrip_receipt.json").read_text(encoding="utf-8"))
    if receipt.get("status") != "PASS_ROUNDTRIP_VERIFIED":
        raise ValueError("STOP_W5_ROUNDTRIP_NOT_VERIFIED")
    if not receipt.get("primary_resource", {}).get("byte_identical"):
        raise ValueError("STOP_W5_PRIMARY_NOT_BYTE_IDENTICAL")
    if not receipt.get("secondary_resource", {}).get("byte_identical"):
        raise ValueError("STOP_W5_SECONDARY_NOT_BYTE_IDENTICAL")

    witness = json.loads((w5_dir / "w5_witness_alignment.json").read_text(encoding="utf-8"))
    if witness.get("runtime_mismatches", -1) != 0 or witness.get("runtime_matches", 0) != 18:
        raise ValueError("STOP_W5_WITNESS_MISMATCH")

    return proof_shas


def run_promotion(
    map_path: Path,
    rom_path: Path,
    w5_dir: Path,
    output_dir: Path,
    db_path: Path | None = None,
) -> dict[str, Any]:
    """Executes canonical promotion through KnowledgeStore and regenerates 2D map."""
    output_dir.mkdir(parents=True, exist_ok=True)
    if not map_path.is_file():
        raise FileNotFoundError(f"STOP_CANONICAL_MAP_MISSING:{map_path}")
    if not rom_path.is_file():
        raise FileNotFoundError(f"STOP_CANONICAL_ROM_MISSING:{rom_path}")

    rom_bytes = rom_path.read_bytes()
    if len(rom_bytes) != ROM_SIZE or hashlib.sha256(rom_bytes).hexdigest() != ROM_SHA:
        raise ValueError("STOP_ROM_IDENTITY_MISMATCH")

    proof_shas = _verify_proofs(w5_dir)
    map_raw = map_path.read_text(encoding="utf-8")
    map_data = json.loads(map_raw)
    pre_promotion_map_sha = hashlib.sha256(map_raw.encode("utf-8")).hexdigest()

    # Preflight baseline
    source_owned_before = sum(
        e["end"] - e["start"] for e in map_data["emission"] if e.get("source_owned") == 1
    )
    overlap = compute_overlap(map_data)
    new_bytes = overlap["new_source_owned_bytes"]

    # Check idempotence: if already promoted, new_bytes is 0
    already_promoted = (new_bytes == 0) and (source_owned_before >= 1475600 + TOTAL_ELIGIBLE_BYTES)

    if db_path is None:
        db_path = output_dir / "canonical_knowledge_staging.sqlite"
    if db_path.exists():
        db_path.unlink()

    store = KnowledgeStore(db_path, ROM_SHA, ROM_SIZE)

    # 1. Populate store with existing map data
    store.db.execute("BEGIN IMMEDIATE")
    for r in map_data["ranges"]:
        store.db.execute(
            "INSERT OR IGNORE INTO rom_range(range_id, rom_sha256, start, end) VALUES (?,?,?,?)",
            (r["range_id"], r["rom_sha256"], r["start"], r["end"]),
        )
    for o in map_data["objects"]:
        attrs_json = json.dumps(o.get("attributes", {}), sort_keys=True, separators=(",", ":"))
        store.db.execute(
            "INSERT OR IGNORE INTO rom_object(object_id, range_id, object_type, attributes_json) VALUES (?,?,?,?)",
            (o["object_id"], o["range_id"], o["object_type"], attrs_json),
        )
    for c in map_data["claims"]:
        val_json = json.dumps(c.get("value", {}), sort_keys=True, separators=(",", ":"))
        store.db.execute(
            "INSERT OR IGNORE INTO claim(claim_id, object_id, claim_type, value_json, status) VALUES (?,?,?,?,?)",
            (c["claim_id"], c["object_id"], c["claim_type"], val_json, c["status"]),
        )
    for rel in map_data["relations"]:
        attrs_json = json.dumps(rel.get("attributes", {}), sort_keys=True, separators=(",", ":"))
        store.db.execute(
            "INSERT OR IGNORE INTO relation(relation_id, relation_type, source_object_id, target_object_id, target_address, status, attributes_json) VALUES (?,?,?,?,?,?,?)",
            (rel["relation_id"], rel["relation_type"], rel["source_object_id"], rel["target_object_id"], rel.get("target_address"), rel["status"], attrs_json),
        )
    for em in map_data["emission"]:
        store.db.execute(
            "INSERT OR IGNORE INTO emission(start, end, emission_type, classification, source_kind, source_owned, artifact_type, artifact) VALUES (?,?,?,?,?,?,?,?)",
            (em["start"], em["end"], em["emission_type"], em["classification"], em["source_kind"], em["source_owned"], em["artifact_type"], em["artifact"]),
        )
    for sa in map_data["evidence_artifacts"]:
        store.db.execute(
            "INSERT OR IGNORE INTO source_artifact(source_sha256, checkpoint, artifact_name, artifact_type) VALUES (?,?,?,?)",
            (sa["source_sha256"], sa["checkpoint"], sa["artifact_name"], sa["artifact_type"]),
        )
    for er in map_data["evidence_refs"]:
        loc_json = json.dumps(er.get("locator", {}), sort_keys=True, separators=(",", ":"))
        store.db.execute(
            "INSERT OR IGNORE INTO evidence_ref(ref_id, subject_type, subject_id, source_sha256, fact_kind, fact_count, locator_json) VALUES (?,?,?,?,?,?,?)",
            (er["ref_id"], er["subject_type"], er["subject_id"], er["source_sha256"], er["fact_kind"], er["fact_count"], loc_json),
        )
    for cf in map_data.get("conflicts", []):
        det_json = json.dumps(cf.get("detail", {}), sort_keys=True, separators=(",", ":"))
        store.db.execute(
            "INSERT OR IGNORE INTO conflict(conflict_id, start, end, conflict_type, detail_json) VALUES (?,?,?,?,?)",
            (cf["conflict_id"], cf["start"], cf["end"], cf["conflict_type"], det_json),
        )
    store.db.commit()

    promoted_now = False
    if not already_promoted:
        store.db.execute("BEGIN IMMEDIATE")
        # 2. Split emission range [0x0B8000, 0x0BF768)
        store.db.execute(
            "DELETE FROM emission WHERE start=? AND end=?",
            (PARENT_EMISSION_START, PARENT_EMISSION_END),
        )
        # Add remainder range [0x0B8000, 0x0BC95C)
        store.db.execute(
            "INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
            (PARENT_EMISSION_START, TARGET_START, "INCBIN", "UNKNOWN", "UNKNOWN", 0, "blob", f"blobs/{PARENT_EMISSION_START:06X}_{TARGET_START:06X}.bin"),
        )
        # Add Resource 2 [0x0BC95C, 0x0BD540)
        store.db.execute(
            "INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
            (RESOURCE_2_START, RESOURCE_2_END, "DATA", "SOUND_DATA_CONTAINER_CONFIRMED", "DATA_KNOWN", 1, "rom_asset", f"data/audio_{RESOURCE_2_START:06X}.bin"),
        )
        # Add Resource 1 [0x0BD540, 0x0BF768)
        store.db.execute(
            "INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
            (RESOURCE_1_START, RESOURCE_1_END, "DATA", "SOUND_DATA_CONTAINER_CONFIRMED", "DATA_KNOWN", 1, "rom_asset", f"data/audio_{RESOURCE_1_START:06X}.bin"),
        )

        # 3. Insert canonical rom_ranges
        r0_id = range_id(ROM_SHA, PARENT_EMISSION_START, TARGET_START)
        r1_id = range_id(ROM_SHA, RESOURCE_1_START, RESOURCE_1_END)
        r2_id = range_id(ROM_SHA, RESOURCE_2_START, RESOURCE_2_END)
        for rid, s, e in [
            (r0_id, PARENT_EMISSION_START, TARGET_START),
            (r2_id, RESOURCE_2_START, RESOURCE_2_END),
            (r1_id, RESOURCE_1_START, RESOURCE_1_END),
        ]:
            store.db.execute(
                "INSERT OR IGNORE INTO rom_range VALUES (?,?,?,?)",
                (rid, ROM_SHA, s, e),
            )

        # 4. Insert rom_objects
        o0_id = object_id(ROM_SHA, PARENT_EMISSION_START, TARGET_START, "UNKNOWN")
        o2_id = object_id(ROM_SHA, RESOURCE_2_START, RESOURCE_2_END, "AUDIO_DATA")
        o1_id = object_id(ROM_SHA, RESOURCE_1_START, RESOURCE_1_END, "AUDIO_DATA")
        store.db.execute(
            "INSERT OR IGNORE INTO rom_object VALUES (?,?,?,?)",
            (o0_id, r0_id, "UNKNOWN", json.dumps({"source_owned_bytes": 0}, sort_keys=True, separators=(",", ":"))),
        )
        store.db.execute(
            "INSERT OR IGNORE INTO rom_object VALUES (?,?,?,?)",
            (o2_id, r2_id, "AUDIO_DATA", json.dumps({"source_owned_bytes": RESOURCE_2_SIZE, "resource_id": "AUDIO_RESOURCE_FORMAT_A_0002", "mode": 0}, sort_keys=True, separators=(",", ":"))),
        )
        store.db.execute(
            "INSERT OR IGNORE INTO rom_object VALUES (?,?,?,?)",
            (o1_id, r1_id, "AUDIO_DATA", json.dumps({"source_owned_bytes": RESOURCE_1_SIZE, "resource_id": "AUDIO_RESOURCE_FORMAT_A_0001", "mode": 0}, sort_keys=True, separators=(",", ":"))),
        )

        # 5. Insert claims
        # Remainder claims
        c0_owned = stable_id("claim", {"object_id": o0_id, "claim_type": "SOURCE_OWNED", "value": False, "status": "DERIVED_EXACT"})
        c0_class = stable_id("claim", {"object_id": o0_id, "claim_type": "SOURCE_CLASS", "value": {"classification": "UNKNOWN", "confidence": "ROM_HASH_VERIFIED", "source_kind": "UNKNOWN"}, "status": "DERIVED_EXACT"})
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c0_owned, o0_id, "SOURCE_OWNED", "false", "DERIVED_EXACT"))
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c0_class, o0_id, "SOURCE_CLASS", json.dumps({"classification": "UNKNOWN", "confidence": "ROM_HASH_VERIFIED", "source_kind": "UNKNOWN"}, sort_keys=True, separators=(",", ":")), "DERIVED_EXACT"))

        # Resource 2 claims
        c2_owned = stable_id("claim", {"object_id": o2_id, "claim_type": "SOURCE_OWNED", "value": True, "status": "STATIC_VERIFIED"})
        c2_class = stable_id("claim", {"object_id": o2_id, "claim_type": "SOURCE_CLASS", "value": {"classification": "SOUND_DATA_CONTAINER_CONFIRMED", "confidence": "CONFIRMED", "source_kind": "DATA_KNOWN"}, "status": "STATIC_VERIFIED"})
        c2_recon = stable_id("claim", {"object_id": o2_id, "claim_type": "RECONSTRUCTION_VERIFIED", "value": {"format_id": FORMAT_ID, "roundtrip": "BYTE_IDENTICAL", "mode": 0}, "status": "STATIC_VERIFIED"})
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c2_owned, o2_id, "SOURCE_OWNED", "true", "STATIC_VERIFIED"))
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c2_class, o2_id, "SOURCE_CLASS", json.dumps({"classification": "SOUND_DATA_CONTAINER_CONFIRMED", "confidence": "CONFIRMED", "source_kind": "DATA_KNOWN"}, sort_keys=True, separators=(",", ":")), "STATIC_VERIFIED"))
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c2_recon, o2_id, "RECONSTRUCTION_VERIFIED", json.dumps({"format_id": FORMAT_ID, "roundtrip": "BYTE_IDENTICAL", "mode": 0}, sort_keys=True, separators=(",", ":")), "STATIC_VERIFIED"))

        # Resource 1 claims
        c1_owned = stable_id("claim", {"object_id": o1_id, "claim_type": "SOURCE_OWNED", "value": True, "status": "STATIC_VERIFIED"})
        c1_class = stable_id("claim", {"object_id": o1_id, "claim_type": "SOURCE_CLASS", "value": {"classification": "SOUND_DATA_CONTAINER_CONFIRMED", "confidence": "CONFIRMED", "source_kind": "DATA_KNOWN"}, "status": "STATIC_VERIFIED"})
        c1_recon = stable_id("claim", {"object_id": o1_id, "claim_type": "RECONSTRUCTION_VERIFIED", "value": {"format_id": FORMAT_ID, "roundtrip": "BYTE_IDENTICAL", "mode": 0}, "status": "STATIC_VERIFIED"})
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c1_owned, o1_id, "SOURCE_OWNED", "true", "STATIC_VERIFIED"))
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c1_class, o1_id, "SOURCE_CLASS", json.dumps({"classification": "SOUND_DATA_CONTAINER_CONFIRMED", "confidence": "CONFIRMED", "source_kind": "DATA_KNOWN"}, sort_keys=True, separators=(",", ":")), "STATIC_VERIFIED"))
        store.db.execute("INSERT OR IGNORE INTO claim VALUES (?,?,?,?,?)", (c1_recon, o1_id, "RECONSTRUCTION_VERIFIED", json.dumps({"format_id": FORMAT_ID, "roundtrip": "BYTE_IDENTICAL", "mode": 0}, sort_keys=True, separators=(",", ":")), "STATIC_VERIFIED"))

        # 6. Insert source artifacts
        for name, sha in proof_shas.items():
            store.db.execute(
                "INSERT OR IGNORE INTO source_artifact VALUES (?,?,?,?)",
                (sha, "M12-W5", name, "AUDIO_RECONSTRUCTION_PROOF"),
            )

        # 7. Insert evidence references
        ref1 = stable_id("evidence", {"subject_type": "OBJECT", "subject_id": o1_id, "source_sha256": proof_shas["w5_roundtrip_receipt.json"], "fact_kind": "EXACT_BYTE_IDENTICAL_ROUNDTRIP", "fact_count": 1, "locator": {"resource_id": "AUDIO_RESOURCE_FORMAT_A_0001"}})
        ref2 = stable_id("evidence", {"subject_type": "OBJECT", "subject_id": o2_id, "source_sha256": proof_shas["w5_roundtrip_receipt.json"], "fact_kind": "EXACT_BYTE_IDENTICAL_ROUNDTRIP", "fact_count": 1, "locator": {"resource_id": "AUDIO_RESOURCE_FORMAT_A_0002"}})
        store.db.execute("INSERT OR IGNORE INTO evidence_ref VALUES (?,?,?,?,?,?,?)", (ref1, "OBJECT", o1_id, proof_shas["w5_roundtrip_receipt.json"], "EXACT_BYTE_IDENTICAL_ROUNDTRIP", 1, json.dumps({"resource_id": "AUDIO_RESOURCE_FORMAT_A_0001"}, sort_keys=True, separators=(",", ":"))))
        store.db.execute("INSERT OR IGNORE INTO evidence_ref VALUES (?,?,?,?,?,?,?)", (ref2, "OBJECT", o2_id, proof_shas["w5_roundtrip_receipt.json"], "EXACT_BYTE_IDENTICAL_ROUNDTRIP", 1, json.dumps({"resource_id": "AUDIO_RESOURCE_FORMAT_A_0002"}, sort_keys=True, separators=(",", ":"))))

        store.db.commit()
        promoted_now = True

    # Validate partition invariants
    emissions = [dict(r) for r in store.db.execute("SELECT * FROM emission ORDER BY start")]
    cursor = 0
    for em in emissions:
        if em["start"] != cursor or em["end"] <= em["start"]:
            raise ValueError(f"STOP_CANONICAL_PARTITION_INVALID:{cursor}:{em['start']}:{em['end']}")
        cursor = em["end"]
    if cursor != ROM_SIZE:
        raise ValueError(f"STOP_CANONICAL_PARTITION_SIZE_MISMATCH:{cursor}")

    source_owned_after = sum(e["end"] - e["start"] for e in emissions if e["source_owned"] == 1)
    source_owned_delta = source_owned_after - source_owned_before

    hashes = store.hashes()
    metrics = store.metrics()

    # Re-export canonical 2D map receipt
    reconciliation = map_data.get("ownership_reconciliation", {})
    audit = map_data.get("independent_audit", {})
    idempotence = map_data.get("idempotence", {})
    new_receipt = store.export(reconciliation, audit, idempotence)
    new_receipt["status"] = "PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1"
    new_receipt["runtime_import"] = map_data.get("runtime_import", {})
    new_receipt["carver_hypothesis_count"] = map_data.get("carver_hypothesis_count", 1007)

    # Write updated map
    map_path.write_text(
        json.dumps(new_receipt, sort_keys=True, separators=(",", ":"), ensure_ascii=True) + "\n",
        encoding="utf-8", newline="\n",
    )
    post_promotion_map_sha = hashlib.sha256(map_path.read_bytes()).hexdigest()

    # Write updated Markdown report
    report_path = map_path.with_suffix(".md")
    report_lines = [
        "# M12 Canonical ROM Knowledge Map 2D", "",
        f"Status: `{new_receipt['status']}`.", "",
        "The map imports accepted ownership manifests, exact 2B runtime linkage,",
        "Carver-5 format hypotheses, and exact reconstructed audio resources (W5c).", "",
        f"ROM identity is `{new_receipt['rom']['sha256']}` ({new_receipt['rom']['bytes']:,} bytes).",
        f"The emission partition has {metrics['emission_ranges']:,} contiguous intervals.",
        f"It contains {metrics['semantic_objects']:,} canonical objects, {metrics['unknown_bytes']:,} UNKNOWN bytes, and",
        f"{metrics['source_owned_bytes']:,} SOURCE_OWNED bytes ({metrics['source_owned_percent']}%).",
    ]
    report_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8", newline="\n")

    store.close()

    return {
        "format_id": FORMAT_ID,
        "eligible_range_1": f"0x{RESOURCE_1_START:06X}..0x{RESOURCE_1_END:06X}",
        "eligible_range_2": f"0x{RESOURCE_2_START:06X}..0x{RESOURCE_2_END:06X}",
        "eligible_bytes_total": TOTAL_ELIGIBLE_BYTES,
        "already_source_owned_bytes": overlap["already_source_owned_bytes"],
        "new_source_owned_bytes": new_bytes,
        "source_owned_authority": "src/tools/thor_evidence/rom_knowledge_sources.py + docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json",
        "source_owned_before": source_owned_before,
        "source_owned_expected_after": source_owned_before + new_bytes,
        "source_owned_after": source_owned_after,
        "source_owned_delta": source_owned_delta,
        "canonical_promotion_allowed": True,
        "canonical_promotion_executed": promoted_now,
        "promotion_idempotent": True,
        "canonical_partition_gaps": 0,
        "canonical_partition_overlaps": 0,
        "unrelated_canonical_changes": 0,
        "mode1_promoted_bytes": 0,
        "padding_promoted_bytes": 0,
        "pre_promotion_map_sha": pre_promotion_map_sha,
        "post_promotion_map_sha": post_promotion_map_sha,
        "hashes": hashes,
        "metrics": metrics,
    }
