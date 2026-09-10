"""Regression checks for the M12 fixed-stride table contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_fixed_stride_table_promote",
    ROOT / "src/tools/re_m12_fixed_stride_table_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x900)
    old = (MODULE.TABLE_START, MODULE.TABLE_END, MODULE.NEXT_TABLE_BYTES,
           MODULE.CONSUMERS)
    MODULE.TABLE_START = 0x100
    MODULE.TABLE_END = MODULE.TABLE_START + MODULE.TABLE_RECORD_SIZE * MODULE.TABLE_COUNT
    MODULE.NEXT_TABLE_BYTES = b"\x0E\xEE" * 0x40
    MODULE.CONSUMERS = ((0x20, b"consumer"),)
    rom[0x20:0x28] = b"consumer"
    rom[MODULE.TABLE_END:MODULE.TABLE_END + 0x80] = MODULE.NEXT_TABLE_BYTES
    try:
        result = MODULE.parse_table(rom)
        assert result["record_size"] == 0x20
        assert result["records"] == 0x32
        rom[0x20] ^= 1
        try:
            MODULE.parse_table(rom)
        except ValueError as error:
            assert "consumer contract" in str(error)
        else:
            raise AssertionError("changed consumer contract was accepted")
    finally:
        (MODULE.TABLE_START, MODULE.TABLE_END, MODULE.NEXT_TABLE_BYTES,
         MODULE.CONSUMERS) = old
    print("M12 fixed-stride table promotion helper tests passed")


if __name__ == "__main__":
    main()
