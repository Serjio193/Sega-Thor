import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_indirect_body_dispatch", ROOT / "src/tools/m12_indirect_body_dispatch.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_both_indirect_body_forms_and_finite_targets_are_proven():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    result = MODULE.parse_dispatch(rom)
    assert result["status"] == "TWO_INDIRECT_BODY_CALLS_FINITE_TARGETS_PROVEN"
    assert result["unique_target_count"] == 15
    assert result["routine_count"] == 14
    assert {item["indirect_call"] for item in result["sources"]} == {
        "0x03AA28", "0x03AAA8"
    }
    assert result["non_routine_targets"][0]["address"] == "0x03BA46"
    assert result["routines"][-1]["end_exclusive"] == "0x03B092"
    assert result["downstream_unresolved"][0]["pc"] == "0x0003B0A8"
    assert "0x0000B730 -> SAT" in result["join"]


if __name__ == "__main__":
    test_both_indirect_body_forms_and_finite_targets_are_proven()
    print("M12 indirect body dispatch tests passed")
