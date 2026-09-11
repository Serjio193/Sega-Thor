import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_callback_promote", ROOT / "src/tools/re_m12_callback_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_callbacks_are_non_overlapping_and_bounded():
    ranges = [(start, end) for start, end, _ in MODULE.CALLBACKS]
    assert ranges == [(0x10000, 0x10034), (0x30000, 0x30002)]
    assert ranges[0][1] <= ranges[1][0]


def test_generated_sources_have_exact_expected_sizes():
    assert len(MODULE.ASM[0x10000].encode()) > 0
    assert MODULE.ASM[0x30000].strip().endswith("rts")
