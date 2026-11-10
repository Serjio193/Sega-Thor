#!/usr/bin/env python3
"""
M15.250 Final Six-Scenario Single-Owner Native Coverage Accounting.
Reads M15.249 baseline accounting and adds verified M15.250 disjoint portable
guest instruction counts from data/thor-evidence/m15_250_ab_summary.json,
deducting them strictly 1-for-1 from ORIGINAL_CPU_NON_IDLE_STEPS.
Strictly <= 150 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    m249 = json.loads((root / "data/thor-evidence/native_coverage_accounting_m15_249.json").read_text(encoding="utf-8"))
    ab = json.loads((root / "data/thor-evidence/m15_250_ab_summary.json").read_text(encoding="utf-8"))

    assert ab["overall_status"] == "PASS", "M15.250 A/B summary must PASS!"

    scenarios_out = {}
    tot_244, tot_245, tot_246, tot_247, tot_248, tot_249, tot_250 = 0, 0, 0, 0, 0, 0, 0
    calc_proof = {}

    for sc, prev in m249["scenarios"].items():
        m250_steps = ab["scenarios"][sc]["m15_250_metrics"]["disjoint_new_native_steps"]
        total = prev["TOTAL_GUEST_STEPS"]
        non_idle = prev["NON_IDLE_GUEST_STEPS"]
        idle = prev["ORIGINAL_CPU_IDLE_STEPS"]
        prod = prev["PRODUCTION_NATIVE_STEPS"]

        m244 = prev["M15_244_STEPS"]
        m245 = prev["M15_245_STEPS"]
        m246 = prev["M15_246_STEPS"]
        m247 = prev["M15_247_STEPS"]
        m248 = prev["M15_248_STEPS"]
        m249_s = prev["M15_249_STEPS"]

        portable_total = m244 + m245 + m246 + m247 + m248 + m249_s + m250_steps
        residual_cpu = prev["ORIGINAL_CPU_NON_IDLE_STEPS"] - m250_steps
        assert residual_cpu >= 0, f"Residual CPU underflow in {sc}"
        assert prod + portable_total + residual_cpu + idle == total

        tot_244 += m244
        tot_245 += m245
        tot_246 += m246
        tot_247 += m247
        tot_248 += m248
        tot_249 += m249_s
        tot_250 += m250_steps
        calc_proof[sc] = portable_total

        scenarios_out[sc] = {
            "TOTAL_GUEST_STEPS": total,
            "NON_IDLE_GUEST_STEPS": non_idle,
            "PRODUCTION_NATIVE_STEPS": prod,
            "M15_244_STEPS": m244,
            "M15_245_STEPS": m245,
            "M15_246_STEPS": m246,
            "M15_247_STEPS": m247,
            "M15_248_STEPS": m248,
            "M15_249_STEPS": m249_s,
            "M15_250_STEPS": m250_steps,
            "PORTABLE_TOTAL_STEPS": portable_total,
            "ORIGINAL_CPU_NON_IDLE_STEPS": residual_cpu,
            "ORIGINAL_CPU_IDLE_STEPS": idle,
            "RAW_NATIVE_PERCENT": round(((prod + portable_total) / total) * 100.0, 3),
            "NON_IDLE_NATIVE_PERCENT": round(((prod + portable_total) / non_idle) * 100.0, 3),
            "PORTABLE_ONLY_PERCENT": round((portable_total / non_idle) * 100.0, 3),
            "SINGLE_OWNER_SUM_VERIFIED": True,
        }

    cum_total = tot_244 + tot_245 + tot_246 + tot_247 + tot_248 + tot_249 + tot_250
    calc_proof["milestone_breakdown"] = {
        "m15_244": tot_244,
        "m15_245": tot_245,
        "m15_246": tot_246,
        "m15_247": tot_247,
        "m15_248": tot_248,
        "m15_249": tot_249,
        "m15_250": tot_250,
        "exact_formula": f"{tot_244} + {tot_245} + {tot_246} + {tot_247} + {tot_248} + {tot_249} + {tot_250} = {cum_total}",
    }

    doc = {
        "schema": "oasis.m15_250.final_accounting.v1",
        "milestone": "M15.250",
        "cumulative_portable_guest_steps": cum_total,
        "calculation_proof": calc_proof,
        "scenarios": scenarios_out,
    }
    out_p = root / "data/thor-evidence/native_coverage_accounting_m15_250.json"
    out_p.write_text(json.dumps(doc, indent=2), encoding="utf-8")
    print(f"WROTE: {out_p} (M15.250={tot_250}, Cumulative={cum_total})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
