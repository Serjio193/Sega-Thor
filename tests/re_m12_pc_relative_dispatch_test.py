import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_pc_relative_dispatch_promote",
    ROOT / "src/tools/re_m12_pc_relative_dispatch_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_closes_table_targets():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["table"] == (0x00E2F2, 0x00E302)
    assert contract["table_targets"] == (
        0x00E302, 0x00E306, 0x00E308, 0x00E30A,
        0x00E30E, 0x00E318, 0x00E30E, 0x00E310)


def test_split_preserves_dispatch_gap():
    entries = [{"start": 0, "end": 0x100, "kind": "UNKNOWN", "size": 0x100}]
    result = MODULE.split_unknown(
        entries, 0x20, 0x30, "STRUCTURED_DATA_CONFIRMED", "test",
        "TABLE", "bounded")
    assert [(item["start"], item["end"]) for item in result] == [
        (0, 0x20), (0x20, 0x30), (0x30, 0x100)]


if __name__ == "__main__":
    test_contract_closes_table_targets()
    test_split_preserves_dispatch_gap()
    print("M12 PC-relative dispatch helper tests passed")
