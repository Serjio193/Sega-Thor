import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_bit7_lookup", ROOT / "src/tools/re_m12_bit7_lookup_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 64
    assert MOD.INDEX_MASK == 0x3F
    assert MOD.CONSUMER == 0x00F61C
    assert len(MOD.TABLE_SHA256) == 64
    print("M12 bit-7 lookup promotion helper tests passed")


if __name__ == "__main__":
    main()
