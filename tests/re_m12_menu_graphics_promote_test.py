import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_menu_graphics", ROOT / "src/tools/re_m12_menu_graphics_promote.py")
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def main():
    assert len(MOD.STREAMS) == 3
    assert sum(item["source_consumed"] for item in MOD.STREAMS) == 5085
    assert MOD.STREAMS[-1]["start"] + MOD.STREAMS[-1]["source_consumed"] == 0x15CEA0
    assert sorted(MOD.CONSUMERS) == [0x004966, 0x004974, 0x004982]
    print("M12 menu graphics promotion helper tests passed")


if __name__ == "__main__":
    main()
