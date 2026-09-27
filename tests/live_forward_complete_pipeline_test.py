"""Safety tests for automatic post-run stages 5-9."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_complete_pipeline import run_remaining  # noqa: E402
from live_forward_progress import ProgressPublisher  # noqa: E402


class CompletePipelineTests(unittest.TestCase):
    def test_missing_ordered_flow_stops_map_and_retains_cleanup(self) -> None:
        class FakeProgress(ProgressPublisher):
            def __init__(self, path: Path) -> None:
                super().__init__(path)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            receipt = root / "receipt.json"
            receipt.write_text(json.dumps({"runtime": {"run_id": 9},
                "raw_segment_spool": {"raw_path": str(root / "missing.bin"),
                                      "index_path": str(root / "missing.jsonl")}}),
                encoding="utf-8")
            status = root / "status.json"
            progress = FakeProgress(status)
            result = run_remaining(receipt, {}, root / "generation",
                                   root / "analysis", root / "missing.rom", progress)
            progress.complete()
            self.assertEqual(result["pipeline_state"], "ANALYSIS STOPPED ✗")
            self.assertTrue(result["stop"].startswith("STOP_POSTRUN_MAP_FACT_MISSING"))
            self.assertEqual(result["cleanup"]["status"], "RAW_RETAINED")
            snapshot = json.loads(status.read_text(encoding="utf-8"))
            self.assertEqual(snapshot["stages"]["REFRESHING MAP"]["state"], "STOP")
            self.assertEqual(snapshot["overall_state"], "ANALYSIS STOPPED ✗")


if __name__ == "__main__":
    unittest.main(verbosity=2)
