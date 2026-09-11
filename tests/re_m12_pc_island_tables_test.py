import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_pc_island_tables_promote",
    ROOT / "src/tools/re_m12_pc_island_tables_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_closes_all_ranges():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert len(contract["ranges"]) == 15
    assert sum(item["bytes"] for item in contract["ranges"]) == 0x178


def test_split_preserves_neighbors():
    entries = [{"start": 0, "end": 0x200, "kind": "UNKNOWN", "size": 0x200}]
    result = MODULE.split_unknown(entries, 0x40, 0x60, "TABLE", "bounded")
    assert [(item["start"], item["end"]) for item in result] == [
        (0, 0x40), (0x40, 0x60), (0x60, 0x200)]


if __name__ == "__main__":
    test_contract_closes_all_ranges()
    test_split_preserves_neighbors()
    print("M12 PC island table helper tests passed")
