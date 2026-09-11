import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_resource_pointer_table", ROOT / "src/tools/re_m12_resource_pointer_table_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 432
    assert MOD.ENTRY_COUNT == 108
    assert MOD.STREAM_FIRST == 0x1AD000
    assert MOD.STREAM_LAST == 0x1E6EDA
    assert MOD.CONSUMER == 0x00D3B2
    print("M12 resource pointer table promotion helper tests passed")


if __name__ == "__main__":
    main()
