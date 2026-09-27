"""Typed stage results and scheduler effects for post-run analysis."""

from __future__ import annotations

from enum import StrEnum
from typing import Any


class StageResult(StrEnum):
    PASS = "PASS"
    NO_DELTA = "NO_DELTA"
    NONFATAL_UNRESOLVED = "NONFATAL_UNRESOLVED"
    SKIPPED_NOT_APPLICABLE = "SKIPPED_NOT_APPLICABLE"
    FATAL = "FATAL"


class PipelineEffect(StrEnum):
    CONTINUE = "CONTINUE"
    CONTINUE_NONFATAL = "CONTINUE_NONFATAL"
    STOP = "STOP"


NONFATAL_SEMANTIC_STOPS = frozenset({
    "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT",
    "STOP_CONTROLLED_ENTITY_PROVENANCE_INCOMPLETE",
    "STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING",
    "STOP_GAMEPLAY_ACCEPTANCE_CONTRACT_UNPROVEN",
    "STOP_UPSTREAM_GAMEPLAY_SOURCE_UNPROVEN",
    "STOP_NO_DYNAMIC_SAT_GROUP_IDENTIFIED",
})


def stage_outcome(reason: str | None) -> tuple[StageResult, PipelineEffect]:
    """Keep semantic gaps visible while reserving scheduler STOP for fatal results."""
    if reason in NONFATAL_SEMANTIC_STOPS:
        return StageResult.NONFATAL_UNRESOLVED, PipelineEffect.CONTINUE_NONFATAL
    if reason:
        return StageResult.FATAL, PipelineEffect.STOP
    return StageResult.PASS, PipelineEffect.CONTINUE


def annotate_outcome(result: dict[str, Any], reason: str | None) -> dict[str, Any]:
    stage_result, effect = stage_outcome(reason)
    result["stage_result"] = stage_result.value
    result["pipeline_effect"] = effect.value
    if effect == PipelineEffect.CONTINUE_NONFATAL:
        result["state"] = "UNRESOLVED"
    if reason:
        result.setdefault("stop_reason", reason)
        result["unresolved_reason" if effect == PipelineEffect.CONTINUE_NONFATAL else "stop_reason"] = reason
    return result


def retained_cleanup_outcome(error: Exception) -> dict[str, Any] | None:
    """Represent failed absorption as retained evidence, never as cleanup PASS."""
    if getattr(error, "code", None) != "STOP_CLEANUP_NOT_ABSORBED":
        return None
    return {"state": "UNRESOLVED", "status": "RAW_RETAINED", "absorbed": False,
            "deletion_mode": "NONE", "files_deleted": 0, "reason": str(error)}


def finish_unabsorbed_cleanup(result: dict[str, Any], progress: Any,
                              error: Exception) -> bool:
    """Finish safely when lack of absorption proof prevents any raw deletion."""
    cleanup = retained_cleanup_outcome(error)
    if cleanup is None:
        return False
    progress.finish("UNRESOLVED", detail=cleanup["reason"])
    result["cleanup"] = cleanup
    result["stages"]["CLEANUP"] = cleanup
    result.setdefault("semantic_unresolved", []).append(
        {"stage": "CLEANUP", "reason": cleanup["reason"]})
    result["pipeline_state"] = "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
    return True


def record_pipeline_failure(result: dict[str, Any], progress: Any,
                            error: Exception) -> None:
    """Record a fatal stage error without converting its stop into acceptance."""
    reason = str(error)
    progress.finish("STOP", detail=reason)
    result["stop"] = reason
    failed_stage = getattr(progress, "current", "ASM CLOSURE") or "ASM CLOSURE"
    if result["stages"].get(failed_stage, {}).get("state") in {"PASS", "NO_DELTA"}:
        failed_stage = "CLEANUP"
    result["stages"][failed_stage] = {"state": "STOP", "stop": reason}


__all__ = ["NONFATAL_SEMANTIC_STOPS", "PipelineEffect", "StageResult",
           "annotate_outcome", "finish_unabsorbed_cleanup",
           "record_pipeline_failure", "retained_cleanup_outcome", "stage_outcome"]
