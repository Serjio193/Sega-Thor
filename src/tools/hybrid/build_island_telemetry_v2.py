"""Build a fail-closed V2 telemetry receipt from frozen replay traces."""
import argparse
import hashlib
import importlib.util
import json
import re
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
HELPER_TRACE = ROOT / "src/tools/hybrid/normalize_step_helper_trace.py"
EXPECTED_HELPERS = [0x9D70, 0x9D4E, 0x9DB4, 0x9D92, 0x94A2, 0x9C40, 0x8EF8, 0xE268]
EXPECTED_NATIVE = [184, 113, 507, 24, 66, 24, 3, 45047]
RUNS = {
    "debug-1-normalized": "Debug",
    "debug-2": "Debug",
    "release-current-1": "Release",
    "release-current-2": "Release",
    "debug-telemetry-v2": "Debug",
}


def digest(path):
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def line_count(path):
    with path.open("rb") as stream:
        return sum(1 for line in stream if line.strip())


def state_hash_count(path):
    with path.open(encoding="utf-8") as stream:
        return sum(1 for line in stream if json.loads(line).get("state_sha256"))


def load_trace_module():
    spec = importlib.util.spec_from_file_location("step_trace_normalizer", HELPER_TRACE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def rows_from_jsonl(path):
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            row = json.loads(line)
            if row.get("kind") == "STEP_INSTRUCTION":
                yield row


def timing_summary(paths, helper):
    observations = [(key, path) for key, values in paths.items()
                    if key[0] == helper for path in values]
    vectors = set()
    cycles = []
    refresh_values = []
    path_rows = []
    for (entry, identifier), values in sorted(paths.items()):
        if entry != helper:
            continue
        cycle_vectors = sorted({tuple(item["cycle_sequence_to_rts"]) for item in values})
        path_cycles = sorted({item["total_cycles"] for item in values})
        flags = [bool(item["refresh_before_rts"]) for item in values]
        vectors.update((tuple(item["cycle_sequence_to_rts"]), item["total_cycles"],
                        bool(item["refresh_before_rts"])) for item in values)
        cycles.extend(path_cycles)
        refresh_values.extend(flags)
        path_rows.append({
            "PATH_ID": identifier,
            "CALL_COUNT": len(values),
            "CYCLE_VECTOR_SET": [list(vector) for vector in cycle_vectors],
            "TOTAL_CYCLE_SET": path_cycles,
            "REFRESH_VARIANTS": sorted(set(flags)),
            "REFRESH_CROSSING_CALLS": sum(flags),
            "SCHEDULER_YIELDS": "UNINSTRUMENTED",
            "IRQ_PAUSES": "UNINSTRUMENTED",
        })
    return {
        "TIMING_VECTOR_COUNT": len(observations),
        "UNIQUE_TIMING_VECTORS": len(vectors),
        "MIN_CYCLES": min(cycles) if cycles else "UNINSTRUMENTED",
        "MAX_CYCLES": max(cycles) if cycles else "UNINSTRUMENTED",
        "REFRESH_VARIANTS": sorted(set(refresh_values)),
        "REFRESH_CROSSING_CALLS": sum(refresh_values) if refresh_values else "UNINSTRUMENTED",
        "PATH_ID_TELEMETRY_SCOPE": "OBSERVED_NATIVE_CALLS",
        "PATHS": path_rows,
    }


def clean_build_dependencies():
    cmake = ROOT / "src/tools/hybrid/CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8")
    match = re.search(r"add_executable\(oasis_gameplay_session\s+(.*?)\)", text, re.S)
    if not match:
        raise ValueError("oasis_gameplay_session target not found in CMake")
    source_names = re.findall(r"(?<![\w/])([\w./-]+\.cpp)", match.group(1))
    session_sources = sorted({(ROOT / "src/tools/hybrid" / name).resolve()
                              for name in source_names if not name.startswith("${")})
    session_sources = [path for path in session_sources if path.is_file()]
    core_match = re.search(r"add_library\(oasis_core\s+(.*?)\)",
                           (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"), re.S)
    if not core_match:
        raise ValueError("oasis_core target not found in top-level CMake")
    core_names = re.findall(r"(src/[\w./-]+\.cpp)", core_match.group(1))
    core_sources = [(ROOT / name).resolve() for name in core_names]
    if not all(path.is_file() for path in core_sources):
        raise ValueError("missing a declared oasis_core source")
    sources = session_sources + core_sources
    headers = set()
    pending = list(sources)
    include_re = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]', re.M)
    while pending:
        source = pending.pop()
        try:
            contents = source.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for include in include_re.findall(contents):
            candidate_paths = [source.parent / include, ROOT / "src" / include, ROOT / include]
            found = next((candidate.resolve() for candidate in candidate_paths
                          if candidate.is_file() and candidate.suffix in {".h", ".hpp"}), None)
            if found and found not in headers:
                headers.add(found)
                pending.append(found)
    tests = [
        "tests/gameplay_session_test.cpp",
        "tests/gameplay_session_input_sources_test.cpp",
        "tests/native_island_runtime_test.cpp",
        "tests/gameplay_session_replay_diagnostic_test.cpp",
    ]
    return {
        "SOURCE_FILES": [path.relative_to(ROOT).as_posix() for path in session_sources],
        "OASIS_CORE_SOURCE_FILES": [path.relative_to(ROOT).as_posix() for path in core_sources],
        "HEADERS": sorted(path.relative_to(ROOT).as_posix() for path in headers),
        "CMAKE_BUILD_DEPENDENCIES": [
            "CMakeLists.txt (oasis_core, platform-independent core link target)",
            "src/tools/hybrid/CMakeLists.txt (oasis_gameplay_session and focused tests)",
            "cmake/project_tests.cmake",
            "External OASIS_HYBRID_GPGX_SOURCE: GENPLUS-GX 1.7.6, SHA-256 from frozen V1 identity",
            "Canonical ROM supplied outside repository; SHA-256 from frozen V1 identity",
        ],
        "TESTS": tests,
        "REPORTS": [
            "docs/reports/THOR_EIGHT_HELPER_INSTRUCTION_STEP_ISLAND_20261013.json",
            "docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json",
            "docs/reports/THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.json",
        ],
        "HEADER_CLOSURE_METHOD": "recursive quoted project-header scan of CMake-declared gameplay-session sources; angle-bracket external headers excluded",
    }


def build_receipt(baseline, replay_root):
    normalizer = load_trace_module()
    helper_map = {int(item["entry_pc"], 16): item for item in baseline["helpers"]}
    if [int(item["entry_pc"], 16) for item in baseline["helpers"]] != EXPECTED_HELPERS:
        raise ValueError("frozen helper order/set differs from V1")
    if [item["native_calls"] for item in baseline["helpers"]] != EXPECTED_NATIVE:
        raise ValueError("frozen native call counts differ from V1")

    all_enabled = set(normalizer.HELPERS)
    run_rows = []
    canonical_metrics = None
    canonical_paths = None
    source_hashes = set()
    for run_id, config in RUNS.items():
        run_dir = replay_root / run_id / "replay-run"
        calls = run_dir / "calls.jsonl"
        if not calls.is_file():
            raise FileNotFoundError(calls)
        calls_hash = digest(calls)
        source_hashes.add(calls_hash)
        _, paths, metrics = normalizer.normalize(rows_from_jsonl(calls), all_enabled)
        route = {}
        for helper in EXPECTED_HELPERS:
            captured = metrics[helper]
            frozen = helper_map[helper]
            # E268 body trace is intentionally sampled; its full counts come from the V1 receipt.
            if helper != 0xE268:
                expected = (frozen["target_entries"], frozen["native_calls"],
                            frozen["rom_fallback_calls"], frozen["provenance_rejects"])
                actual = (captured["target_entries"], captured["native_calls"],
                          captured["rom_fallback_calls"], captured["provenance_rejects"])
                if actual != expected:
                    raise ValueError(f"{run_id} route counters mismatch for {helper:06X}: {actual} != {expected}")
            route[f"0x{helper:06X}"] = captured
        summary = {
            "run_id": run_id,
            "configuration": config,
            "calls_sha256": calls_hash,
            "step_path_count": sum(1 for key in paths),
            "native_path_observations": sum(len(items) for items in paths.values()),
            "route_metrics_match_frozen_v1": all(
                helper == 0xE268 or route[f"0x{helper:06X}"]["native_calls"] ==
                helper_map[helper]["native_calls"] for helper in EXPECTED_HELPERS),
        }
        run_rows.append(summary)
        if canonical_metrics is None:
            canonical_metrics, canonical_paths = route, paths
        else:
            if route != canonical_metrics:
                raise ValueError(f"per-helper route telemetry differs in {run_id}")
            if paths != canonical_paths:
                raise ValueError(f"instruction path/timing telemetry differs in {run_id}")

    if len(source_hashes) != 1:
        raise ValueError("replay runs do not share identical calls.jsonl telemetry")

    fresh_dir = replay_root / "debug-telemetry-v2"
    reference_dir = replay_root / "debug-1-normalized"
    fresh_run = fresh_dir / "replay-run"
    reference_run = reference_dir / "replay-run"
    compared_files = [
        "full-frame-state.jsonl", "scheduler-sync.jsonl", "architectural-sync.jsonl",
        "z80-ram-sync.bin", "call-z80-ram.bin", "normalized-events.jsonl",
        "external-events.jsonl", "native-island.metrics.json", "e268-step-metrics.json",
    ]
    artifact_comparison = {}
    for name in compared_files:
        left, right = fresh_run / name, reference_run / name
        if not left.is_file() or not right.is_file():
            artifact_comparison[name] = "UNAVAILABLE"
        else:
            artifact_comparison[name] = digest(left) == digest(right)
    replay_result = {
        "run_id": "debug-telemetry-v2",
        "wall_seconds": float((fresh_dir / "wall-seconds.txt").read_text().strip()),
        "completion": (fresh_dir / "stdout.log").read_text(encoding="utf-8").strip().splitlines()[-1],
        "frame_state_records": line_count(fresh_run / "full-frame-state.jsonl"),
        "periodic_hash_records": state_hash_count(fresh_dir / "landmarks.jsonl"),
        "scheduler_sync_records": line_count(fresh_run / "scheduler-sync.jsonl"),
        "architectural_sync_records": line_count(fresh_run / "architectural-sync.jsonl"),
        "landmark_diagnostic": json.loads((fresh_run / "landmark-diagnostic.json").read_text()),
        "artifact_sha256_comparison_to_debug_1": artifact_comparison,
        "all_compared_artifacts_match": all(value is True for value in artifact_comparison.values()),
        "effective_input_sha256": digest(fresh_dir / "input.polls"),
        "input_log_matches_v1": digest(fresh_dir / "input.polls") == baseline["identity"]["input_log_sha256"],
    }
    e268_metrics = json.loads((fresh_run / "e268-step-metrics.json").read_text())
    expected_e268 = next(item for item in baseline["helpers"]
                         if int(item["entry_pc"], 16) == 0xE268)
    for actual_key, expected_key in (("native_calls", "native_calls"),
                                     ("rom_fallback_calls", "rom_fallback_calls"),
                                     ("provenance_rejects", "provenance_rejects"),
                                     ("irq_pause_count", "irq_pause_count"),
                                     ("irq_resume_count", "irq_resume_count"),
                                     ("refresh_events", "refresh_events"),
                                     ("refresh_variant_calls", "refresh_variant_calls")):
        if e268_metrics[actual_key] != expected_e268[expected_key]:
            raise ValueError(f"fresh E268 metric differs from V1: {actual_key}")
    if (replay_result["frame_state_records"] != 3600 or
            replay_result["periodic_hash_records"] != 60 or
            replay_result["scheduler_sync_records"] != 128):
        raise ValueError("fresh telemetry replay has an unexpected frame/scheduler record count")
    if not replay_result["all_compared_artifacts_match"] or not replay_result["input_log_matches_v1"]:
        raise ValueError("fresh telemetry replay differs from frozen reference artifacts")

    telemetry = {}
    coverage = {}
    synthetic_status = {0x9C40: "PARTIAL", 0x9D92: "PARTIAL"}
    for item in baseline["helpers"]:
        helper = int(item["entry_pc"], 16)
        key = f"0x{helper:06X}"
        timing = timing_summary(canonical_paths, helper)
        full_capture_count = timing["TIMING_VECTOR_COUNT"]
        e268_sampled = helper == 0xE268
        coverage[key] = {
            "CPP_PATH_VERIFIED": "YES",
            "NATURAL_REPLAY_PATHS": "YES" if timing["PATHS"] else "UNKNOWN",
            "SYNTHETIC_ORACLE_PATHS": synthetic_status.get(helper, "UNKNOWN"),
            "STEPPER_PATHS": "PARTIAL" if e268_sampled else ("YES" if timing["PATHS"] else "UNKNOWN"),
            "IRQ_PATH_OBSERVED": "YES" if helper == 0xE268 else "UNKNOWN",
            "REFRESH_VARIATION_OBSERVED": (
                "YES" if helper == 0xE268 or len(timing["REFRESH_VARIANTS"]) > 1
                else "NO" if timing["REFRESH_VARIANTS"] else "UNKNOWN"),
            "FULL_CFG_COVERAGE": "NO",
        }
        if e268_sampled:
            timing["TIMING_VECTOR_COUNT_SCOPE"] = f"SAMPLED {full_capture_count}/45047 native calls"
            timing["PATH_ID_TELEMETRY_SCOPE"] = "SAMPLED_NATIVE_CALLS_ONLY"
            timing["REFRESH_CROSSING_CALLS"] = "UNINSTRUMENTED"
        else:
            timing["TIMING_VECTOR_COUNT_SCOPE"] = "ALL_NATIVE_CALLS_IN_PINNED_REPLAY"
        telemetry[key] = {
            "NATIVE_CALL_COUNT": item["native_calls"],
            "ROM_FALLBACK_COUNT": item["rom_fallback_calls"],
            "PROVENANCE_REJECTS": item["provenance_rejects"],
            "PRECONDITION_REJECTS": item["precondition_rejects"],
            "CONTRACT_REJECTS": item["contract_rejects"],
            **timing,
            "REFRESH_EVENT_COUNT": item.get("refresh_events", "UNINSTRUMENTED"),
            "REFRESH_VARIANT_CALL_COUNT": item.get("refresh_variant_calls", "UNINSTRUMENTED"),
            "SCHEDULER_YIELD_COUNT": "UNINSTRUMENTED",
            "IRQ_PAUSE_COUNT": item.get("irq_pause_count", "UNINSTRUMENTED"),
            "IRQ_RESUME_COUNT": item.get("irq_resume_count", "UNINSTRUMENTED"),
            "STEPPER_RESUME_COUNT": (e268_metrics["scheduler_resume_count"] if helper == 0xE268
                                     else "UNINSTRUMENTED"),
            "HOST_PERFORMANCE": {
                "CALL_COUNT": "UNINSTRUMENTED",
                "STEPPER_HOST_TIME": "UNINSTRUMENTED",
                "AVERAGE_HOST_TIME": "UNINSTRUMENTED",
                "P95_HOST_TIME": "UNINSTRUMENTED",
            },
        }
        if e268_sampled:
            telemetry[key]["TIMING_VECTOR_COUNT"] = full_capture_count
            telemetry[key]["TIMING_VECTOR_COUNT_SCOPE"] = "SAMPLED_NATIVE_CALLS_ONLY; aggregate timing counter does not retain per-call vectors"

    v2 = {
        "schema": "oasis.frozen-replay-native-island-baseline.v2",
        "baseline_id": "THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013",
        "previous_baseline_id": baseline["baseline_id"],
        "island_contract_version": baseline["island_contract_version"],
        "identity": baseline["identity"],
        "replay_scope": baseline["replay_scope"],
        "frozen_helper_set": baseline["helpers"],
        "replay_acceptance": baseline["replay_acceptance"],
        "repeat_runs": run_rows,
        "V2_REPLAY_RESULT": replay_result,
        "calls_trace_sha256": next(iter(source_hashes)),
        "timing_telemetry_runs_identical": True,
        "PER_HELPER_TIMING_TELEMETRY": telemetry,
        "PATH_COVERAGE_MATRIX": coverage,
        "IRQ_REFRESH_TELEMETRY": {
            "per_helper": {key: {
                "REFRESH_EVENT_COUNT": value["REFRESH_EVENT_COUNT"],
                "REFRESH_VARIANT_CALL_COUNT": value["REFRESH_VARIANT_CALL_COUNT"],
                "REFRESH_CROSSING_CALLS": value["REFRESH_CROSSING_CALLS"],
                "IRQ_PAUSE_COUNT": value["IRQ_PAUSE_COUNT"],
                "IRQ_RESUME_COUNT": value["IRQ_RESUME_COUNT"],
                "SCHEDULER_YIELD_COUNT": value["SCHEDULER_YIELD_COUNT"],
                "STEPPER_RESUME_COUNT": value["STEPPER_RESUME_COUNT"],
            } for key, value in telemetry.items()},
            "limits": "E268 aggregate IRQ/refresh counters are exact for the replay. Seven helpers have no independent IRQ, scheduler-yield, or refresh-event counters. Path-level refresh_before_rts is a captured refresh state, not an event counter.",
        },
        "HOST_PERFORMANCE_SUMMARY": {
            "per_helper": {key: value["HOST_PERFORMANCE"] for key, value in telemetry.items()},
            "whole_replay": baseline["performance"],
            "fresh_v2_debug_replay_seconds": replay_result["wall_seconds"],
            "per_helper_host_time_status": "UNINSTRUMENTED; existing host wall-time is whole-replay only",
        },
        "CLEAN_BUILD_DEPENDENCY_SET": clean_build_dependencies(),
        "build_validation": baseline["build_and_test_results"],
        "limits": {
            "replay_scope": "pinned deterministic 3600-frame development session",
            "global_helper_verification": "NO",
            "production_runtime_active": "NO",
            "full_cfg_natural_coverage": "NO",
            "V1_immutability": "V1 JSON and report are not modified",
        },
        "authority_totals": baseline["gate_status"],
        "telemetry_caveat": "E268 instruction path telemetry is a 17-call sample; its full native call count and aggregate IRQ/refresh/resume metrics come from the V1 runtime receipt. Per-helper host timing and scheduler yield counts remain UNINSTRUMENTED.",
    }
    return v2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-v1", type=Path, default=ROOT / "docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json")
    parser.add_argument("--replay-root", type=Path, default=ROOT / "build/multi-helper-e268-20261013/frozen-repro")
    parser.add_argument("--output", type=Path, default=ROOT / "docs/reports/THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.json")
    args = parser.parse_args()
    baseline = json.loads(args.baseline_v1.read_text(encoding="utf-8"))
    receipt = build_receipt(baseline, args.replay_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "runs": len(receipt["repeat_runs"]),
                      "helpers": len(receipt["PER_HELPER_TIMING_TELEMETRY"]),
                      "native_calls": baseline["aggregate_counts"]["native_calls"],
                      "calls_trace_sha256": receipt["calls_trace_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
