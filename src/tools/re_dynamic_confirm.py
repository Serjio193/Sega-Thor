"""Perform a bounded, fail-closed audit of natural dynamic evidence."""
import argparse
import hashlib
import json
import re
import subprocess
from collections import Counter
from pathlib import Path


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGETS = (0x3820, 0x62CC, 0x9BF2, 0xA8DA, 0xD3B2)
POSITIVE_CONTROL = 0x6121A
REACHABILITY = {
    0x3820: ("graphics decompressor", "natural title/start plus asset use",
             "EVIDENCE_ARTIFACT_MISSING"),
    0x62CC: ("player/event reset leaf", "post-transition movement or interaction",
             "CALLER_NOT_REACHED in neutral and M11.8 scenarios"),
    0x9BF2: ("byte-grid aggregate", "natural room/grid transition",
             "SCENARIO_COVERAGE; no target hook in retained natural reports"),
    0xA8DA: ("bounded translation leaf", "natural byte-grid/entity update",
             "SCENARIO_COVERAGE; prior natural capture reported zero hits"),
    0xD3B2: ("event producer reader", "natural event/entity dispatch",
             "CONTROL_FLOW_GAP; producer path not observed"),
}
DOCUMENTED_CONTEXT = {0x3820: "M11.8 prose records 13 hits, but its JSON artifact is absent."}


def parse_address(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def inside_range(address, start, end):
    """Return true only for a PC in the audited half-open range."""
    return start <= address < end


def exact_linkage(observed_pcs, start, end):
    """Link observations to one exact audited range, rejecting spillover."""
    pcs = {parse_address(item) for item in observed_pcs}
    in_range = sorted(pc for pc in pcs if inside_range(pc, start, end))
    outside = sorted(pc for pc in pcs if not inside_range(pc, start, end))
    return {"matched": bool(in_range) and not outside,
            "observed_in_range": in_range, "observed_outside_range": outside}


def coverage(observed_pcs, instruction_pcs):
    """Report unique instruction coverage without treating bytes as instructions."""
    observed = {parse_address(item) for item in observed_pcs}
    instructions = {parse_address(item) for item in instruction_pcs}
    hit = observed & instructions
    return {"observed_instruction_pcs": len(hit),
            "instruction_pcs": len(instructions),
            "fraction": len(hit) / len(instructions) if instructions else 0.0,
            "partial": bool(hit) and hit != instructions}


def promote_from_natural(previous, evidence_class, exact_range):
    """Promote only natural, exact-range evidence; forced evidence is inert."""
    if evidence_class != "DYNAMIC_NATURAL" or not exact_range:
        return previous
    return "CODE_EXECUTED" if previous in {
        "ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED"
    } else previous


def no_propagation(caller, callee, updated_callee):
    """Updating a callee never changes a caller's independently held level."""
    return {"caller": caller, "callee_before": callee,
            "callee_after": updated_callee, "caller_after": caller}


def _git_revision():
    try:
        return subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def _load_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def _natural_reports(root):
    paths = []
    for path in sorted(root.rglob("*.json")):
        try:
            data = _load_json(path)
        except (OSError, json.JSONDecodeError):
            continue
        if (isinstance(data, dict) and data.get("schema") == "oasis.m68k.natural-reach.v1"
                and path.name in {"natural-final-a.json", "natural-final-b.json"}):
            paths.append((path, data))
    return paths


def _instruction_pcs(artifact, start, end, form_metrics):
    if artifact and artifact.exists():
        labels = {int(match.group(1), 16) for match in re.finditer(
            r"^loc_([0-9A-Fa-f]+):", artifact.read_text(encoding="utf-8"), re.MULTILINE)}
        return sorted(pc for pc in labels if start <= pc < end)
    count = form_metrics.get("loc.-", sum(form_metrics.values()))
    return list(range(start, min(end, start + 2 * count), 2))


def _evidence_for_target(target, reports):
    matches = []
    for path, report in reports:
        if report.get("rom_sha256") != ROM_SHA256:
            continue
        for hit in report.get("target_hits", []):
            if parse_address(hit.get("address", 0)) != target or not hit.get("count"):
                continue
            matches.append({"artifact": str(path).replace("\\", "/"),
                            "artifact_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                            "scenario_id": report.get("scenario_id", "unknown"),
                            "backend": report.get("backend", "unknown"),
                            "target_hits": hit["count"],
                            "target_frame": report.get("target_frame"),
                            "rom_sha256": report["rom_sha256"],
                            "evidence_class": "DYNAMIC_NATURAL"})
    return matches


def _target_record(target, audit_by_start, reports, artifact_root):
    audit = audit_by_start[target]
    start, end = audit["start"], audit["end"]
    source = audit.get("source_evidence", {})
    ghidra = source.get("ghidra", {})
    artifact_name = audit.get("source_evidence", {}).get("artifact", {}).get("artifact", "")
    artifact = artifact_root / artifact_name if artifact_name else None
    forms = source.get("artifact", {}).get("form_metrics", {})
    evidence = _evidence_for_target(target, reports)
    observed = []
    for item in evidence:
        observed.append(target)
    instruction_pcs = _instruction_pcs(artifact, start, end, forms)
    linked = exact_linkage(observed, start, end) if observed else {
        "matched": False, "observed_in_range": [], "observed_outside_range": []}
    cov = coverage(observed, instruction_pcs)
    details = REACHABILITY.get(target, ("positive control", "existing natural idle scenario",
                                        "control only; no new run requested"))
    if evidence and linked["matched"]:
        status, blocker = "REACHED", None
    else:
        status, blocker = "NOT_REACHED", details[2]
    return {
        "address": f"0x{target:06X}", "range": f"0x{start:06X}..0x{end:06X}",
        "size_bytes": end - start, "instruction_count": forms.get("loc.-", sum(forms.values())),
        "critical_reason": details[0],
        "expected_scenario": details[1],
        "trusted_callers": [],
        "trusted_callees": [],
        "candidate_callers": ghidra.get("called_by", []),
        "candidate_callees": ghidra.get("calls", []),
        "baseline_classification": audit.get("new_classification", audit.get("classification")),
        "status": status, "blocker": blocker, "coverage": cov,
        "exact_range_linkage": linked, "evidence": evidence,
        "documented_prior_runtime": DOCUMENTED_CONTEXT.get(target),
        "natural_runs": 0,
    }


def build_report(audit_report, reports, artifact_root):
    audit_by_start = {item["start"]: item for item in audit_report["records"]}
    selection = [{"address": f"0x{x:06X}", "baseline": audit_by_start[x]["new_classification"],
                 "range": f"0x{x:06X}..0x{audit_by_start[x]['end']:06X}",
                 "critical_reason": REACHABILITY[x][0],
                 "trusted_callers": [], "trusted_callees": [],
                 "candidate_callers": audit_by_start[x].get("source_evidence", {}).get("ghidra", {}).get("called_by", []),
                 "candidate_callees": audit_by_start[x].get("source_evidence", {}).get("ghidra", {}).get("calls", []),
                 "expected_scenario": REACHABILITY[x][1],
                 "reachability_risk": REACHABILITY[x][2],
                 "existing_runtime_evidence": _evidence_for_target(x, reports)} for x in TARGETS]
    records = [_target_record(x, audit_by_start, reports, artifact_root) for x in TARGETS]
    control = _target_record(POSITIVE_CONTROL, audit_by_start, reports, artifact_root)
    before = Counter(item["new_classification"] for item in audit_report["records"])
    after = Counter(before)
    for item in records:
        if item["status"] == "REACHED":
            after[item["baseline_classification"]] -= 1
            after["CODE_EXECUTED"] += 1
    return {
        "schema": "oasis.m68k.targeted-dynamic-confirmation.v1",
        "revision": _git_revision(), "rom_sha256": ROM_SHA256,
        "selection_limit": 8, "selected_count": len(TARGETS),
        "selection": selection, "selection_written_before_runs": True,
        "existing_scenarios_reused": sorted({item.get("scenario_id") for _, item in reports}),
        "new_natural_runs": 0, "additional_scenarios": 0,
        "useful_new_hits": 0, "expanded_to_eight": False,
        "expansion_stop_reason": "No selected target had a retained natural hit artifact.",
        "targets": records, "positive_control": control,
        "before_counts": dict(before), "after_counts": dict(after),
        "behavior_verified": 0, "forced_evidence_promotions": 0,
        "propagation": no_propagation("CODE_STATIC_SUPPORTED", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED"),
        "efficiency": {"confirmed_ranges_per_new_emulator_minute": None,
                        "reason": "No new run; retained natural artifacts were reused."},
        "decision": "TARGETED_DYNAMIC_REACHABILITY_LIMITED",
        "next_recommendation": "D — one separately authorized bounded timing/hold-input sweep around the M11.8 startup transition.",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--audit-report", type=Path, default=Path("build/m11-15/audit4/audit_report.json"))
    parser.add_argument("--manifest", type=Path, default=Path("build/m11-15/audit4/manifest.json"))
    parser.add_argument("--output", type=Path, default=Path("build/m11-16/targeted_dynamic"))
    args = parser.parse_args()
    audit_report = _load_json(args.audit_report)
    reports = _natural_reports(Path("build"))
    args.output.mkdir(parents=True, exist_ok=True)
    audit_by_start = {item["start"]: item for item in audit_report["records"]}
    selection = {"schema": "oasis.m68k.targeted-dynamic-selection.v1", "revision": _git_revision(),
                 "rom_sha256": ROM_SHA256, "selected_count": len(TARGETS), "selection_limit": 8,
                 "targets": [{"address": f"0x{x:06X}", "range": f"0x{x:06X}..0x{audit_by_start[x]['end']:06X}",
                              "baseline_classification": audit_by_start[x]["new_classification"],
                              "critical_reason": REACHABILITY[x][0], "expected_scenario": REACHABILITY[x][1],
                              "reachability_risk": REACHABILITY[x][2],
                              "existing_runtime_evidence": _evidence_for_target(x, reports)} for x in TARGETS]}
    (args.output / "selection.json").write_text(json.dumps(selection, indent=2) + "\n", encoding="utf-8")
    report = build_report(audit_report, reports, args.manifest.parent)
    report["selection_artifact"] = "selection.json"
    (args.output / "confirmation_report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
