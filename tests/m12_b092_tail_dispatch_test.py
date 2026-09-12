import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_b092_tail_dispatch", ROOT / "src/tools/m12_b092_tail_dispatch.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_b092_tail_table_and_all_targets_are_closed():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    result = MODULE.parse_tail(rom)
    assert result["status"] == "SECOND_LEVEL_TAIL_FINITE_DOMAIN_PROVEN"
    assert result["root"]["index_source"] == "0x00FFAFB0"
    assert result["root"]["jump_pc"] == "0x03B0A8"
    assert result["table"]["entry_count"] == 4
    assert [item["target"] for item in result["targets"]] == [
        "0x03B0BC", "0x03B0E8", "0x03B132", "0x03B0BA"
    ]
    assert len(result["routines"]) == 4
    assert all(not item["direct_calls"] for item in result["routines"])
    assert result["interpreter_test"]["conclusion"] == "NO_PROVEN_COMMAND_OR_FRAME_INTERPRETER"


if __name__ == "__main__":
    test_b092_tail_table_and_all_targets_are_closed()
    print("M12 B092 tail dispatch tests passed")
