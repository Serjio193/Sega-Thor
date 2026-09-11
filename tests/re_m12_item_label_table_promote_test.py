import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_item_label_table", ROOT / "src/tools/re_m12_item_label_table_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 512
    assert MOD.RECORD_SIZE * MOD.RECORD_COUNT == 512
    assert MOD.CONSUMERS == (0x0041A6, 0x0041CA)
    assert len(MOD.TABLE_SHA256) == 64
    print("M12 item label table promotion helper tests passed")


if __name__ == "__main__":
    main()
