"""Reproduce, validate, and adopt the exact M14.4 ASM map proposal."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from rom_knowledge_map import KnowledgeStore, canonical
from rom_knowledge_map_adoption_validation import (EXPECTED_COMPONENTS, EXPECTED_OPERATIONS,
    PROPOSAL_HASH, PROPOSAL_ID, _tracked_identity, reproduce_proposal,
    validate_proposal, as_store)
from rom_knowledge_map_adoption_apply import apply_generation, _logical_generation

def campaign(base_db: Path, rom: Path, decoder: Path, assembler: Path,
             output: Path, tracked_receipt: Path) -> dict[str, Any]:
    receipt, reproduced, campaign_root = reproduce_proposal(base_db, rom, decoder,
        assembler, output / "reproduction", tracked_receipt)
    with as_store(base_db) as base:
        _tracked_identity(receipt, reproduced, base)
    proposal_db = campaign_root / "proposal-child.sqlite"
    audit = validate_proposal(base_db, proposal_db, receipt, reproduced,
        campaign_root, rom, assembler, output)
    first = apply_generation(proposal_db, base_db, audit, output / "generation-a", "gen-m14-5")
    second = apply_generation(proposal_db, base_db, audit, output / "generation-b", "gen-m14-5")
    fields = ("generation_id", "map_hash", "emission_hash", "generation_sha256",
              "before_partition", "after_partition", "before_metrics", "after_metrics",
              "application_lineage", "components", "global_object_views")
    if any(first[key] != second[key] for key in fields) or \
            _logical_generation(output / "generation-a" / "canonical.sqlite") != \
            _logical_generation(output / "generation-b" / "canonical.sqlite"):
        raise ValueError("STOP_M14_5_NONDETERMINISTIC_MAP_APPLICATION")
    if not first["components"] or first["after_partition"]["gaps"] or \
            first["after_partition"]["overlaps"] or \
            first["after_metrics"]["source_owned_bytes"] != 1_487_672 or \
            first["after_metrics"]["unknown_bytes"] >= first["before_metrics"]["unknown_bytes"]:
        raise ValueError("STOP_M14_5_ACCEPTANCE_INVARIANTS")
    return {"schema": "oasis.m14.5.exact-asm-map-adoption.v1",
        "acceptance_status": "PASS_EXACT_ASM_MAP_ADOPTION_V1",
        "proposal": {key: audit["identity"][field] for key, field in (
            ("proposal_id", "proposal_id"), ("proposal_hash", "proposal_hash"),
            ("base_generation", "base_generation"), ("base_map_hash", "base_map_hash"),
            ("graph_hash", "graph_hash"), ("emission_hash", "emission_hash"))},
        "m14_4_components": EXPECTED_COMPONENTS,
        "m14_4_roundtrip_bytes": sum(item["size"] for item in audit["valid_components"]),
        "operation_counts": {name: sum(op["operation"] == name for op in audit["operations"])
            for name in ("SPLIT_RANGE", "ADD_BOUNDARY", "CLASSIFY_RANGE", "ADD_REFERENCE",
                         "ADD_SYMBOL")} | {"OTHER": sum(op["operation"] not in {
                             "SPLIT_RANGE", "ADD_BOUNDARY", "CLASSIFY_RANGE",
                             "ADD_REFERENCE", "ADD_SYMBOL"} for op in audit["operations"])},
        "proposal_operations_total": len(audit["operations"]),
        "proposal_operations_accepted": len(audit["accepted_operation_ordinals"]),
        "proposal_operations_rejected": audit["rejected_operation_count"],
        "operation_audit": audit["operation_audit"],
        "components": first["components"],
        "new_asm_components": len(first["components"]),
        "new_asm_bytes": first["after_metrics"]["asm_bytes"] -
            first["before_metrics"]["asm_bytes"],
        "unknown_bytes_before": first["before_metrics"]["unknown_bytes"],
        "unknown_bytes_after": first["after_metrics"]["unknown_bytes"],
        "unknown_bytes_delta": first["after_metrics"]["unknown_bytes"] -
            first["before_metrics"]["unknown_bytes"],
        "unknown_ranges_before": first["before_partition"]["unknown_ranges"],
        "unknown_ranges_after": first["after_partition"]["unknown_ranges"],
        "map_ranges_before": first["before_partition"]["ranges"],
        "map_ranges_after": first["after_partition"]["ranges"],
        "source_owned_before": first["before_metrics"]["source_owned_bytes"],
        "source_owned_after": first["after_metrics"]["source_owned_bytes"],
        "source_owned_delta": first["after_metrics"]["source_owned_bytes"] -
            first["before_metrics"]["source_owned_bytes"],
        "stage7_eligible_components": sum(item["stage7"]["stage7_eligible"]
            for item in audit["components"]), "stage7_promoted_components": 0,
        "stage7_promotions_performed": False,
        "stale_parent_rejected": first["stale_parent_rejected"],
        "deterministic_replay": "PASS", "generation_a": first, "generation_b": second,
        "canonical_partition": first["after_partition"]}

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-db", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--decoder", required=True, type=Path)
    parser.add_argument("--assembler", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--tracked-receipt", type=Path, default=Path(
        "docs/reports/THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json"))
    args = parser.parse_args()
    report = campaign(args.base_db, args.rom, args.decoder, args.assembler,
                      args.output.resolve(), args.tracked_receipt)
    payload = canonical(report) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(payload, encoding="utf-8")
    print(payload, end="")


if __name__ == "__main__":
    main()
