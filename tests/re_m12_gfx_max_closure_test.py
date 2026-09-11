import importlib.util
import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_max_closure", ROOT / "src/tools/re_m12_gfx_max_closure.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    for index in range(MODULE.TABLE_COUNT):
        rom[MODULE.TABLE_START + index * 4:MODULE.TABLE_START + index * 4 + 4] = (
            (0x100 + index * 4).to_bytes(4, "big"))
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    table = MODULE.table_contract(bytes(rom))
    assert table["bytes"] == 84
    assert table["count"] == 21
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries, MODULE.TABLE_START, MODULE.TABLE_END)
    assert promoted[1]["classification"] == "SCREEN_GROUP_POINTER_TABLE"
    assert promoted[1]["size"] == 84
    print("M12 graphics-max closure helper tests passed")


if __name__ == "__main__":
    main()
