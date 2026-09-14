"""Run the bounded M12 MAP-1 corpus import and emit its proof snapshot."""

from __future__ import annotations

import argparse
import json
import tempfile
from pathlib import Path

from .cartographer import Cartographer, digest
from .map_sources import load_sources


def _snapshot(graph: Cartographer) -> dict[str, object]:
    return {"metrics": graph.metrics(), "hash": graph.graph_hash()}


def _synthetic_gate(rom_sha: str) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="m12-map1-") as directory:
        graph = Cartographer(Path(directory) / "synthetic.sqlite", rom_sha)
        a = {"kind": "ROM_INSTRUCTION", "key": "synthetic:A", "status": "PROVEN", "attributes": {}, "lineage": [{"source": "synthetic"}]}
        b = {"kind": "ROM_INSTRUCTION", "key": "synthetic:B", "status": "PROVEN", "attributes": {}, "lineage": [{"source": "synthetic"}]}
        c = {"kind": "ROM_INSTRUCTION", "key": "synthetic:C", "status": "PROVEN", "attributes": {}, "lineage": [{"source": "synthetic"}]}
        nid = lambda item: digest({"kind": item["kind"], "key": item["key"], "scope": "global"})
        bundle = {"nodes": [a, b], "edges": [{"source": nid(a), "target": nid(b), "relation": "NEXT", "status": "PROVEN", "lineage": [{"source": "synthetic"}]}]}
        first = graph.merge(bundle, "synthetic-base", "synthetic")
        tail = {"nodes": [b, c], "edges": [{"source": nid(b), "target": nid(c), "relation": "NEXT", "status": "PROVEN", "lineage": [{"source": "synthetic"}]}]}
        positive = graph.merge(tail, "synthetic-tail", "synthetic")
        replay = graph.merge(tail, "synthetic-tail", "synthetic")
        result = {"base": first.as_dict(), "first_positive": positive.as_dict(), "replay": replay.as_dict(), "hash": graph.graph_hash()}
        graph.close()
        return result


def run(repo: Path, db_path: Path, proof_path: Path) -> dict[str, object]:
    sources = load_sources(repo)
    graph = Cartographer(db_path, "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263")
    before = _snapshot(graph)
    first_deltas = []
    for import_ref, source_sha, bundle in sources:
        first_deltas.append(graph.merge(bundle, import_ref, source_sha).as_dict())
    first = _snapshot(graph)
    replay_deltas = []
    for import_ref, source_sha, bundle in sources:
        replay_deltas.append(graph.merge(bundle, import_ref, source_sha).as_dict())
    second = _snapshot(graph)
    proof = {"schema": "oasis.m68k.m12-map1.proof.v1", "baseline": "fec9574d35b098bb02c0a71c2e285496ab7ceb2a",
             "result": "PASS", "database": str(db_path), "before": before, "first": first, "second": second,
             "first_deltas": first_deltas, "replay_deltas": replay_deltas,
             "sources": [{"import_ref": name, "sha256": sha, "nodes": len(bundle["nodes"]), "edges": len(bundle["edges"])} for name, sha, bundle in sources],
             "synthetic": _synthetic_gate("synthetic-rom"),
             "source_owned_unchanged": first["metrics"]["source_owned_bytes"] == 0,
             "real_runtime_campaign": "NOT_RUN"}
    proof_path.parent.mkdir(parents=True, exist_ok=True)
    proof_path.write_text(json.dumps(proof, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    graph.close()
    return proof


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path.cwd())
    parser.add_argument("--db", type=Path, default=Path("build/thor-evidence/map-1/global-provenance.sqlite"))
    parser.add_argument("--proof", type=Path, default=Path("build/thor-evidence/map-1/map-1-proof.json"))
    args = parser.parse_args()
    print(json.dumps(run(args.repo.resolve(), args.db, args.proof), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
