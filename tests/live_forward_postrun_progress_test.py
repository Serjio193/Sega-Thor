"""Deterministic progress/heartbeat truth checks for post-run 2I.1."""

from __future__ import annotations

import json
from pathlib import Path
import tempfile
import time
import unittest
import sys
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_progress import (  # noqa: E402
    ProgressPublisher, STAGES, heartbeat_state, mark_failed, overall_from_stages,
    repair_terminal_snapshot, stage_percent,
)


class PostRunProgressTests(unittest.TestCase):
    def test_atomic_publish_retries_transient_windows_reader_lock(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            real_replace = __import__("os").replace
            calls = {"count": 0}

            def replace_with_transient_lock(source: str, target: str) -> None:
                calls["count"] += 1
                if calls["count"] < 3:
                    raise PermissionError(5, "reader lock")
                real_replace(source, target)

            with mock.patch("live_forward_progress.os.replace",
                            side_effect=replace_with_transient_lock):
                publisher.start("AUDITING FLOW")
                publisher.rows["AUDITING FLOW"]["processed_units"] = 1
                publisher.write(force=True)
            self.assertGreaterEqual(calls["count"], 3)
            self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["processed_units"], 1)

    def test_exact_progress_is_real_and_clamped(self) -> None:
        self.assertEqual(stage_percent({"processed_units": 18, "total_units": 29}), 62)
        self.assertEqual(stage_percent({"processed_units": 900, "total_units": 100}), 100)
        self.assertEqual(stage_percent({"processed_units": -1, "total_units": 100}), 0)

    def test_unknown_total_is_indeterminate(self) -> None:
        self.assertIsNone(stage_percent({"processed_units": 11, "total_units": None}))
        self.assertIsNone(stage_percent({"processed_units": 11}))

    def test_heartbeat_buckets_and_quiet_live_backend(self) -> None:
        self.assertEqual(heartbeat_state({"timestamp_monotonic": 99.0}, 100.0), "LIVE")
        self.assertEqual(heartbeat_state({"timestamp_monotonic": 90.0}, 100.0), "BUSY / WAITING")
        self.assertEqual(heartbeat_state({"timestamp_monotonic": 89.9}, 100.0), "NO UPDATE")

    def test_partial_pipeline_cannot_be_full_complete(self) -> None:
        rows = {stage: {"state": "PASS"} for stage in STAGES[:4]}
        rows.update({stage: {"state": "SKIPPED_NOT_APPLICABLE"} for stage in STAGES[4:]})
        self.assertEqual(overall_from_stages(rows), "PARTIAL ANALYSIS COMPLETE ⚠")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("FINALIZING RUN", total=1, unit="receipt")
            publisher.update(1)
            publisher.finish()
            publisher.skip_remaining("NOT APPLIED")
            publisher.complete()
            status = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(status["overall_state"], "PARTIAL ANALYSIS COMPLETE ⚠")

    def test_active_or_pending_prevents_complete(self) -> None:
        rows = {stage: {"state": "PASS"} for stage in STAGES}
        rows["CLEANUP"] = {"state": "PENDING"}
        self.assertEqual(overall_from_stages(rows), "ANALYSIS RUNNING…")
        rows["CLEANUP"] = {"state": "ACTIVE"}
        self.assertEqual(overall_from_stages(rows), "ANALYSIS RUNNING…")

    def test_stopped_stage_is_explicit_terminal_stop(self) -> None:
        rows = {stage: {"state": "PENDING"} for stage in STAGES}
        rows["REFRESHING MAP"] = {"state": "STOP"}
        self.assertEqual(overall_from_stages(rows), "ANALYSIS STOPPED ✗")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("REFRESHING MAP", total=1, unit="generation")
            publisher.finish("STOP", detail="STOP_POSTRUN_MAP_FACT_MISSING")
            publisher.complete()
            status = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(status["overall_state"], "ANALYSIS STOPPED ✗")

    def test_running_active_has_active_stage(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("REFRESHING MAP", total=10, unit="facts")
            status = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(status["pipeline_state"], "RUNNING")
            self.assertEqual(status["active_stage"], "REFRESHING MAP")

    def test_stale_heartbeat_with_live_pid_is_not_failure(self) -> None:
        snapshot = {"timestamp_monotonic": 1.0, "pipeline_state": "RUNNING",
                    "backend_pid": 1234, "active_stage": "REFRESHING MAP"}
        self.assertEqual(heartbeat_state(snapshot, 20.0), "NO UPDATE")
        self.assertEqual(snapshot["pipeline_state"], "RUNNING")
        self.assertEqual(snapshot["active_stage"], "REFRESHING MAP")

    def test_backend_death_error_blocks_later_stages(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("REFRESHING MAP")
            mark_failed(path, "RuntimeError: coordinator crashed")
            status = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(status["pipeline_state"], "FAILED")
            self.assertIsNone(status["active_stage"])
            self.assertEqual(status["stages"]["REFRESHING MAP"]["state"], "ERROR")
            self.assertEqual(status["stages"]["CONTROL PROVENANCE"]["state"], "BLOCKED")

    def test_partial_and_complete_terminal_states_have_no_active_stage(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            partial_path = Path(directory) / "partial.json"
            partial = ProgressPublisher(partial_path)
            partial.start("FINALIZING RUN")
            partial.finish()
            partial.skip_remaining("compact-only developer mode")
            partial_status = json.loads(partial_path.read_text(encoding="utf-8"))
            self.assertEqual(partial_status["pipeline_state"], "PARTIAL_COMPLETE")
            self.assertIsNone(partial_status["active_stage"])
            self.assertTrue(all(row["state"] != "ACTIVE" for row in partial_status["stages"].values()))

            complete_path = Path(directory) / "complete.json"
            complete = ProgressPublisher(complete_path)
            for stage in STAGES:
                complete.start(stage)
                complete.finish()
            complete.complete()
            complete_status = json.loads(complete_path.read_text(encoding="utf-8"))
            self.assertEqual(complete_status["pipeline_state"], "COMPLETE")
            self.assertIsNone(complete_status["active_stage"])

    def test_malformed_terminal_snapshot_is_repaired(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("REFRESHING MAP")
            snapshot = json.loads(path.read_text(encoding="utf-8"))
            snapshot["pipeline_state"] = "FAILED"
            snapshot["overall_state"] = "ANALYSIS FAILED ✗"
            path.write_text(json.dumps(snapshot), encoding="utf-8")
            repaired = repair_terminal_snapshot(path)
            self.assertIsNone(repaired["active_stage"])
            self.assertEqual(repaired["stages"]["REFRESHING MAP"]["state"], "ERROR")
            self.assertEqual(repaired["stages"]["CONTROL PROVENANCE"]["state"], "BLOCKED")

    def test_new_postrun_can_start_after_failed_previous_run(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            old_path = Path(directory) / "old.json"
            old = ProgressPublisher(old_path)
            old.start("REFRESHING MAP")
            old.finish("STOP", detail="STOP_POSTRUN_MAP_AUDIT_FAILED")
            new_path = Path(directory) / "new.json"
            new = ProgressPublisher(new_path)
            new.start("FINALIZING RUN")
            status = json.loads(new_path.read_text(encoding="utf-8"))
            self.assertEqual(status["pipeline_state"], "RUNNING")
            self.assertEqual(status["active_stage"], "FINALIZING RUN")
    def test_all_accepted_non_skipped_stages_allow_full_complete(self) -> None:
        rows = {stage: {"state": "PASS"} for stage in STAGES}
        self.assertEqual(overall_from_stages(rows), "ANALYSIS COMPLETE ✓")

    def test_unresolved_semantic_stage_has_distinct_completed_ui_state(self) -> None:
        rows = {stage: {"state": "PASS"} for stage in STAGES}
        rows["CONTROLLED ENTITY PROVENANCE"] = {
            "state": "UNRESOLVED", "detail": "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT"}
        self.assertEqual(overall_from_stages(rows),
                         "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.rows = rows
            publisher.complete()
            snapshot = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(snapshot["pipeline_state"], "COMPLETE_WITH_UNRESOLVED")
            self.assertEqual(snapshot["overall_state"],
                             "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE")
            self.assertEqual(snapshot["stages"]["CONTROLLED ENTITY PROVENANCE"]["detail"],
                             "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT")

    def test_atomic_snapshot_and_backend_failure(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "status.json"
            publisher = ProgressPublisher(path)
            publisher.start("AUDITING FLOW", total=10, unit="records")
            publisher.update(3, detail="quiet SQLite work")
            json.loads(path.read_text(encoding="utf-8"))
            mark_failed(path, "backend exited")
            status = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(status["overall_state"], "ANALYSIS FAILED ✗")
            self.assertEqual(status["stage_state"], "ERROR")

    def test_ui_main_view_keeps_json_in_details_only(self) -> None:
        source = (ROOT / "tools" / "bizhawk-native-ring" / "live_forward_postrun_window.py").read_text()
        self.assertIn("def show_details", source)
        self.assertIn("json.dumps({\"status\": status, \"report\": report}", source)
        self.assertIn("PARTIAL ANALYSIS COMPLETE", source)
        self.assertIn("Backend complete", source)
        self.assertIn('style.configure("TLabelframe", background=panel', source)
        self.assertIn('style.configure("Dark.Horizontal.TProgressbar"', source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
