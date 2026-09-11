import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_table_03b8de_promote",
    ROOT / "src/tools/re_m12_table_03b8de_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_closes_eight_records():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["table"]["bytes"] == 0x80
    assert len(contract["records"]) == 8
    assert contract["records"][0]["fields"] == [0x03BA46, 0x17093C, 0x1713F6, 0x03B95C]
    assert contract["records"][7]["fields"] == [0x171832, 0x171A62, 0x03BDA6, 0xFFFF0017]


def test_split_only_promotes_unknown_suffix():
    entries = [
        {"start": 0x03B000, "end": 0x03B8E2, "kind": "STRUCTURED_DATA_CONFIRMED", "size": 0},
        {"start": 0x03B8E2, "end": 0x03BF86, "kind": "UNKNOWN", "size": 0},
    ]
    result = MODULE.split_unknown(entries, 0x03B8E2, 0x03B95E)
    assert [(item["start"], item["end"]) for item in result] == [
        (0x03B000, 0x03B8E2), (0x03B8E2, 0x03B95E), (0x03B95E, 0x03BF86)
    ]


if __name__ == "__main__":
    test_contract_closes_eight_records()
    test_split_only_promotes_unknown_suffix()
    print("M12 0x03B8DE table helper tests passed")
