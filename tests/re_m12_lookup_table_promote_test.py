"""Regression checks for the M12 exact lookup-table contracts."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_lookup_table_promote", ROOT / "src/tools/re_m12_lookup_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x600)
    word_start = 0x100
    byte_start = 0x180
    old = (MODULE.WORD_TABLE_START, MODULE.WORD_TABLE_END,
           MODULE.BYTE_TABLE_START, MODULE.BYTE_TABLE_END)
    MODULE.WORD_TABLE_START, MODULE.WORD_TABLE_END = word_start, word_start + 0x80
    MODULE.BYTE_TABLE_START, MODULE.BYTE_TABLE_END = byte_start, byte_start + 0x200
    rom[word_start:word_start + 0x80] = b"\x0E\xEE" * 0x40
    rom[byte_start:byte_start + 0x200] = bytes(range(256)) * 2
    try:
        result = MODULE.parse_tables(rom, verify_code=False)
    finally:
        (MODULE.WORD_TABLE_START, MODULE.WORD_TABLE_END,
         MODULE.BYTE_TABLE_START, MODULE.BYTE_TABLE_END) = old
    assert result["word_table"]["entries"] == 0x40
    assert result["byte_table"]["bytes"] == 0x200
    print("M12 exact lookup-table promotion helper tests passed")


if __name__ == "__main__":
    main()
