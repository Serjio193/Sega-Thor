"""Checks fail-closed handling and stable path aggregation for island telemetry."""
import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "src/tools/hybrid/build_island_telemetry_v2.py"
SPEC = importlib.util.spec_from_file_location("island_telemetry_v2", SCRIPT)
TOOL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TOOL)


class TelemetryAggregationTests(unittest.TestCase):
    def test_missing_path_metrics_stay_uninstrumented(self):
        result = TOOL.timing_summary({}, 0x9D70)
        self.assertEqual(result["TIMING_VECTOR_COUNT"], 0)
        self.assertEqual(result["MIN_CYCLES"], "UNINSTRUMENTED")
        self.assertEqual(result["MAX_CYCLES"], "UNINSTRUMENTED")
        self.assertEqual(result["REFRESH_CROSSING_CALLS"], "UNINSTRUMENTED")

    def test_path_counts_refreshes_and_cycle_vectors(self):
        key = (0x9D70, "PCSEQ_TEST")
        sample = {
            "cycle_sequence_to_rts": [4, 10],
            "total_cycles": 126,
            "refresh_before_rts": False,
        }
        paths = {key: [sample, dict(sample), {
            "cycle_sequence_to_rts": [4, 24],
            "total_cycles": 140,
            "refresh_before_rts": True,
        }]}
        result = TOOL.timing_summary(paths, 0x9D70)
        self.assertEqual(result["TIMING_VECTOR_COUNT"], 3)
        self.assertEqual(result["UNIQUE_TIMING_VECTORS"], 2)
        self.assertEqual((result["MIN_CYCLES"], result["MAX_CYCLES"]), (126, 140))
        path = result["PATHS"][0]
        self.assertEqual(path["CALL_COUNT"], 3)
        self.assertEqual(path["REFRESH_CROSSING_CALLS"], 1)
        self.assertEqual(path["SCHEDULER_YIELDS"], "UNINSTRUMENTED")


if __name__ == "__main__":
    unittest.main()
