"""Regression checks for the bounded word-transform contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_bounded_word_transform",
    ROOT / "src/tools/re_m12_bounded_word_transform_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    rom[MODULE.ROUTINE:MODULE.ROUTINE + len(MODULE.ROUTINE_BYTES)] = MODULE.ROUTINE_BYTES
    for address, expected in MODULE.CALLERS + (MODULE.POINTER_INITIALIZER,):
        rom[address:address + len(expected)] = expected
    contract = MODULE.parse_contract(bytes(rom), verify_identity=False)
    assert contract["table_end"] - contract["table_start"] == 0x80
    assert contract["word_count"] == 0x40
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries)
    owned = next(item for item in promoted if item["kind"] == "STRUCTURED_DATA_CONFIRMED")
    assert owned["start"] == MODULE.TABLE_START
    assert owned["end"] == MODULE.TABLE_END
    assert owned["classification"] == "BOUNDED_WORD_TRANSFORM_TABLE"
    print("M12 bounded word-transform helper tests passed")


if __name__ == "__main__":
    main()
