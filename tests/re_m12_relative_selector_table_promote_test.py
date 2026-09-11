"""Regression checks for the exact AUTO39 relative selector table."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_relative_selector_table", ROOT / "src/tools/re_m12_relative_selector_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.TABLE_END - MODULE.TABLE_START == 10
    assert MODULE.SELECTOR_OFFSETS == (0, 2, 4, 6, 8)
    assert MODULE.TABLE_VALUES == (10, 28, 52, 82, 118)
    assert MODULE.TARGETS == (0x15A9B0, 0x15A9C4, 0x15A9DE, 0x15A9FE, 0x15AA24)
    entries = [{"start": 0, "end": MODULE.TABLE_END + 1,
                "kind": "UNKNOWN", "size": MODULE.TABLE_END + 1}]
    promoted = MODULE.split_unknown(entries)
    assert promoted[1]["size"] == 10
    assert promoted[1]["classification"] == "RELATIVE_SELECTOR_OFFSET_TABLE_5X16"
    print("M12 relative selector table promotion helper tests passed")


if __name__ == "__main__":
    main()
