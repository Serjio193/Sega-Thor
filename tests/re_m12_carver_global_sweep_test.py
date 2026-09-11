"""Synthetic contracts for global runtime-span normalization and blockers."""

import importlib.util
from pathlib import Path
import sys


MODULE_PATH = Path(__file__).parents[1] / "src/tools/m12_carver_global_sweep.py"
sys.path.insert(0, str(MODULE_PATH.parent))
SPEC = importlib.util.spec_from_file_location("m12_carver_global_sweep", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_adjacent_reads_merge_and_count():
    records = [
        {"id": "a", "start": 0x100, "end": 0x102, "scenario_id": "s",
         "reader_pc": 0x200, "access_type": "read", "width_bytes": 2,
         "parser": None, "consumer": None, "destination_domain": None,
         "order": 1, "exact_boundary": False, "src_after": 0x102,
         "consumed_bytes": 2},
        {"id": "b", "start": 0x102, "end": 0x104, "scenario_id": "s",
         "reader_pc": 0x200, "access_type": "read", "width_bytes": 2,
         "parser": None, "consumer": None, "destination_domain": None,
         "order": 2, "exact_boundary": False, "src_after": 0x104,
         "consumed_bytes": 2},
    ]
    result = MODULE._merge_runtime(records)
    assert len(result) == 1
    assert (result[0]["start"], result[0]["end"]) == (0x100, 0x104)
    assert result[0]["repetition_count"] == 2
    assert result[0]["consumed_bytes"] == 4


def test_unobserved_gap_is_classified_globally():
    class FakeDB:
        conflicts = {}

        def _related_evidence(self, gap):
            return []

    gap = {"start": 0x1000, "end": 0x1100, "conflicts": []}
    assert MODULE._blocker(gap, FakeDB(), [], []) == ("A", "never observed at runtime by the available scenarios")


if __name__ == "__main__":
    test_adjacent_reads_merge_and_count()
    test_unobserved_gap_is_classified_globally()
    print("M12 Carver global sweep tests passed")
