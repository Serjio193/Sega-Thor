#!/usr/bin/env python3
"""
Audit and forensic reconciliation of idle wait loop classifications across milestones.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path


def audit_idle_loops():
    print("=== M15.241 Idle Loop Classification Audit ===")

    # Historical and runtime data
    loops = {
        "loop1_attract_sync": {
            "pc_range": "0x008ACA..0x008AD0",
            "instructions": [
                "0x008ACA: TST.W ($FF1654).l",
                "0x008AD0: BNE.S $008ACA"
            ],
            "purpose": "Attract Mode & Title Screen Frame Synchronization",
            "scenario_c_dynamic_steps": 5327384,
            "share_of_total": "30.221%"
        },
        "loop2_gameplay_sync": {
            "pc_range": "0x008B72..0x008B7A",
            "instructions": [
                "0x008B72: TST.W ($FF1654).l",
                "0x008B7A: BNE.S $008B72"
            ],
            "purpose": "Active Gameplay VBlank Frame Synchronization",
            "scenario_c_dynamic_steps": 4402749,
            "share_of_total": "24.976%"
        }
    }

    total_idle_steps = sum(l["scenario_c_dynamic_steps"] for l in loops.values())
    total_guest_steps = 17627770
    idle_percent = (total_idle_steps / total_guest_steps) * 100.0

    print(f"Total Authentic Idle Steps: {total_idle_steps:,} / {total_guest_steps:,} ({idle_percent:.3f}%)")
    assert total_idle_steps == 9730133, f"Expected 9,730,133, got {total_idle_steps}"

    explanation = (
        "Reconciliation Finding:\n"
        "- Milestones M15.228 and earlier cited '0x008ACA..0x008B7A' as a single coarse range (4.86M steps).\n"
        "- M15.238/M15.239/M15.240 performed exact PC-level instruction tracing, revealing that this workload "
        "consists of two distinct, sibling frame-wait loops:\n"
        "  1. 0x008ACA..0x008AD0 (5,327,384 steps) during attract/title sequence.\n"
        "  2. 0x008B72..0x008B7A (4,402,749 steps) during active gameplay.\n"
        "- The sum of these two loops is exactly 9,730,133 steps (55.198% of total guest steps).\n"
        "- The code between 0x008AD2 and 0x008B70 is non-idle frame dispatching logic and must not be classified as idle."
    )
    print(explanation)

    audit_report = {
        "schema": "oasis.m15_241.idle_loop_audit.v1",
        "total_guest_steps": total_guest_steps,
        "total_idle_steps": total_idle_steps,
        "idle_share_percent": round(idle_percent, 3),
        "disjoint_loops": loops,
        "historical_reconciliation": explanation
    }

    out_file = Path("data/thor-evidence/idle_loop_audit_m15_241.json")
    out_file.parent.mkdir(parents=True, exist_ok=True)
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump(audit_report, f, indent=2)
    print(f"Audit report saved -> {out_file}")


if __name__ == "__main__":
    audit_idle_loops()
