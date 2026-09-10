"""Regression checks for the M12 nested pointer-record table contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_level_table_promote", ROOT / "src/tools/re_m12_level_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    old = (MODULE.TABLE_START, MODULE.TABLE_COUNT, MODULE.TABLE_END,
           MODULE.RECORD_AREA_START, MODULE.RECORD_AREA_END)
    MODULE.TABLE_START, MODULE.TABLE_COUNT = 32, 2
    MODULE.TABLE_END = 36
    MODULE.RECORD_AREA_START, MODULE.RECORD_AREA_END = 46, 72
    rom = bytearray(96)
    rom[32:34] = (4).to_bytes(2, "big", signed=True)
    rom[34:36] = (6).to_bytes(2, "big", signed=True)
    rom[36:38] = (0).to_bytes(2, "big")
    rom[38:40] = (26).to_bytes(2, "big", signed=True)
    rom[40:42] = (1).to_bytes(2, "big")
    rom[42:44] = (22).to_bytes(2, "big", signed=True)
    rom[44:46] = (24).to_bytes(2, "big", signed=True)
    rom[64:68] = b"\x10\x10A\0"
    rom[68:72] = b"\x10\x10B\0"
    try:
        result = MODULE.parse_level_table(rom, verify_code=False)
    finally:
        (MODULE.TABLE_START, MODULE.TABLE_COUNT, MODULE.TABLE_END,
         MODULE.RECORD_AREA_START, MODULE.RECORD_AREA_END) = old
    assert len(result["groups"]) == 2
    assert [(item["start"], item["end"]) for item in result["records"]] == [
        (64, 68), (68, 72)]
    print("M12 nested level-table promotion helper tests passed")


if __name__ == "__main__":
    main()
