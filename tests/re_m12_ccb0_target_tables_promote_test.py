"""Regression checks for CC-B0 selector and interval helpers."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_ccb0_target_tables_promote",
    ROOT / "src/tools/re_m12_ccb0_target_tables_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x50000)
    rom[MODULE.CONSUMER:MODULE.CONSUMER + len(MODULE.SELECTOR_CONTRACT)] = (
        MODULE.SELECTOR_CONTRACT)
    table = 0x4371E
    for index in range(0x20):
        rom[table + index * 4:table + index * 4 + 4] = (0x200 + index * 2).to_bytes(4, "big")
    report = MODULE.parse_targets(rom)
    assert report["slot_count"] == 0x100
    assert report["slot_stride"] == 2
    assert report["targets"][0] == {"start": 0x200, "end": 0x400}
    assert MODULE.merge_intervals([(0x100, 0x200), (0x1F0, 0x300)]) == [(0x100, 0x300)]
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_interval(entries, 0x200, 0x400)
    assert promoted[1]["classification"] == "CCB0_RELATIVE_TARGET_TABLE_256X16"
    assert promoted[1]["size"] == 0x200
    print("M12 CC-B0 target-table promotion helper tests passed")


if __name__ == "__main__":
    main()
