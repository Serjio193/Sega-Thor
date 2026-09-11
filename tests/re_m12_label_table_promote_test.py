import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_label_table", ROOT / "src/tools/re_m12_label_table_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 48
    assert len(MOD.TABLE_BYTES) == 48
    assert MOD.RECORD_SIZE * MOD.RECORD_COUNT == 48
    assert MOD.TABLE_BYTES[:8] == bytes.fromhex("06574541504F4E20")
    assert MOD.TABLE_BYTES[-8:] == bytes.fromhex("034F4B3F20202020")
    print("M12 label table promotion helper tests passed")


if __name__ == "__main__":
    main()
