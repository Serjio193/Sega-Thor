import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_multi_dispatch_targets_promote",
    ROOT / "src/tools/re_m12_multi_dispatch_targets_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_and_selector_layout():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert len(contract["table_values"]) == 15
    assert contract["selector_offsets"] == [0, 4, 8, 12, 16, 20, 24, 28]
    assert contract["table_values"][7] == 0x03AAEE
    assert contract["unbounded_target_left_unknown"] == "0x03BA46"


def test_split_preserves_unknown_neighbors():
    entries = [{"start": 0, "end": 0x100, "kind": "UNKNOWN", "size": 0x100}]
    result = MODULE.split_unknown(entries, 0x20, 0x30, "CODE_VERIFIED", "test", "bounded")
    assert [(item["start"], item["end"]) for item in result] == [
        (0, 0x20), (0x20, 0x30), (0x30, 0x100)]


if __name__ == "__main__":
    test_contract_and_selector_layout()
    test_split_preserves_unknown_neighbors()
    print("M12 multi-dispatch target helper tests passed")
