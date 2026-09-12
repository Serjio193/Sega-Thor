import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_a372_caller_join", ROOT / "src/tools/m12_a372_caller_join.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_static_scheduler_join():
    rom_path = ROOT / "build/reference/Beyond Oasis (USA).bin"
    if not rom_path.exists():
        return
    result = MODULE.parse_join(rom_path.read_bytes())
    assert result["status"] == "STATIC_SCHEDULER_TO_SHADOW_SAT_JOIN_PROVEN"
    assert result["join"].startswith("0x008B22 -> 0x008E90 -> 0x00A196")
    assert result["a196_to_a372"]["call"]["target"] == "0x00A342"
    assert result["ff1858_selector_join"]["following_calls"] == ["0x008E90", "0x00A196"]
    assert result["a6a0_family"]["callers"] == ["0x00428A", "0x004A5A"]
    assert result["negative_evidence"]["direct_calls_to_a196_in_bounded_range"] == []
    assert result["semantic_result"]["frame"] == "NOT_PROVEN"
    assert result["ownership"]["rom_bytes_added"] == 0


if __name__ == "__main__":
    test_static_scheduler_join()
