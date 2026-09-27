"""Pre-reclaim acceptance gate for generic closure evidence."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any


def generic_acceptance_failure(run_analysis: Path,
                               stage_results: dict[str, Any]) -> str | None:
    stage = stage_results.get("GENERIC RECURSIVE CLOSURE", {})
    if stage.get("state") not in {"PASS", "NO_DELTA"} or \
            stage.get("deterministic_replay_status") != "PASS" or \
            stage.get("gap_ranking_status") != "PASS":
        return "generic closure, gap ranking, and deterministic replay are not accepted"
    root = run_analysis / "generic-recursive-closure"
    closure_path = root / "postrun_generic_closure_receipt.json"
    engine_path = root / "postrun_recursive_closure_receipt.json"
    ranking_path = root / "postrun_capture_gap_ranking.json"
    if not all(path.is_file() for path in (closure_path, engine_path, ranking_path)):
        return "generic closure acceptance artifacts are missing"
    closure = json.loads(closure_path.read_text(encoding="utf-8"))
    engine = json.loads(engine_path.read_text(encoding="utf-8"))
    ranking = json.loads(ranking_path.read_text(encoding="utf-8"))
    ranking_hash = hashlib.sha256(ranking_path.read_bytes()).hexdigest()
    if closure.get("deterministic_replay_status") != "PASS" or \
            closure.get("gap_ranking_status") != "PASS" or \
            closure.get("deterministic_replay_sha256") != stage.get("deterministic_replay_sha256") or \
            engine.get("status") not in {"PASS", "NO_DELTA"} or \
            engine.get("output_hashes", {}).get("postrun_capture_gap_ranking.json") != ranking_hash or \
            ranking.get("schema") != "oasis.m13.generic-gap-ranking.v1" or \
            not isinstance(ranking.get("gaps"), list):
        return "generic closure replay or gap ranking is incomplete or altered"
    return None
