import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_enum_lookup", ROOT / "src/tools/re_m12_enum_lookup_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 64
    assert MOD.SELECTOR_MAX == 0x3F
    assert MOD.VALUE_MAX == 4
    assert MOD.CONSUMER == 0x007A6C
    print("M12 enum lookup promotion helper tests passed")


if __name__ == "__main__":
    main()
