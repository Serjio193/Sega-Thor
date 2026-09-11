"""Regression checks for constant-D0 CC-B0 slot selection."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_ccb0_selected_slots_promote",
    ROOT / "src/tools/re_m12_ccb0_selected_slots_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x50000)
    rom[MODULE.GROUP.CONSUMER:MODULE.GROUP.CONSUMER + len(MODULE.GROUP.CONSUMER_CONTRACT)] = (
        MODULE.GROUP.CONSUMER_CONTRACT)
    for index in range(MODULE.GROUP.TABLE_COUNT):
        rom[MODULE.GROUP.TABLE_START + index * 4:MODULE.GROUP.TABLE_START + index * 4 + 4] = (
            (0x1000 + index * 0x100).to_bytes(4, "big"))
    for _, d0_address, d0 in MODULE.CALLERS:
        rom[d0_address:d0_address + 4] = bytes((0x30, 0x3C, d0 >> 8, d0 & 0xFF))
    for caller, _, _ in MODULE.CALLERS:
        rom[caller:caller + len(MODULE.JMP)] = MODULE.JMP
    table, slots = MODULE.parse_slots(rom)
    assert table["count"] == 0x20
    assert len(slots) == len(set(item["slot"] for item in slots))
    assert all(item["slot"] + 2 <= len(rom) for item in slots)
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    for item in slots:
        entries = MODULE.split_slot(entries, item["slot"])
    assert sum(e["size"] for e in entries if e["kind"] == "STRUCTURED_DATA_CONFIRMED") == 2 * len(slots)
    print("M12 CC-B0 selected-slot promotion helper tests passed")


if __name__ == "__main__":
    main()
