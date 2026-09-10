"""Regression checks for exact erased-ROM padding promotion."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_padding_promote", ROOT / "src/tools/re_m12_padding_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x5000)
    rom[0x120:0x1200] = bytes([0x11]) * (0x1200 - 0x120)
    rom[0x1200:0x2000] = bytes([MODULE.FILL_BYTE]) * 0xE00
    rom[0x3200:0x4200] = bytes([MODULE.FILL_BYTE]) * 0x1000
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    runs = MODULE.discover(rom, entries)
    assert [(item["start"], item["end"]) for item in runs] == [(0x1200, 0x2000)]
    entries = MODULE.split_unknown(entries, 0x1200, 0x2000)
    owned = [item for item in entries if item["kind"] != "UNKNOWN"]
    assert owned[0]["kind"] == "PADDING_ALIGNMENT_CONFIRMED"
    assert owned[0]["classification"] == "ERASED_ROM_ALIGNMENT_PADDING"
    assert owned[0]["size"] == 0xE00
    print("M12 erased alignment padding promotion helper tests passed")


if __name__ == "__main__":
    main()
