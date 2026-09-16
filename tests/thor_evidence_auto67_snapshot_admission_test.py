import inspect
import sys
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_snapshot_admission import (  # noqa: E402
    SNAPSHOT_POOL_CAPACITY,
    SnapshotAdmissionProbe,
    SnapshotPool,
    admission_static_audit,
    run_admission_experiment,
)


def event(seq: int, **extra) -> dict:
    return {"epoch": 1, "seq": seq, "occurrence_id": f"epoch=1:seq={seq}",
            "kind": "RAM_WRITE_SAMPLE", "pc": "0x100", "address": "0x200",
            **extra}


class SnapshotAdmissionTest(unittest.TestCase):
    def test_bounded_experiment_proves_1_to_1_capacity(self):
        result = run_admission_experiment()
        self.assertEqual(result["snapshot_pool_capacity"], 16)
        self.assertEqual(result["occurrences_seen"], 16)
        self.assertEqual(result["occurrences_frozen"], 16)
        self.assertEqual(result["snapshots_dispatched"], 16)
        self.assertEqual(result["snapshots_completed"], 16)
        self.assertEqual(result["snapshot_pool_full_count"], 0)
        self.assertEqual(result["current_snapshot_depth"], 0)
        self.assertEqual(result["peak_snapshot_depth"], 16)

    def test_worker_and_window_metrics_are_observed_without_new_queue(self):
        result = run_admission_experiment()
        self.assertEqual(result["worker_leases"], 16)
        self.assertEqual(result["worker_returns"], 16)
        self.assertEqual(result["workers_busy_peak"], 16)
        self.assertEqual(result["workers_idle_min"], 0)
        self.assertGreaterEqual(result["rolling_window_overwrites"], 1)
        self.assertIn("16", result["occupancy_histogram"])

    def test_occurrence_receives_immutable_snapshot_identity_and_fields(self):
        pool = SnapshotPool()
        source = event(7)
        frozen = pool.freeze(source, 1, 3, 7, 5)
        self.assertIsNotNone(frozen)
        assert frozen is not None
        source["seq"] = 99
        source["kind"] = "CONFLICT"
        newer = pool.freeze(event(8), 1, 4, 8, 5)
        self.assertIsNotNone(newer)
        copy = pool.get(frozen.snapshot_identity, (1, 7))
        self.assertEqual(copy.occurrence_identity, (1, 7))
        self.assertEqual(copy.snapshot_epoch, 1)
        self.assertEqual(copy.first_sequence, 3)
        self.assertEqual(copy.latest_sequence, 7)
        self.assertEqual(copy.count, 5)
        self.assertEqual(copy, frozen)

    def test_dispatcher_event_carries_snapshot_identity_and_bounds(self):
        probe = SnapshotAdmissionProbe(worker_count=16, window_capacity=32,
                                       processing_delay=0.1)
        probe.start()
        try:
            probe.ingest(event(7), 3, 7, 5)
            item = next(item for item in probe.dispatcher.window.items
                        if item["seq"] == 7)
            self.assertEqual(item["occurrence_id"], "epoch=1:seq=7")
            self.assertTrue(item["snapshot_identity"].startswith("SNAP-"))
            self.assertEqual(item["snapshot_epoch"], 1)
            self.assertEqual(item["first_sequence"], 3)
            self.assertEqual(item["latest_sequence"], 7)
            self.assertEqual(item["count"], 5)
        finally:
            probe.stop()

    def test_snapshot_stays_unchanged_while_occurrence_waits_in_window(self):
        probe = SnapshotAdmissionProbe(worker_count=16, window_capacity=32,
                                       processing_delay=0.2)
        probe.start()
        try:
            for seq in range(16):
                probe.ingest(event(seq), seq, seq + 10, 11)
            time.sleep(0.01)
            frozen = probe._snapshots[(1, 15)]
            before = probe.pool.get(frozen.snapshot_identity, (1, 15))
            self.assertEqual(probe.pool.snapshot()["current_snapshot_depth"], 16)
            self.assertEqual(before.latest_sequence, 25)
            probe.wait_for_returns(16)
            self.assertEqual(before, frozen)
        finally:
            probe.stop()

    def test_pool_full_is_fail_visible_and_never_overwrites(self):
        pool = SnapshotPool()
        held = [pool.freeze(event(i), 1, i, i, 1) for i in range(16)]
        self.assertTrue(all(item is not None for item in held))
        rejected = pool.freeze(event(16), 1, 16, 16, 1)
        self.assertIsNone(rejected)
        report = pool.snapshot()
        self.assertEqual(report["snapshot_pool_full_count"], 1)
        self.assertEqual(report["occurrences_without_snapshot"], 1)
        self.assertEqual(report["current_snapshot_depth"], 16)
        self.assertIn("SNAPSHOT_POOL_FULL", [item["operation"]
                                               for item in report["occupancy_trace"]])

    def test_identity_mismatch_fails_closed(self):
        pool = SnapshotPool()
        with self.assertRaises(ValueError):
            pool.freeze({"epoch": 2, "seq": 3,
                         "occurrence_id": "epoch=2:seq=4"}, 2, 1, 3, 3)

    def test_static_audit_excludes_semantic_preworker_selection(self):
        audit = admission_static_audit()
        self.assertTrue(audit["semantic_filter_absent"])
        self.assertEqual(audit["dispatcher_admission_forbidden_tokens"], [])
        self.assertTrue(audit["existing_order_is_preserved"])
        self.assertFalse(audit["obsolete_active_claim_counter_in_admission"])
        self.assertTrue(audit["snapshot_freeze_reads_only_identity_and_slot"])
        self.assertFalse(audit["global_map_or_cartographer_in_admission"])

    def test_sampling_policy_is_mechanical_and_classes_are_explicit(self):
        lua = (ROOT / "src/tools/thor_evidence/capture/live_opportunistic.lua").read_text()
        self.assertIn('default_stride = capture_mode == "burst" and 1 or 16', lua)
        self.assertIn('append_event("RAM_WRITE_SAMPLE"', lua)
        self.assertIn('append_event("FRAME_PC"', lua)
        self.assertIn('sampling_policy', lua)
        source = inspect.getsource(SnapshotPool.freeze)
        self.assertNotIn('event.get("kind")', source)
        self.assertNotIn('event.get("address")', source)
        self.assertNotIn('event.get("pc")', source)

    def test_pool_shape_is_fixed_and_no_pending_fifo_exists(self):
        self.assertEqual(SNAPSHOT_POOL_CAPACITY, 16)
        self.assertNotIn("pending", SnapshotPool.__dict__)
        self.assertNotIn("queue", SnapshotPool.__dict__)


if __name__ == "__main__":
    unittest.main()
