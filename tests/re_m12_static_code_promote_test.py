"""Regression checks for the AUTO24 exact static-island contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_static_code_promote", ROOT / "src/tools/re_m12_static_code_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    assert MODULE.END - MODULE.START == 0x9C
    assert MODULE.CALLERS == (0x1672E, 0x16742)
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.promote(entries)
    island = next(item for item in promoted if item["start"] == MODULE.START)
    assert island["kind"] == "CODE_VERIFIED"
    assert island["size"] == 0x9C
    asm = ROOT / "build" / "m12-auto24-exg-normalization.asm"
    asm.write_text("    exg.w D4,A6\n")
    MODULE.normalize_asm(asm)
    assert asm.read_text() == "    exg A4,A6\n"
    asm.unlink()
    print("M12 exact static-code promotion helper tests passed")


if __name__ == "__main__":
    main()
