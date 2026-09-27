"""R5 in-process post-run coordinator contracts."""
from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
import live_forward_postrun_coordinator as coordinator  # noqa: E402


class CoordinatorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(); root = Path(self.temp.name)
        self.root = root; self.receipt = root / "receipt.json"
        self.receipt.write_text(json.dumps({"runtime": {"run_id": 7},
            "raw_segment_spool": {"segments": 2}}), encoding="utf-8")
        startup = mock.Mock(); startup.master_sha256 = "a" * 64
        self.context = coordinator.PostRunContext(
            run_id=7, rom_path=root / "rom.md", master_root=root / "master",
            receipt_path=self.receipt, decoder=None, startup=startup,
            segment_total=2, configuration={})

    def tearDown(self) -> None:
        self.temp.cleanup()

    @staticmethod
    def _fake_compact(_receipt, _master, _rom, *_args, progress=None, **_kwargs):
        progress.start("FINALIZING RUN", 1, "receipt")
        progress.update(1)
        progress.finish("PASS")
        progress.start("AUDITING FLOW", 2, "FLOW records")
        progress.update(2)
        progress.finish("PASS")
        progress.complete({"pipeline_state": "ANALYSIS COMPLETE ✓"})
        return {"pipeline_state": "ANALYSIS COMPLETE ✓", "stage_results": {
            "FINALIZING RUN": {"state": "PASS"}, "AUDITING FLOW": {"state": "PASS"}},
            "source_owned_delta": 0}

    def test_a_b_c_h_i_j_k_in_process_without_status_or_report(self) -> None:
        with mock.patch.object(coordinator, "compact", self._fake_compact):
            result = coordinator.PostRunCoordinator().run(self.context)
        self.assertTrue(result.complete)
        self.assertEqual(result.report["source_owned_delta"], 0)
        self.assertEqual([event["event"] for event in result.events],
                         ["stage_started", "stage_progress", "stage_completed",
                          "stage_started", "stage_progress", "stage_completed",
                          "pipeline_complete"])
        self.assertFalse((self.root / "status.json").exists())
        self.assertFalse((self.root / "report.json").exists())

    def test_d_e_background_completion_is_independent_of_window(self) -> None:
        with mock.patch.object(coordinator, "compact", self._fake_compact):
            owner = coordinator.PostRunCoordinator()
            thread = owner.run_background(self.context)
            thread.join(5)
        self.assertFalse(thread.is_alive())
        self.assertIsNotNone(owner.result)
        self.assertTrue(owner.result.complete)

    def test_f_g_stop_and_unexpected_error_leave_no_active_stage(self) -> None:
        def stop(_receipt, _master, _rom, *_args, progress=None, **_kwargs):
            progress.start("CONTROL PROVENANCE", 2, "FLOW")
            progress.finish("STOP", "STOP_CONTROL_PROVENANCE")
            progress.complete({"pipeline_state": "ANALYSIS STOPPED ✗",
                               "stop": "STOP_CONTROL_PROVENANCE"})
            return {"pipeline_state": "ANALYSIS STOPPED ✗", "stop": "STOP_CONTROL_PROVENANCE",
                    "stage_results": {"CONTROL PROVENANCE": {"state": "STOP"}}}

        with mock.patch.object(coordinator, "compact", stop):
            result = coordinator.PostRunCoordinator().run(self.context)
        self.assertEqual(result.overall_status, "ANALYSIS STOPPED ✗")
        self.assertIsNone(result.active_stage)

        with mock.patch.object(coordinator, "compact", side_effect=RuntimeError("boom")):
            result = coordinator.PostRunCoordinator().run(self.context)
        self.assertEqual(result.overall_status, "ANALYSIS FAILED ✗")
        self.assertIsNone(result.active_stage)

    def test_stop_snapshot_is_terminal_and_not_complete(self) -> None:
        def stop(_receipt, _master, _rom, *_args, progress=None, **_kwargs):
            progress.start("CONTROL PROVENANCE", 1, "FLOW")
            progress.finish("STOP", "STOP_CONTROL_PROVENANCE")
            progress.complete({"pipeline_state": "ANALYSIS STOPPED ✗",
                               "stop": "STOP_CONTROL_PROVENANCE"})
            return {"pipeline_state": "ANALYSIS STOPPED ✗", "stop": "STOP_CONTROL_PROVENANCE"}

        status = self.root / "stop-status.json"
        with mock.patch.object(coordinator, "compact", stop):
            result = coordinator.PostRunCoordinator().run(
                self.context, coordinator.ProgressEventSink(7, status))
        self.assertEqual(result.overall_status, "ANALYSIS STOPPED ✗")
        snapshot = json.loads(status.read_text(encoding="utf-8"))
        self.assertEqual(snapshot["pipeline_state"], "STOPPED")
        self.assertEqual(snapshot["overall_state"], "ANALYSIS STOPPED ✗")

    def test_subprocess_shadow_projection_ignores_volatile_generation_hashes(self) -> None:
        left = {"generation_id": "master-old", "master_sha256": "a" * 64,
                "stage_results": {"CONTROL PROVENANCE": {"state": "NO_DELTA"}},
                "new_instructions": 0, "new_edges": 2, "terminal_facts": 3,
                "source_owned_delta": 0}
        right = {"generation_id": "master-old", "master_sha256": "b" * 64,
                 "stage_results": {"CONTROL PROVENANCE": {"state": "NO_DELTA"}},
                 "new_instructions": 0, "new_edges": 2, "terminal_facts": 3,
                 "source_owned_delta": 0}
        self.assertTrue(coordinator.semantic_equal(left, right))

    def test_analysis_performance_report_records_wall_and_r7_invariants(self) -> None:
        report = {"flow_handoff": {"mode": "IN_MEMORY_STREAM", "segments": 20,
                                    "records": 100, "disk_reads": 0, "disk_writes": 0,
                                    "buffer_peak_chunks": 2, "buffer_peak_bytes": 64,
                                    "ram_peak_bytes": 4096}, "source_owned_delta": 0}
        rows = {"REFRESHING MAP": {"duration_seconds": 2.0},
                "ASM CLOSURE": {"duration_seconds": 3.0}}
        coordinator.attach_analysis_performance(report, rows,
                                                 {"segments": 20, "records": 100,
                                                  "chunks_transferred": 20,
                                                  "buffer_peak_chunks": 2,
                                                  "buffer_peak_bytes": 64})
        performance = report["analysis_performance"]
        self.assertEqual(performance["analysis_wall_seconds"], 5.0)
        self.assertEqual(performance["segments_per_second"], 4.0)
        self.assertEqual(performance["r7_invariants"]["flow_disk_reads"], 0)
        self.assertEqual(performance["r7_invariants"]["flow_disk_writes"], 0)
        self.assertEqual(performance["ram_peak_bytes"], 4096)


if __name__ == "__main__":
    unittest.main(verbosity=2)
