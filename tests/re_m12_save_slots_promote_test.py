"""Regression checks for exact save-slot contract parsing."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_save_slots_promote", ROOT / "src/tools/re_m12_save_slots_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    for address, contract in MODULE.CONTRACTS.items():
        rom[address:address + len(contract)] = contract
    report = MODULE.parse_contracts(rom)
    assert [item["bytes"] for item in report] == [0x114, 0x14]
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries, 0x2025CD, 0x2026E1)
    assert promoted[1]["classification"] == "SAVE_SLOT_SERIALIZATION_RANGE"
    assert promoted[1]["size"] == 0x114
    print("M12 save-slot promotion helper tests passed")


if __name__ == "__main__":
    main()
