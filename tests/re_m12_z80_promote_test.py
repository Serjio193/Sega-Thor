import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_z80_promote", ROOT / "src/tools/re_m12_z80_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.UPLOAD_END - MODULE.UPLOAD_START == 0x2000
    assert MODULE.UPLOAD_DESTINATION == 0xA00000
    entries = [{"start": 0, "end": 0x70000, "kind": "UNKNOWN"}]
    promoted = MODULE.split_unknown(entries)
    owned = next(item for item in promoted if item["start"] == MODULE.UPLOAD_START)
    assert owned["end"] == MODULE.UPLOAD_END
    assert owned["classification"] == "Z80_ASM_SOURCE_OWNED"
    print("M12 Z80 promotion helper tests passed")


if __name__ == "__main__":
    main()
