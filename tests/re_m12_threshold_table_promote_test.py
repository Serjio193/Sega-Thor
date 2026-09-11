import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_threshold_table", ROOT / "src/tools/re_m12_threshold_table_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 18
    assert MOD.VALUES[-1] == 0xFFFF
    assert len(MOD.VALUES) == 9
    assert MOD.VALUES[:-1] == tuple(sorted(MOD.VALUES[:-1]))
    print("M12 threshold table promotion helper tests passed")


if __name__ == "__main__":
    main()
