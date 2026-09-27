"""Structural contract for the live Stage 7 -> 8 -> 9 handoff."""

from __future__ import annotations

import ast
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STAGE7 = ROOT / "src" / "tools" / "thor_evidence" / "map_driven_asm_stage7.py"
COORDINATOR = ROOT / "tools" / "bizhawk-native-ring" / "live_forward_complete_pipeline.py"


def _function(tree: ast.AST, name: str) -> ast.FunctionDef:
    return next(node for node in ast.walk(tree)
                if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
                and node.name == name)


def main() -> int:
    stage7_text = STAGE7.read_text(encoding="utf-8")
    stage7_tree = ast.parse(stage7_text)
    stage7 = _function(stage7_tree, "run_stage7")
    called = {node.func.id for node in ast.walk(stage7)
              if isinstance(node, ast.Call) and isinstance(node.func, ast.Name)}
    assert "run_stage8" not in called
    assert "_audit_materialized" not in called
    assert "_write_json_atomic" in called
    assert "STOP_STAGE8_SUBPROCESS_TIMEOUT" in stage7_text

    coordinator = COORDINATOR.read_text(encoding="utf-8")
    stage7_pos = coordinator.index("stage7 = run_stage7(")
    audit_start = coordinator.index('progress.start("FULL ROM AUDIT"')
    stage8_pos = coordinator.index("stage8 = run_stage8(")
    cleanup_start = coordinator.index('progress.start("CLEANUP"')
    assert stage7_pos < audit_start < stage8_pos < cleanup_start
    assert "ANALYSIS COMPLETE ✓" in coordinator
    print("Live Stage 7->8->9 structural contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
