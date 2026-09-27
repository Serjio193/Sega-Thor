"""Verified MASTER V2 read authority for provenance and post-run outcomes."""
from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

from master_v2_shadow import (LegacyState, SCHEMA, legacy_absorption_projection,
                              legacy_outcomes_projection, legacy_provenance_projection,
                              read_master_v2_section)

STOP_UNAVAILABLE = "STOP_MASTER_OUTCOME_UNAVAILABLE"
STOP_MISMATCH = "STOP_MASTER_OUTCOME_PROJECTION_MISMATCH"
REQUIRED_SECTIONS = ("provenance", "outcomes", "absorption_history")


def _load(path: Path, section: str) -> dict[str, Any]:
    try:
        value = json.loads(read_master_v2_section(path, section))
    except (OSError, ValueError, json.JSONDecodeError) as error:
        raise ValueError(f"{STOP_UNAVAILABLE}:{section}") from error
    if not isinstance(value, dict) or value.get("schema") not in {
            "oasis.m12.master-v2.provenance.v1",
            "oasis.m12.master-v2.outcomes.v1",
            "oasis.m12.master-v2.absorption-history.v1"}:
        raise ValueError(f"{STOP_UNAVAILABLE}:{section}")
    return value


class MasterProvenanceView:
    """Structured Stage 6 state; it never falls back to legacy JSON."""

    def __init__(self, path: Path):
        self.path = Path(path).resolve()
        self.state = _load(self.path, "provenance")

    @property
    def run_id(self) -> int:
        return int(self.state["run_id"])

    @property
    def status(self) -> str | None:
        return self.state.get("status")

    @property
    def relations(self) -> dict[str, Any]:
        return dict(self.state.get("relations", {}))

    @property
    def counters(self) -> dict[str, Any]:
        return dict(self.state.get("counters", {}))

    @property
    def receipt(self) -> dict[str, Any]:
        return dict(self.state.get("receipt", {}))


class MasterOutcomeView:
    """Single read layer for accepted Stage 5-9 outcomes and absorption history."""

    def __init__(self, path: Path):
        self.path = Path(path).resolve()
        self.provenance = MasterProvenanceView(self.path)
        self.outcomes = _load(self.path, "outcomes")
        self.absorption = _load(self.path, "absorption_history")
        run_ids = {int(self.outcomes.get("run_id", -1)),
                    int(self.absorption.get("run_id", -1)), self.provenance.run_id}
        if len(run_ids) != 1:
            raise ValueError(f"{STOP_UNAVAILABLE}:run_id")

    @property
    def run_id(self) -> int:
        return self.provenance.run_id

    @property
    def stage_outcomes(self) -> dict[str, Any]:
        return dict(self.outcomes.get("stages", {}))

    @property
    def absorbed_runs(self) -> list[dict[str, Any]]:
        return list(self.absorption.get("absorbed_receipts", []))

    def require_stage(self, stage: str, accepted: set[str] | None = None) -> dict[str, Any]:
        value = self.stage_outcomes.get(stage)
        if not isinstance(value, dict):
            raise ValueError(f"{STOP_UNAVAILABLE}:{stage}")
        status = value.get("status") or value.get("state")
        if accepted is not None and status not in accepted:
            raise ValueError(f"{STOP_UNAVAILABLE}:{stage}:{status}")
        return value


def verify_projection(state: LegacyState, master_path: Path) -> dict[str, Any]:
    """Compare structured legacy projections with all R3 MASTER sections."""
    view = MasterOutcomeView(master_path)
    expected = {"provenance": legacy_provenance_projection(state),
                "outcomes": legacy_outcomes_projection(state),
                "absorption_history": legacy_absorption_projection(state)}
    actual = {"provenance": view.provenance.state, "outcomes": view.outcomes,
              "absorption_history": view.absorption}
    if actual != expected:
        raise ValueError(STOP_MISMATCH)
    return {"status": "PASS", "run_id": view.run_id,
            "sections": {name: "PASS" for name in REQUIRED_SECTIONS}}


def open_outcome_authority(path: Path) -> MasterOutcomeView:
    """Open R3 authority and fail closed when any required section is unavailable."""
    return MasterOutcomeView(path)


def resolve_master_outcome_path(project_root: Path) -> Path:
    """Resolve the current MASTER V2 pointer before legacy shadow fixtures."""
    configured = os.environ.get("THOR_MASTER_V2_PATH")
    if configured:
        path = Path(configured)
    else:
        pointer_path = Path(project_root) / "build" / "thor-evidence" / "master-v2" / "current.json"
        if not pointer_path.is_file():
            pointer_path = Path(project_root) / "build" / "thor-evidence" / \
                "master-v2-shadow-r3" / "shadow-current.json"
        try:
            pointer = json.loads(pointer_path.read_text(encoding="utf-8"))
            path = pointer_path.parent / str(pointer["path"])
        except (OSError, KeyError, TypeError, json.JSONDecodeError) as error:
            raise ValueError(STOP_UNAVAILABLE) from error
    if not path.is_file():
        raise ValueError(STOP_UNAVAILABLE)
    return path.resolve()


__all__ = ["MasterOutcomeView", "MasterProvenanceView", "REQUIRED_SECTIONS",
           "STOP_MISMATCH", "STOP_UNAVAILABLE", "open_outcome_authority",
           "resolve_master_outcome_path", "verify_projection"]
