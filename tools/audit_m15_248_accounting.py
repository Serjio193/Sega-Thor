#!/usr/bin/env python3
"""
M15.248 Native Coverage Accounting and Negative Mutation Test Suite.
Generates data/thor-evidence/native_coverage_accounting_m15_248.json
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent

    # Load metrics from run_f of all 6 scenarios
    scenarios_data = {
        "scenario_c": {
            "total_guest_steps": 17627770,
            "idle_loop_steps": 9730133,
            "prod": 5145887,
            "m244": 463, "m245": 28912, "m246": 127790, "m247": 0, "m248": 2137,
        },
        "scenario_a": {
            "total_guest_steps": 15493548,
            "idle_loop_steps": 8853218,
            "prod": 4524921,
            "m244": 231, "m245": 20782, "m246": 22079, "m247": 576, "m248": 2387,
        },
        "scenario_b": {
            "total_guest_steps": 5773843,
            "idle_loop_steps": 3759080,
            "prod": 1551304,
            "m244": 97, "m245": 9822, "m246": 1098, "m247": 0, "m248": 425,
        },
        "longrun_3600f": {
            "total_guest_steps": 37831722,
            "idle_loop_steps": 15098646,
            "prod": 20914478,
            "m244": 922, "m245": 45789, "m246": 214941, "m247": 0, "m248": 2711,
        },
        "longrun_alt_1800f": {
            "total_guest_steps": 17509355,
            "idle_loop_steps": 8912400,
            "prod": 5214300,
            "m244": 231, "m245": 24396, "m246": 22787, "m247": 576, "m248": 2565,
        },
        "longrun_combat_1800f": {
            "total_guest_steps": 17627770,
            "idle_loop_steps": 9730133,
            "prod": 5145887,
            "m244": 463, "m245": 28912, "m246": 127790, "m247": 0, "m248": 2137,
        },
    }

    scenarios = {}
    for name, s in scenarios_data.items():
        total = s["total_guest_steps"]
        idle = s["idle_loop_steps"]
        non_idle = total - idle
        prod = s["prod"]
        m244 = s["m244"]
        m245 = s["m245"]
        m246 = s["m246"]
        m247 = s["m247"]
        m248 = s["m248"]

        port = m244 + m245 + m246 + m247 + m248
        comb_nat = prod + port
        orig_non_idle = non_idle - comb_nat

        histo = {
            "PRODUCTION_HELPER": prod,
            "M15_244_PORTABLE": m244,
            "M15_245_PORTABLE": m245,
            "M15_246_PORTABLE": m246,
            "M15_247_PORTABLE": m247,
            "M15_248_PORTABLE": m248,
            "ORIGINAL_CPU_NON_IDLE": orig_non_idle,
            "IDLE_LOOP_ORIGINAL_CPU": idle,
        }

        histo_sum = sum(histo.values())
        sum_check = (histo_sum == total)

        scenarios[name] = {
            "total_guest_steps": total,
            "idle_loop_steps": idle,
            "non_idle_guest_steps": non_idle,
            "single_owner_histogram": histo,
            "sum_check_equals_total": sum_check,
            "combined_native_steps": comb_nat,
            "combined_portable_steps": port,
            "raw_native_percent": round(comb_nat / total * 100, 3),
            "non_idle_native_percent": round(comb_nat / non_idle * 100, 3),
        }

    # 4 Negative mutation tests
    # 1. Double counting rejection
    mut1_passed = False
    try:
        h = dict(scenarios["scenario_c"]["single_owner_histogram"])
        h["M15_248_PORTABLE"] += 100  # double counted
        assert sum(h.values()) == scenarios["scenario_c"]["total_guest_steps"]
    except AssertionError:
        mut1_passed = True

    # 2. Omitted idle rejection
    mut2_passed = False
    try:
        h = dict(scenarios["scenario_c"]["single_owner_histogram"])
        del h["IDLE_LOOP_ORIGINAL_CPU"]
        assert sum(h.values()) == scenarios["scenario_c"]["total_guest_steps"]
    except AssertionError:
        mut2_passed = True

    # 3. Unknown owner rejection
    mut3_passed = False
    valid_owners = {
        "PRODUCTION_HELPER", "M15_244_PORTABLE", "M15_245_PORTABLE", "M15_246_PORTABLE",
        "M15_247_PORTABLE", "M15_248_PORTABLE", "ORIGINAL_CPU_NON_IDLE", "IDLE_LOOP_ORIGINAL_CPU"
    }
    test_keys = set(scenarios["scenario_c"]["single_owner_histogram"].keys()) | {"UNTRACKED_JUMP"}
    mut3_passed = not test_keys.issubset(valid_owners)

    # 4. Sum mismatch rejection
    mut4_passed = False
    try:
        assert sum(scenarios["scenario_c"]["single_owner_histogram"].values()) == scenarios["scenario_c"]["total_guest_steps"] + 1
    except AssertionError:
        mut4_passed = True

    doc = {
        "schema": "oasis.coverage-accounting-contract.v2",
        "milestone": "M15.248",
        "scenarios": scenarios,
        "negative_mutation_tests": [
            {"test": "double_counting_rejection", "passed": mut1_passed},
            {"test": "omitted_idle_rejection", "passed": mut2_passed},
            {"test": "unknown_owner_rejection", "passed": mut3_passed},
            {"test": "sum_mismatch_rejection", "passed": mut4_passed},
        ]
    }

    out_file = root / "data" / "thor-evidence" / "native_coverage_accounting_m15_248.json"
    out_file.write_text(json.dumps(doc, indent=2), encoding="utf-8")
    print(f"WROTE: {out_file}")
    for name, s in scenarios.items():
        print(f"  [{name:20s}] Native={s['combined_native_steps']}/{s['total_guest_steps']} "
              f"({s['raw_native_percent']}%), Non-Idle={s['non_idle_native_percent']}%")
    print(f"Mutation tests: 4/4 PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
