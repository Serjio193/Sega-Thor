import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_dispatch_pointer_table", ROOT / "src/tools/re_m12_dispatch_pointer_table_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert MOD.TABLE_END - MOD.TABLE_START == 356
    assert len(MOD.POINTERS) == 89
    assert MOD.POINTERS[0] == 0x0115C8
    assert MOD.POINTERS[-1] == 0x016CBE
    assert MOD.POINTERS.count(0x00E0B8) == 7
    assert MOD.CONSUMERS == (0x00FD70, 0x00FDF4, 0x00FF7E)
    print("M12 dispatch pointer table promotion helper tests passed")


if __name__ == "__main__":
    main()
