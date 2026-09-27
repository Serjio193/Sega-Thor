"""Coordinator handoff for the M13 generic closure stage."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from live_forward_generic_closure_stage import run_generic_closure


def run_generic_closure_handoff(receipt: dict[str, Any], analysis_dir: Path,
                                rom_path: Path, total_segments: int | None,
                                progress: Any, result: dict[str, Any]) -> tuple[dict[str, Any], str | None]:
    progress.start("GENERIC RECURSIVE CLOSURE", total=total_segments, unit="FLOW records",
                   detail="sealing normalized corpus and reaching provenance fixpoint")
    try:
        stage = run_generic_closure(receipt, analysis_dir, rom_path, progress)
        stage["state"] = stage.get("status", stage.get("state", "STOP"))
        result["stages"]["GENERIC RECURSIVE CLOSURE"] = stage
        if stage["state"] in {"PASS", "NO_DELTA", "SKIPPED_NOT_APPLICABLE"}:
            progress.finish(stage["state"], detail="generic closure complete")
            return stage, None
        progress.finish("STOP", detail="generic closure stopped")
        return stage, stage.get("reason", "STOP_GENERIC_CLOSURE")
    except Exception as error:
        reason = f"STOP_GENERIC_CLOSURE_EXCEPTION:{type(error).__name__}"
        detail = f"{reason}: {error!r}"
        progress.finish("STOP", detail=detail)
        stage = {"state": "STOP", "stop": reason, "exception_type": type(error).__name__,
                 "exception_repr": repr(error)}
        result["stages"]["GENERIC RECURSIVE CLOSURE"] = stage
        return stage, reason
