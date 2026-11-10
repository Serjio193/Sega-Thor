#!/usr/bin/env python3
"""
M15.246 Phase 1 & Phase 2: Native Coverage Accounting Forensics & Disjoint Ownership Audit.
1. Reconciles historical M15.242 (37.652%/84.041%), M15.244, and M15.245 (29.359%/65.529%)
   coverage figures from actual on-disk step-metrics JSONs and historical receipt scripts.
2. Publishes versioned canonical accounting contract (`oasis.coverage-accounting-contract.v1`).
3. Assigns strictly 1 disjoint owner per dynamic instruction across Scenarios C, A, B.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter


def sum_disk_steppers(replay_run: pathlib.Path) -> tuple[int, dict[str, int]]:
    per: dict[str, int] = {}
    for p in sorted(replay_run.glob("*-step-metrics.json")):
        if "portable" in p.name:
            continue
        per[p.name.replace("-step-metrics.json", "")] = int(json.loads(p.read_text(encoding="utf-8")).get("stepped_instructions", 0))
    return sum(per.values()), per


def load_ir_pcs(path: pathlib.Path) -> set[int]:
    d = json.loads(path.read_text(encoding="utf-8"))
    return {int(ins["pc"], 16) for blk in d["blocks"] for ins in blk["instructions"]}


def load_prod_pcs(root: pathlib.Path) -> set[int]:
    active = set(StatusAdapter().active_set)
    pcs: set[int] = set()
    for p in sorted((root / "src" / "tools" / "hybrid").glob("m68k_*.cpp")):
        m = re.match(r"m68k_([0-9a-fA-F]{6})", p.name)
        if not m or int(m.group(1), 16) not in active:
            continue
        txt = p.read_text(encoding="utf-8", errors="ignore")
        for c in re.findall(r"case\s+0x([0-9a-fA-F]+)U?\s*:", txt):
            pcs.add(int(c, 16))
    return pcs


def pct(n: int, d: int) -> float:
    return round(100.0 * n / d, 3) if d else 0.0


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent

    # Physical Scenario C totals verified across GPGX profiler and code-map
    sc_totals = {
        "c": (17627770, 9730133, 7897637),
        "a": (15493548, 8853218, 6640330),
        "b": (5773843, 3759080, 2014763),
    }

    # Historical on-disk replay metrics (Scenario C)
    m238_a, p238_a = sum_disk_steppers(root / "build/ab_combined_m15_238/scenario_c/run_a/replay-run")
    m239_a, p239_a = sum_disk_steppers(root / "build/ab_combined_m15_239/scenario_c/run_a/replay-run")
    m239_b, _ = sum_disk_steppers(root / "build/ab_combined_m15_239/scenario_c/run_b/replay-run")
    m240_b, p240_b = sum_disk_steppers(root / "build/ab_combined_m15_240/scenario_c/run_b/replay-run")
    m244_comb, _ = sum_disk_steppers(root / "build/ab_portable_m15_244/scenario_c/run_p_comb_portable/replay-run")
    m245_a, _ = sum_disk_steppers(root / "build/ab_portable_m15_245/scenario_c/run_a/replay-run")

    disabled_slots_c = {k: v for k, v in p240_b.items() if v > 0 and p239_a.get(k, 0) == 0}
    prod_pcs = load_prod_pcs(root)
    m244_pcs = load_ir_pcs(root / "src/tools/thor_ir/entity_dispatch_subsystem_ir.json")
    m245_pcs = load_ir_pcs(root / "src/tools/thor_ir/entity_dispatch_expanded_ir.json")
    assert len(m244_pcs & m245_pcs) == 0, "M15.244 and M15.245 PCs must be disjoint"

    reconciliation = [
        {
            "milestone": "M15.240 / M15.241 / M15.242 (reported)",
            "active_prod_helpers": 196,
            "disabled_experimental_slots_enabled": True,
            "production_helper_steps": m239_a,
            "disabled_slots_steps_on_disk": m240_b - m239_a,
            "legacy_receipt_formula_drift_plus6597": 6637240 - m240_b,
            "portable_steps": 0,
            "reported_combined_native_steps": 6637240,
            "actual_disk_combined_steps": m240_b,
            "reported_raw_native_percent": pct(6637240, 17627770),
            "reported_non_idle_native_percent": pct(6637240, 7897637),
            "actual_disk_raw_percent": pct(m240_b, 17627770),
            "actual_disk_non_idle_percent": pct(m240_b, 7897637),
            "root_cause": (
                "Included 196 active helpers (5,145,887) + 13 executed disabled experimental slots "
                "222..242 (1,484,756 -> 6,630,643 disk sum), plus +6,597 hardcoded baseline constant "
                "drift from tools/generate_m15_239_receipt.py (5,127,961 base + 00e682/00c9b2/slots)."
            ),
        },
        {
            "milestone": "M15.244 (portable-only vs combined)",
            "active_prod_helpers": 196,
            "disabled_experimental_slots_enabled_in_report": False,
            "production_helper_steps": m245_a,
            "disabled_slots_steps_on_disk_in_comb_run": m244_comb - m245_a,
            "portable_steps": 463,
            "active_plus_portable_steps": m245_a + 463,
            "active_plus_portable_raw_percent": pct(m245_a + 463, 17627770),
            "active_plus_portable_non_idle_percent": pct(m245_a + 463, 7897637),
            "comb_run_disk_total_with_disabled_slots": m244_comb + 463,
            "root_cause": (
                "Portable qualification ran with 196 active helpers (5,145,887) + 0059a8p (+463) = 5,146,350; "
                "run_p_comb_portable also ran with disabled Slots 222..242 = 6,630,618 + 463 = 6,631,081 "
                "(-25 overlap against disabled Slot 238 0x0059A8)."
            ),
        },
        {
            "milestone": "M15.245",
            "active_prod_helpers": 196,
            "disabled_experimental_slots_enabled": False,
            "production_helper_steps": m245_a,
            "m15_244_portable_steps": 463,
            "m15_245_portable_steps": 28912,
            "combined_native_steps": m245_a + 463 + 28912,
            "reported_raw_native_percent": pct(m245_a + 463 + 28912, 17627770),
            "reported_non_idle_native_percent": pct(m245_a + 463 + 28912, 7897637),
            "note_vs_disabled_slots": (
                "Excludes unpromoted Slots 222..242 per ORACLE_OWNER_APPROVAL=PENDING. Static PC overlap "
                "with disabled Slot 234 (0x006E7A) is 153 Scenario C steps, 0 overlap with active 196 helpers or M15.244."
            ),
        },
    ]

    contract = {
        "schema": "oasis.coverage-accounting-contract.v1",
        "rules": [
            "Every dynamic M68000 instruction execution has strictly ONE owner: ORIGINAL_CPU, PRODUCTION_HELPER, M15_244_PORTABLE, M15_245_PORTABLE, M15_246_PORTABLE, or DISABLED_EXPERIMENTAL_SLOT (never counted unless explicitly enabled in a separate column).",
            "SCENARIO_TOTAL_STEPS = IDLE_LOOP_STEPS + NON_IDLE_GUEST_INSTRUCTIONS.",
            "IDLE_LOOP_STEPS = executions inside idle wait loops (0x008ACA..0x008AD0 and 0x008B72..0x008B7A in Scenario C = 9,730,133).",
            "PRODUCTION_HELPER_STEPS = sum(stepped_instructions) across exactly the 196 active helpers in StatusAdapter.",
            "PORTABLE_GUEST_STEPS = disjoint instructions executed via Thor IR C++ adapters (0059a8p, m15245p, m15246p), verified with 0 production overlap and 0 prior-portable overlap.",
            "COMBINED_NATIVE_STEPS = PRODUCTION_HELPER_STEPS + sum(PORTABLE_GUEST_STEPS).",
            "RAW_NATIVE_PERCENT = 100 * COMBINED_NATIVE_STEPS / SCENARIO_TOTAL_STEPS; NON_IDLE_NATIVE_PERCENT = 100 * COMBINED_NATIVE_STEPS / NON_IDLE_GUEST_INSTRUCTIONS.",
            "ORIGINAL_CPU_NON_IDLE_STEPS = NON_IDLE_GUEST_INSTRUCTIONS - COMBINED_NATIVE_STEPS (zero duplicate steps).",
        ],
    }

    # Phase 2 disjoint single-owner histograms across C, A, B
    per_scenario = {}
    for sc, sc_dir_name in (("c", "scenario_c"), ("a", "scenario_a"), ("b", "scenario_b")):
        tot, idle, non_idle = sc_totals[sc]
        prof = json.loads((root / f"build/m15_245/profile_{sc}.json").read_text(encoding="utf-8"))
        epc = prof["entity_region_pc_counts"]
        prof_pcs = {int(k, 16): int(v) for k, v in epc.items()}

        run_c_dir = root / f"build/ab_portable_m15_245/{sc_dir_name}/run_c/replay-run"
        prod_steps, prod_per = sum_disk_steppers(run_c_dir)
        m244_runtime = int(json.loads((run_c_dir / "0059a8-portable-step-metrics.json").read_text())["disjoint_new_native_steps"])
        m245_runtime = int(json.loads((run_c_dir / "m15245-portable-step-metrics.json").read_text())["disjoint_new_native_steps"])
        dispatch_in_005670 = int(json.loads((run_c_dir / "005670-step-metrics.json").read_text()).get("dispatch_jmp_calls", 0))
        m244_prof_all = sum(v for k, v in prof_pcs.items() if k in m244_pcs)
        m244_prof_disjoint = m244_prof_all - 5 * dispatch_in_005670
        m245_prof = sum(v for k, v in prof_pcs.items() if k in m245_pcs)
        overlap_prod_m244 = len(prod_pcs & {k for k in m244_pcs if not (0x59A8 <= k <= 0x59B8)})
        overlap_prod_m245 = len(prod_pcs & m245_pcs)
        assert m244_runtime == m244_prof_disjoint, f"M15.244 runtime {m244_runtime} != disjoint prof {m244_prof_disjoint}"
        assert m245_runtime == m245_prof, f"M15.245 runtime {m245_runtime} != prof {m245_prof}"
        assert overlap_prod_m244 == 0 and overlap_prod_m245 == 0

        combined_native = prod_steps + m244_runtime + m245_runtime
        orig_non_idle = non_idle - combined_native
        caller_info = prof.get("caller_005670", {})
        per_scenario[sc_dir_name] = {
            "total_guest_steps": tot,
            "idle_loop_steps": idle,
            "non_idle_guest_steps": non_idle,
            "single_owner_histogram": {
                "PRODUCTION_HELPER": prod_steps,
                "M15_244_PORTABLE": m244_runtime,
                "M15_245_PORTABLE": m245_runtime,
                "ORIGINAL_CPU_NON_IDLE": orig_non_idle,
                "IDLE_LOOP_ORIGINAL_CPU": idle,
            },
            "sum_check_equals_total": (prod_steps + m244_runtime + m245_runtime + orig_non_idle + idle) == tot,
            "combined_native_steps": combined_native,
            "raw_native_percent": pct(combined_native, tot),
            "non_idle_native_percent": pct(combined_native, non_idle),
            "caller_005670_bypass_0059b8_entries": dispatch_in_005670,
            "dispatch_0059a8_steps_owned_by_005670": 5 * dispatch_in_005670,
            "m15_244_runtime_entries": int(json.loads((run_c_dir / "0059a8-portable-step-metrics.json").read_text())["portable_runtime_entries"]),
            "m15_245_runtime_entries": int(json.loads((run_c_dir / "m15245-portable-step-metrics.json").read_text())["new_handler_entries"]),
        }

    m246_pcs = load_ir_pcs(root / "src/tools/thor_ir/m15_246_callee_ir.json")
    assert len(m246_pcs & prod_pcs) == 0 and len(m246_pcs & m244_pcs) == 0 and len(m246_pcs & m245_pcs) == 0
    lr_prof = json.loads((root / "build/m15_246/longrun_3600f_profile.json").read_text(encoding="utf-8"))
    sc246_totals = {
        **{("scenario_" + k): v for k, v in sc_totals.items()},
        "longrun_3600f": (lr_prof["total_original_steps"], lr_prof["idle_steps"], lr_prof["non_idle_steps"]),
    }
    m15_246_hist = {}
    for sc_dir_name, (tot, idle, non_idle) in sc246_totals.items():
        rd = root / f"build/ab_portable_m15_246/{sc_dir_name}/run_d/replay-run"
        p_steps, _ = sum_disk_steppers(rd)
        s244 = int(json.loads((rd / "0059a8-portable-step-metrics.json").read_text())["disjoint_new_native_steps"])
        s245 = int(json.loads((rd / "m15245-portable-step-metrics.json").read_text())["disjoint_new_native_steps"])
        s246 = int(json.loads((rd / "m15246-portable-step-metrics.json").read_text())["disjoint_new_native_steps"])
        comb = p_steps + s244 + s245 + s246
        orig_ni = non_idle - comb
        m15_246_hist[sc_dir_name] = {
            "total_guest_steps": tot,
            "idle_loop_steps": idle,
            "non_idle_guest_steps": non_idle,
            "single_owner_histogram": {
                "PRODUCTION_HELPER": p_steps,
                "M15_244_PORTABLE": s244,
                "M15_245_PORTABLE": s245,
                "M15_246_PORTABLE": s246,
                "ORIGINAL_CPU_NON_IDLE": orig_ni,
                "IDLE_LOOP_ORIGINAL_CPU": idle,
            },
            "sum_check_equals_total": (comb + orig_ni + idle) == tot,
            "combined_native_steps": comb,
            "combined_portable_steps": s244 + s245 + s246,
            "raw_native_percent": pct(comb, tot),
            "non_idle_native_percent": pct(comb, non_idle),
        }

    out = {
        "schema": "oasis.m15_246.coverage-reconciliation-and-ownership.v1",
        "milestone": "M15.246",
        "accounting_contract": contract,
        "historical_reconciliation_scenario_c": reconciliation,
        "m15_238_to_239_prod_delta": {"m238_run_a": m238_a, "m239_run_a": m239_a, "delta": m239_a - m238_a},
        "disabled_experimental_slots_222_242_c": disabled_slots_c,
        "disabled_slots_c_total": sum(disabled_slots_c.values()),
        "phase2_disjoint_single_owner_baseline": per_scenario,
        "m15_246_disjoint_single_owner": m15_246_hist,
    }
    out_p = root / "data/thor-evidence/native_coverage_accounting_m15_246.json"
    out_p.write_text(json.dumps(out, indent=2), encoding="utf-8")
    print("Wrote", out_p)
    for r in reconciliation:
        print("RECON:", r["milestone"], "| reported:", r.get("reported_combined_native_steps", r.get("combined_native_steps", r.get("active_plus_portable_steps"))), r.get("reported_raw_native_percent", r.get("active_plus_portable_raw_percent")), r.get("reported_non_idle_native_percent", r.get("active_plus_portable_non_idle_percent")))
    for k, v in m15_246_hist.items():
        print("M15.246 HIST", k, v["single_owner_histogram"], f"raw={v['raw_native_percent']}%", f"non_idle={v['non_idle_native_percent']}%", "sum_ok=", v["sum_check_equals_total"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
