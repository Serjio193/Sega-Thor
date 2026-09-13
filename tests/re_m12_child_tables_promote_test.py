import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_child_tables_promote",
    ROOT / "src/tools/re_m12_child_tables_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_closes_all_child_streams():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["table"]["bytes"] == 0xEA
    assert contract["table"]["promoted_bytes"] == 0xE8
    assert [item["selector"] for item in contract["children"]] == list(range(7))
    assert contract["children"][-1]["end"] == "0x03BA46"
    assert all(item["sentinel"]["word"] == "0xFFFF" for item in contract["children"])


def test_split_preserves_owned_alias_prefix_and_unknown_suffix():
    entries = [
        {"start": 0x03B8DE, "end": 0x03B95E,
         "kind": "STRUCTURED_DATA_CONFIRMED", "size": 0},
        {"start": 0x03B95E, "end": 0x03BD86,
         "kind": "UNKNOWN", "size": 0},
    ]
    result = MODULE.split_unknown(entries)
    assert [(item["start"], item["end"], item["kind"]) for item in result] == [
        (0x03B8DE, 0x03B95E, "STRUCTURED_DATA_CONFIRMED"),
        (0x03B95E, 0x03BA46, "STRUCTURED_DATA_CONFIRMED"),
        (0x03BA46, 0x03BD86, "UNKNOWN"),
    ]


if __name__ == "__main__":
    test_contract_closes_all_child_streams()
    test_split_preserves_owned_alias_prefix_and_unknown_suffix()
    print("M12 selector child-table promotion helpers passed")
