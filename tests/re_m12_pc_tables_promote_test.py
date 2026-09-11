import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_pc_tables_promote", ROOT / "src/tools/re_m12_pc_tables_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_ranges_are_even_and_non_overlapping():
    ranges = [(start, end) for start, end, _ in MODULE.RANGES]
    assert all(start < end and start % 2 == 0 and end % 2 == 0
               for start, end in ranges)
    assert all(left[1] <= right[0] for left, right in zip(ranges, ranges[1:]))


def test_contract_bytes_are_stable():
    assert sum(end - start for start, end, _ in MODULE.RANGES) == 352
