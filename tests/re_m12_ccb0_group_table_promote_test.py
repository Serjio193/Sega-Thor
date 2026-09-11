"""Regression checks for the bounded CC-B0 group pointer table."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_ccb0_group_table_promote",
    ROOT / "src/tools/re_m12_ccb0_group_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x50000)
    table = MODULE.TABLE_START
    rom[MODULE.CONSUMER:MODULE.CONSUMER + len(MODULE.CONSUMER_CONTRACT)] = (
        MODULE.CONSUMER_CONTRACT)
    for index in range(MODULE.TABLE_COUNT):
        rom[table + index * MODULE.TABLE_STRIDE:table + index * MODULE.TABLE_STRIDE + 4] = (
            (0x200 + index * 2).to_bytes(4, "big"))
    report = MODULE.parse_table(rom)
    assert report["start"] == table
    assert report["end"] == MODULE.TABLE_END
    assert report["count"] == 0x20
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries, table, MODULE.TABLE_END)
    assert promoted[1]["classification"] == "GROUP_POINTER_TABLE_32X32"
    assert promoted[1]["size"] == 0x80
    print("M12 CC-B0 group-table promotion helper tests passed")


if __name__ == "__main__":
    main()
