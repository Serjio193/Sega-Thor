"""Regression checks for the exact indexed offset-table contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_indexed_offset_table_promote",
    ROOT / "src/tools/re_m12_indexed_offset_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.selector_offsets() == tuple(range(0, 0x20, 2))
    entries = [{"start": 0, "end": 0x20000, "kind": "UNKNOWN", "size": 0x20000}]
    promoted = MODULE.split_unknown(entries)
    assert promoted[1]["start"] == 0xAD56
    assert promoted[1]["end"] == 0xAD76
    assert promoted[1]["size"] == 0x20
    assert promoted[1]["classification"] == "INDEXED_WORD_OFFSET_TABLE"
    print("M12 indexed offset-table promotion helper tests passed")


if __name__ == "__main__":
    main()
