"""Regression checks for exact-small-table contract parsing."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_exact_small_tables_promote",
    ROOT / "src/tools/re_m12_exact_small_tables_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x60000)
    for start, end, consumer, contract, _ in MODULE.TABLES:
        rom[consumer:consumer + len(bytes.fromhex(contract))] = bytes.fromhex(contract)
    report = MODULE.parse_tables(rom)
    assert [item["bytes"] for item in report] == [0x20, 8, 0x12]
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries, 0x3E0B8, 0x3E0D8)
    assert promoted[1]["classification"] == "EXACT_SMALL_LOOKUP_TABLE"
    assert promoted[1]["size"] == 0x20
    print("M12 exact-small-table promotion helper tests passed")


if __name__ == "__main__":
    main()
