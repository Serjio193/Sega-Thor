"""Deterministic A–O checks for the snapshot-only 2H Worker window."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_worker_control import LiveWorkerControlPublisher  # noqa: E402
from live_forward_worker_control_model import (  # noqa: E402
    DEFAULT_CHAIN_DEPTH, DEFAULT_WORKER_COUNT, METRIC_NAMES, PLAN_NAMES,
    EvidenceGrowthMeter, allocation_preflight, calculate_resource_budget,
    load_next_run_config, parse_live_control, process_memory_total,
    save_next_run_config, system_memory_values, validate_config,
)
from live_forward_worker_control_window import (  # noqa: E402
    close_window, read_snapshot, snapshot_for_display,
)


def live_control_line(state: int = 2, progress: int = 4) -> str:
    metrics = [0] * len(METRIC_NAMES)
    metrics[0], metrics[3], metrics[9] = 1, 1, 3
    plan = [1, 20, 65536, 2040, 2, 128, 128, 65280, 65280, 8, 8,
            64, 131072, 256, 65496, 196872]
    header = ",".join(str(item) for item in [120, 1, 20, 1, *metrics])
    next_plan = ",".join(str(item) for item in plan)
    worker = ",".join(str(item) for item in [0, state, progress, 20, 3, 2, 2, 1])
    return f"{header}|1,20,{next_plan}|0|{worker}"


class LiveWorkerControlTests(unittest.TestCase):
    def test_a_config_save_and_load_are_exact_and_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "next-run.json"
            expected = save_next_run_config(path, "64", "500")
            first = path.read_bytes()
            actual, state = load_next_run_config(path)
            self.assertEqual(expected, {"chain_depth": 500, "worker_count": 64})
            self.assertEqual(actual, expected)
            self.assertEqual(state, "SAVED")
            save_next_run_config(path, 64, 500)
            self.assertEqual(path.read_bytes(), first)

    def test_b_saving_next_run_does_not_mutate_captured_current_values(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "next-run.json"
            save_next_run_config(path, 16, 20)
            current, _ = load_next_run_config(path)
            current_workers, current_depth = current["worker_count"], current["chain_depth"]
            save_next_run_config(path, 64, 500)
            self.assertEqual((current_workers, current_depth), (16, 20))

    def test_c_next_launch_reads_saved_values(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "next-run.json"
            save_next_run_config(path, 8, 21)
            loaded, _ = load_next_run_config(path)
            self.assertEqual((loaded["worker_count"], loaded["chain_depth"]), (8, 21))

    def test_d_arbitrarily_large_positive_integer_is_not_clamped(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "next-run.json"
            workers, depth = 10**120, 10**180
            config = save_next_run_config(path, workers, depth)
            loaded, _ = load_next_run_config(path)
            self.assertEqual(config, {"worker_count": workers, "chain_depth": depth})
            self.assertEqual(loaded, config)

    def test_e_zero_negative_float_and_text_are_rejected(self) -> None:
        for workers, depth in ((0, 2), (-1, 2), (1.5, 2), ("sixteen", 2), (2, 0)):
            with self.subTest(workers=workers, depth=depth):
                with self.assertRaises(ValueError):
                    validate_config(workers, depth)

    def test_f_preflight_rejects_shortfall_with_exact_required_available_and_delta(self) -> None:
        config = validate_config("512", "10000")
        result = allocation_preflight(config, {"total_native_bytes": 9000}, 6400)
        self.assertEqual(result["status"], "REJECTED")
        self.assertEqual(result["requested_worker_count"], 512)
        self.assertEqual(result["requested_chain_depth"], 10000)
        self.assertEqual(result["required_native_bytes"], 9000)
        self.assertEqual(result["available_native_budget_bytes"], 6400)
        self.assertEqual(result["shortfall_bytes"], 2600)

    def test_f2_missing_native_plan_is_pending_not_a_false_pass(self) -> None:
        result = allocation_preflight(validate_config(2, 20), None, 700)
        self.assertEqual(result["status"], "PENDING")
        self.assertIsNone(result["required_native_bytes"])

    def test_g_launch_values_are_the_original_request_without_clamping(self) -> None:
        requested = validate_config("100000", "1000000")
        result = allocation_preflight(requested, None, 1234, "NATIVE_PLAN_REJECTED")
        self.assertEqual(result["requested_worker_count"], 100000)
        self.assertEqual(result["requested_chain_depth"], 1000000)
        self.assertEqual(result["rejection_reason"], "NATIVE_PLAN_REJECTED")

    def test_h_worker_snapshot_reconciles_state_depth_and_lifecycle(self) -> None:
        snapshot = parse_live_control(live_control_line(state=2, progress=4))
        self.assertTrue(snapshot["pool_active"])
        self.assertEqual(snapshot["workers"], [{
            "worker_id": 0, "state": "CAPTURING", "progress": 4, "depth": 20,
            "capture_starts": 3, "capture_completions": 2,
            "analyses": 2, "releases": 1}])
        with self.assertRaises(ValueError):
            parse_live_control(live_control_line(state=2, progress=21))

    def test_i_system_ram_used_and_available_reconcile(self) -> None:
        values = system_memory_values(64 * 1024, 24 * 1024)
        self.assertEqual(values["used_bytes"] + values["available_bytes"],
                         values["total_bytes"])
        self.assertIsNone(system_memory_values(100, 101)["used_bytes"])

    def test_j_distinct_process_working_sets_are_summed_once(self) -> None:
        self.assertEqual(process_memory_total([(10, 100), (11, 200), (10, 1000)]), 300)
        self.assertIsNone(process_memory_total([(10, None)]))

    def test_k_known_evidence_growth_has_exact_average_rate(self) -> None:
        meter = EvidenceGrowthMeter()
        self.assertIsNone(meter.sample(100, 10.0)["rate_bytes_per_second"])
        self.assertIsNone(meter.sample(700, 12.0)["rate_bytes_per_second"])
        result = meter.sample(1100, 15.0)
        self.assertEqual(result["state"], "MEASURED_RATE")
        self.assertEqual(result["rate_bytes_per_second"], 200.0)
        self.assertAlmostEqual(result["projected_gib_per_hour"],
                               200 * 3600 / (1024 ** 3))

    def test_l_no_evidence_growth_reports_zero_after_a_real_sample_window(self) -> None:
        meter = EvidenceGrowthMeter()
        self.assertEqual(meter.sample(2048, 1.0)["state"], "CALCULATING")
        result = meter.sample(2048, 7.0)
        self.assertEqual(result["state"], "MEASURED_RATE")
        self.assertEqual(result["rate_bytes_per_second"], 0.0)
        self.assertEqual(result["projected_gib_per_hour"], 0.0)

    def test_m_closing_window_only_removes_its_preview_and_leaves_runtime_alive(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            preview = Path(directory) / "preview.txt"
            preview.write_text("candidate", encoding="ascii")
            runtime = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(1)"])
            destroyed: list[bool] = []
            close_window(preview, lambda: destroyed.append(True))
            self.assertFalse(preview.exists())
            self.assertEqual(destroyed, [True])
            self.assertIsNone(runtime.poll())
            runtime.terminate()
            runtime.wait(timeout=2)

    def test_n_slow_window_is_not_on_the_host_update_path(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            process = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(1)"])
            publisher = LiveWorkerControlPublisher.__new__(LiveWorkerControlPublisher)
            import threading
            publisher._lock = threading.Lock()
            start = time.monotonic()
            publisher.update({}, process, start, 0)
            self.assertLess(time.monotonic() - start, 0.1)
            self.assertEqual(publisher._latest["emuhawk_pid"], process.pid)
            process.terminate()
            process.wait(timeout=2)

    def test_o_missing_or_bad_snapshot_is_a_waiting_state_not_a_window_failure(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "missing.json"
            self.assertIsNone(read_snapshot(path))
            self.assertEqual(snapshot_for_display(None), {})
            path.write_text("{broken", encoding="utf-8")
            self.assertIsNone(read_snapshot(path))
            self.assertEqual(snapshot_for_display(read_snapshot(path)), {})
            path.write_text(json.dumps({"schema": "wrong"}), encoding="utf-8")
            self.assertIsNone(read_snapshot(path))
            path.write_text(json.dumps({"schema": "oasis.m12.live-worker-control.v1",
                                        "runtime_state": "WAITING"}), encoding="utf-8")
            self.assertEqual(read_snapshot(path)["runtime_state"], "WAITING")

    def test_resource_budget_uses_exact_available_memory(self) -> None:
        budget = calculate_resource_budget(0, native_budget_cap=100,
            process_budget_cap=200, core_reserve_cap=50, system_reserve=0,
            memory_bytes_each=64)
        self.assertEqual(budget["available_physical_bytes"], 0)
        self.assertEqual(budget["native_budget_bytes"], 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
