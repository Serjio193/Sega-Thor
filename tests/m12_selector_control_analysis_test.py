import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_selector_control_analysis", ROOT / "src/tools/m12_selector_control_analysis.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_dispatcher_proves_selector_handler_entry():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    result = MODULE.parse_control(rom)
    assert result["status"] == "UPSTREAM_CONTROL_PROVEN_WITH_EXACT_INDIRECT_ENTRY"
    assert result["dispatcher"]["target_entry"] == {"selector_value": "0x10", "target": "0x03A748"}
    assert result["dispatcher"]["direct_incoming_to_handler"] == []
    assert {item["value"] for item in result["higher_level_state"]["writes"]} >= {"0x0010", "0x0004", "0x0008", "0x000C"}
    assert result["selector_handler"]["semantic_classification"] == "NEUTRAL_FINITE_SELECTOR_LOOP"


if __name__ == "__main__":
    test_dispatcher_proves_selector_handler_entry()
    print("M12 selector control analysis tests passed")
