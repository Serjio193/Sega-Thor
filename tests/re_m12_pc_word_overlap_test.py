import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_pc_word_overlap_promote",
    ROOT / "src/tools/re_m12_pc_word_overlap_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_and_overlap():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["table_start"] == 0x062DA8
    assert contract["table_end"] == 0x062DC8
    assert contract["unknown_prefix"] == [0x062DA8, 0x062DC0]
    assert contract["overlap"] == [0x062DC0, 0x062DC8]
    assert len(contract["words"]) == 16


def test_split_preserves_neighbors():
    entries = [{"start": 0, "end": 0x200, "kind": "UNKNOWN", "size": 0x200}]
    result = MODULE.split_unknown(entries, 0x40, 0x58)
    assert [(item["start"], item["end"]) for item in result] == [
        (0, 0x40), (0x40, 0x58), (0x58, 0x200)]


if __name__ == "__main__":
    test_contract_and_overlap()
    test_split_preserves_neighbors()
    print("M12 PC-relative overlapping word-table helper tests passed")
