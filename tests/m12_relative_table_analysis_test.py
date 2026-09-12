import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_relative_table_analysis", ROOT / "src/tools/m12_relative_table_analysis.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_relative_pair_grammar_and_blocker_are_explicit():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    result = MODULE.parse_relative_table(rom)
    assert result["status"] == "GRAMMAR_PROVEN_EXTENT_BLOCKED_BY_ONE_UNTERMINATED_STREAM"
    assert result["root"]["pointer_storage_extent"] == ["0x03BDA6", "0x03BDCA"]
    assert result["consumer_grammar"]["element_width_bytes"] == 4
    assert "ADDA.W" in result["consumer_grammar"]["relative_entry"]
    assert len(result["consumed_pointer_entries"]) == 11
    assert result["extent"]["blocker"].startswith("stream target 0x03BDD8")
    assert result["join"].endswith("0x0000B730 -> SAT")


if __name__ == "__main__":
    test_relative_pair_grammar_and_blocker_are_explicit()
    print("M12 relative table analysis tests passed")
