"""Regression checks for the M12-AUTO candidate selector."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_auto_promote", ROOT / "src/tools/re_m12_auto_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    code = [
        {"start": 0x100, "end": 0x180, "size": 0x80, "exact": True, "called_by": 2},
        {"start": 0x140, "end": 0x150, "size": 0x10, "exact": True, "called_by": 1},
        {"start": 0x200, "end": 0x210, "size": 0x10, "exact": True, "called_by": 0},
    ]
    selected, assets, ranges = MODULE.selected(code, {"records": []})
    assert [(x["start"], x["end"]) for x in selected] == [(0x100, 0x180)]
    assert not assets
    assert ranges == [(0x100, 0x180)]
    print("M12-AUTO helper tests passed")


if __name__ == "__main__":
    main()
