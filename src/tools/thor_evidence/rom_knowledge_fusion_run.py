"""Reproducible M14.2B-R generation and order-independence acceptance runner."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

try:
    from .rom_knowledge_fusion import clone_and_fuse
    from .rom_knowledge_fusion_query import global_object_view, why
    from .rom_knowledge_map import KnowledgeStore
except ImportError:
    from rom_knowledge_fusion import clone_and_fuse
    from rom_knowledge_fusion_query import global_object_view, why
    from rom_knowledge_map import KnowledgeStore


def capture(root: Path) -> tuple[int, dict[str, Path]]:
    name = root.name
    run_id = int(name.split("-", 2)[1])
    closure = root / "generic-recursive-closure"
    files = {"gameplay": root / "postrun_gameplay_provenance.json",
        "sprite": root / "postrun_sprite_provenance.json",
        "vdp_dma": root / "postrun_vdp_dma.json",
        "gameplay_receipt": root / "postrun_gameplay_receipt.json",
        "sprite_receipt": root / "postrun_sprite_receipt.json",
        "vdp_receipt": root / "postrun_vdp_receipt.json",
        "generic_graph": closure / "postrun_provenance_graph.json",
        "closure_receipt": closure / "postrun_generic_closure_receipt.json",
        "normalized_corpus": closure / "normalized_generic_corpus.json"}
    missing = [str(path) for path in files.values() if not path.is_file()]
    if missing:
        raise ValueError("STOP_FUSION_CAPTURE_ARTIFACT_MISSING:" + ",".join(missing))
    return run_id, files


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-db", type=Path, required=True)
    parser.add_argument("--capture", type=Path, action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--reverse-output", type=Path, required=True)
    parser.add_argument("--parent-generation", required=True)
    args = parser.parse_args()
    captures = [capture(path) for path in args.capture]
    first = clone_and_fuse(args.base_db, args.output, "gen-m14-2b-forward",
                           args.parent_generation, captures, verify_idempotence=True)
    reverse = clone_and_fuse(args.base_db, args.reverse_output, "gen-m14-2b-reverse",
                             args.parent_generation, list(reversed(captures)))
    if first["graph_hash"] != reverse["graph_hash"]:
        raise ValueError("STOP_FUSION_ORDER_INDEPENDENCE_MISMATCH")
    witness = first["witness_paths"][0]
    emitter_id = witness["nodes"][1]["object_id"]
    query_store = KnowledgeStore(args.output, "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263",
                                 3145728, read_only=True)
    try:
        view = global_object_view(query_store, emitter_id)
        rationale = why(query_store, witness["relations"][0])
    finally:
        query_store.close()
    gameplay_sha = first["captures"][0]["source_hashes"]["gameplay"]
    sprite_sha = first["captures"][0]["source_hashes"]["sprite"]
    independent = gameplay_sha != sprite_sha
    if first["runtime_occurrences"] < 1:
        raise ValueError("STOP_FUSION_SCOPED_OCCURRENCE_MISSING")
    print(json.dumps({"schema": "oasis.m14.2b.global-evidence-fusion-acceptance.v1",
        "pass": first["cross_source_connections"] > 0 and
            first["components_after"] < first["components_before"] and
            first["multi_source_objects"] > 0 and independent and
            len(view["supporting_analyzers"]) >= 2 and bool(rationale["inputs"]),
        "order_independent": True, "forward": first,
        "reverse_graph_hash": reverse["graph_hash"],
        "acceptance_witness": {
            "source_a_artifact_sha256": gameplay_sha,
            "source_a_analyzer": "M12 gameplay RAM/SAT provenance",
            "source_a_facts": "EXACT RAM shadow writer and RAM-to-DMA transfer",
            "source_b_artifact_sha256": sprite_sha,
            "source_b_analyzer": "M12 Sprite/SAT provenance",
            "source_b_facts": "EXACT DMA-to-hardware-SAT destination and entries",
            "source_a_differs_from_b": independent,
            "runtime_capture_scoped_occurrence_count": first["runtime_occurrences"],
            "global_view_supporting_analyzers": view["supporting_analyzers"],
            "global_view_runtime_occurrences": len(view["runtime_occurrences"]),
            "why_rule": rationale["rule_id"], "why_inputs": rationale["inputs"]},
        "full_selector_rom_renderer_chain_available": False},
        sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
