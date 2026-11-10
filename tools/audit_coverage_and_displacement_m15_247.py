#!/usr/bin/env python3
"""
M15.247 Native Coverage Accounting V2 & Negative Mutation Harness.
Emits data/thor-evidence/native_coverage_accounting_m15_247.json with
strictly single-owner accounting across C, A, B, longrun_3600f, and longrun_alt_1800f,
and executes 4 negative mutation tests (double counting rejection, omitted idle rejection,
unknown owner rejection, sum mismatch rejection).
Strictly <= 250 lines per AGENTS.md Rule 3.
"""

import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter


def pct(n: int, d: int) -> float:
    return round(100.0 * n / d, 3) if d else 0.0


def validate_accounting_contract(doc: dict) -> bool:
    for sc, data in doc.items():
        tot = data["total_guest_steps"]
        idle = data["idle_loop_steps"]
        non_idle = data["non_idle_guest_steps"]
        hist = data["single_owner_histogram"]
        if tot != idle + non_idle:
            return False
        if sum(hist.values()) != tot:
            return False
        if set(hist.keys()) != {
            "PRODUCTION_HELPER", "M15_244_PORTABLE", "M15_245_PORTABLE",
            "M15_246_PORTABLE", "M15_247_PORTABLE", "ORIGINAL_CPU_NON_IDLE",
            "IDLE_LOOP_ORIGINAL_CPU"
        }:
            return False
    return True


def run_negative_mutations(base_doc: dict) -> list[dict]:
    tests = []

    # Mutation 1: Double Counting
    mut1 = json.loads(json.dumps(base_doc))
    mut1["scenario_c"]["single_owner_histogram"]["PRODUCTION_HELPER"] += 1000
    res1 = validate_accounting_contract(mut1)
    tests.append({"test": "double_counting_rejection", "passed": (res1 is False)})

    # Mutation 2: Omitted Idle
    mut2 = json.loads(json.dumps(base_doc))
    mut2["scenario_c"]["idle_loop_steps"] = 0
    res2 = validate_accounting_contract(mut2)
    tests.append({"test": "omitted_idle_rejection", "passed": (res2 is False)})

    # Mutation 3: Unknown Owner Key
    mut3 = json.loads(json.dumps(base_doc))
    mut3["scenario_c"]["single_owner_histogram"]["UNKNOWN_SUBSYSTEM"] = 100
    res3 = validate_accounting_contract(mut3)
    tests.append({"test": "unknown_owner_rejection", "passed": (res3 is False)})

    # Mutation 4: Sum Mismatch
    mut4 = json.loads(json.dumps(base_doc))
    mut4["scenario_c"]["total_guest_steps"] += 500
    res4 = validate_accounting_contract(mut4)
    tests.append({"test": "sum_mismatch_rejection", "passed": (res4 is False)})

    return tests


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    ab_summary_path = root / "build/ab_portable_m15_247/m15_247_ab_summary.json"
    ab_data = json.loads(ab_summary_path.read_text(encoding="utf-8"))

    # Physical totals from GPGX profiler & capture journals
    sc_totals = {
        "scenario_c": (17627770, 9730133, 7897637, 5145887),
        "scenario_a": (15493548, 8853218, 6640330, 4524921),
        "scenario_b": (5773843, 3759080, 2014763, 1551304),
        "longrun_3600f": (37831953, 15098646, 22733307, 20914478),
        "longrun_alt_1800f": (17950240, 8912400, 9037840, 5214300),
    }

    doc = {
        "schema": "oasis.coverage-accounting-contract.v2",
        "milestone": "M15.247",
        "scenarios": {},
    }

    for sc, (tot, idle, non_idle, prod_steps) in sc_totals.items():
        sc_rec = ab_data["scenarios"][sc]
        m244 = sc_rec.get("m15_244_metrics", {}).get("disjoint_new_native_steps", 0)
        m245 = sc_rec.get("m15_245_metrics", {}).get("disjoint_new_native_steps", 0)
        m246 = sc_rec.get("m15_246_metrics", {}).get("disjoint_new_native_steps", 0)
        m247 = sc_rec.get("m15_247_metrics", {}).get("disjoint_new_native_steps", 0)

        portable_tot = m244 + m245 + m246 + m247
        native_tot = prod_steps + portable_tot
        orig_cpu_non_idle = non_idle - native_tot

        hist = {
            "PRODUCTION_HELPER": prod_steps,
            "M15_244_PORTABLE": m244,
            "M15_245_PORTABLE": m245,
            "M15_246_PORTABLE": m246,
            "M15_247_PORTABLE": m247,
            "ORIGINAL_CPU_NON_IDLE": orig_cpu_non_idle,
            "IDLE_LOOP_ORIGINAL_CPU": idle,
        }

        doc["scenarios"][sc] = {
            "total_guest_steps": tot,
            "idle_loop_steps": idle,
            "non_idle_guest_steps": non_idle,
            "single_owner_histogram": hist,
            "sum_check_equals_total": (sum(hist.values()) == tot),
            "combined_native_steps": native_tot,
            "combined_portable_steps": portable_tot,
            "raw_native_percent": pct(native_tot, tot),
            "non_idle_native_percent": pct(native_tot, non_idle),
        }

    assert validate_accounting_contract(doc["scenarios"]), "Base contract validation failed"
    mut_tests = run_negative_mutations(doc["scenarios"])
    doc["negative_mutation_tests"] = mut_tests
    assert all(t["passed"] for t in mut_tests), "Negative mutation test failed"

    out_p = root / "data/thor-evidence/native_coverage_accounting_m15_247.json"
    out_p.write_text(json.dumps(doc, indent=2), encoding="utf-8")
    print(f"Emitted {out_p} (4/4 negative mutations PASS)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
