import copy
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.cartographer import Cartographer
from thor_evidence.walker1 import make_bundle, negative_join_results, validate_runtime


class WalkerOneTest(unittest.TestCase):
    def runtime(self):
        return {
            "schema": "oasis.m12.walker1.runtime.v1",
            "run_id": "r1", "capture_id": "c1", "fragment_id": "f1",
            "phase": "DONE",
            "block": {"guard": "0x0027CA", "entry": "0x0027CE", "exit": "0x0027E2"},
            "metrics": {"w1_count": 1, "w2_count": 1, "max_w1_records": 1,
                        "max_w2_records": 2, "contract_skips": 1,
                        "global_off_body_executions": 1, "callbacks_during_b_off": 0,
                        "frame_p99_ms": 20, "frames_over_50ms": 0},
            "w1": [{"sequence": 1, "frame": 1, "pc": "0x0027C4"}],
            "w2": [{"sequence": 1, "frame": 1, "pc": "0x0027E8"}],
            "data_accesses": [{"address": "0x00C00004"}], "off_pcs": [],
        }

    def test_runtime_gate_and_negative_joins(self):
        self.assertTrue(validate_runtime(self.runtime())["passed"])
        self.assertEqual(negative_join_results(), {
            "different_restore_epoch": True, "marked_gap": True,
            "same_value_different_version": True, "violated_guard": True,
            "different_run_id": True,
        })

    def test_cartographer_replay_has_zero_delta(self):
        runtime = self.runtime()
        rom = bytearray(0x27EA + 2)
        rom[0x27E8:0x27EA] = b"\x48\x45"  # type: ignore[index]
        bundle, dependency = make_bundle(runtime, rom)
        self.assertEqual(dependency, "0x0027E8")
        with tempfile.TemporaryDirectory() as directory:
            graph = Cartographer(Path(directory) / "map.sqlite", "rom")
            first = graph.merge(bundle, "walker1-test", "contract").as_dict()
            replay = graph.merge(bundle, "walker1-test", "contract").as_dict()
            self.assertGreater(first["new_nodes"], 0)
            self.assertGreater(first["new_edges"], 0)
            self.assertEqual(replay["new_nodes"], 0)
            self.assertEqual(replay["new_edges"], 0)
            graph.close()

    def test_gap_rejects_runtime_gate(self):
        runtime = copy.deepcopy(self.runtime())
        runtime["off_pcs"] = ["0x0027D0"]
        runtime["metrics"]["callbacks_during_b_off"] = 1
        self.assertFalse(validate_runtime(runtime)["passed"])


if __name__ == "__main__":
    unittest.main()
