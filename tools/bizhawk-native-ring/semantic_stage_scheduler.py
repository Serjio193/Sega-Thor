"""Run semantic candidate stages without blocking independent ROM closure."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from generic_closure_handoff import run_generic_closure_handoff
from live_forward_controlled_entity_stage import run_controlled_entity_pipeline_stage
from live_forward_gameplay_stage import run_gameplay_pipeline_stage
from pipeline_outcomes import PipelineEffect, annotate_outcome


def run_semantic_stages(receipt: dict[str, Any], rom_path: Path, analysis_dir: Path,
                        total_segments: int | None, progress: Any,
                        sprite_result: dict[str, Any], result: dict[str, Any]) -> str | None:
    gameplay, gameplay_stop = run_gameplay_pipeline_stage(
        receipt, rom_path, analysis_dir, total_segments, progress, sprite_result)
    annotate_outcome(gameplay, gameplay_stop)
    result["stages"]["GAMEPLAY RAM / ENTITY CANDIDATES"] = gameplay
    if gameplay_stop and gameplay["pipeline_effect"] != PipelineEffect.CONTINUE_NONFATAL:
        return gameplay_stop

    controlled, controlled_stop = run_controlled_entity_pipeline_stage(
        receipt, rom_path, analysis_dir, total_segments, progress, gameplay)
    annotate_outcome(controlled, controlled_stop)
    result["stages"]["CONTROLLED ENTITY PROVENANCE"] = controlled
    if controlled_stop and controlled["pipeline_effect"] != PipelineEffect.CONTINUE_NONFATAL:
        return controlled_stop

    unresolved = [{"stage": name, "reason": row["unresolved_reason"]}
                  for name, row in result["stages"].items()
                  if row.get("stage_result") == "NONFATAL_UNRESOLVED"]
    if unresolved:
        result["semantic_unresolved"] = unresolved
    _, generic_stop = run_generic_closure_handoff(
        receipt, analysis_dir, rom_path, total_segments, progress, result)
    return generic_stop


__all__ = ["run_semantic_stages"]
