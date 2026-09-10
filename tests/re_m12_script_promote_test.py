"""Regression checks for the M12 indexed script-table contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_script_promote", ROOT / "src/tools/re_m12_script_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    table = 32
    rom = bytearray(96)
    offsets = (8, 10, 12)
    for index, offset in enumerate(offsets):
        entry = table + 2 * index
        rom[entry:entry + 2] = offset.to_bytes(2, "big", signed=True)
    rom[40:43] = b"A\x01B"
    rom[43] = 0
    rom[44:47] = b"C\x02D"
    rom[47] = 0
    rom[48:51] = b"E\x07F"
    rom[51] = 0
    old = (MODULE.TABLE_START, MODULE.TABLE_COUNT, MODULE.CALLER_START,
           MODULE.PARSER_START, MODULE.CALLER_BYTES, MODULE.PARSER_BYTES)
    MODULE.TABLE_START, MODULE.TABLE_COUNT = table, 3
    MODULE.CALLER_START, MODULE.PARSER_START = 0, 8
    MODULE.CALLER_BYTES = bytes(0)
    MODULE.PARSER_BYTES = bytes(0)
    try:
        result = MODULE.parse_table(rom)
    finally:
        (MODULE.TABLE_START, MODULE.TABLE_COUNT, MODULE.CALLER_START,
         MODULE.PARSER_START, MODULE.CALLER_BYTES, MODULE.PARSER_BYTES) = old
    assert [(x["start"], x["end"]) for x in result["records"]] == [
        (40, 44), (44, 48), (48, 52)]
    assert result["records"][0]["controls"] == [1]
    print("M12 indexed script promotion helper tests passed")


if __name__ == "__main__":
    main()
