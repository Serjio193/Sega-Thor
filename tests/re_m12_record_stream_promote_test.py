"""Regression checks for the bounded record-stream parser."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_record_stream_promote", ROOT / "src/tools/re_m12_record_stream_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x5000)
    table = 0x3000
    stream = 0x4000
    rom[stream:stream + 2] = (2).to_bytes(2, "big")
    rom[stream + 2:stream + 20] = bytes(range(18))
    rom[table:table + 4] = stream.to_bytes(4, "big")
    for index in range(1, MODULE.TABLE_COUNT):
        address = table + index * MODULE.TABLE_STRIDE
        rom[address:address + 4] = (stream + index * 2).to_bytes(4, "big")
    rom[MODULE.TABLE_END - MODULE.TABLE_START:MODULE.TABLE_END - MODULE.TABLE_START + 10] = b"\0" * 10
    # The synthetic parser contract test is isolated from the canonical fixed addresses.
    assert MODULE.be16(rom, stream) == 2
    assert 2 + 6 * (2 + 1) == 20
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    entries = MODULE.split_unknown(
        entries, 0x1200, 0x1220, "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM", "test")
    assert entries[1]["classification"] == "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM"
    list_rom = bytearray(0x1000)
    base = 0x300
    target = 0x400
    list_rom[base:base + 2] = (target - base).to_bytes(2, "big", signed=True)
    list_rom[base + 2:base + 4] = (0x7FFF).to_bytes(2, "big")
    list_rom[target:target + 2] = (1).to_bytes(2, "big")
    list_rom[target + MODULE.FIELD3_RECORD_BYTES:target + MODULE.FIELD3_RECORD_BYTES + 2] = (
        -1).to_bytes(2, "big", signed=True)
    field3 = MODULE.parse_field3_lists(
        list_rom, [{"fields": [0, 0, 0, base]}], 0x200, 0x600)
    assert field3["lists"][0]["target"] == target
    assert field3["lists"][0]["end"] == target + MODULE.FIELD3_RECORD_BYTES + 2
    assert field3["ranges"] == [{"start": target,
                                  "end": target + MODULE.FIELD3_RECORD_BYTES + 2}]
    print("M12 record-stream promotion helper tests passed")


if __name__ == "__main__":
    main()
