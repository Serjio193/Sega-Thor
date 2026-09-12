import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_d406_residual_census",
    ROOT / "src/tools/re_m12_gfx_d406_residual_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    target = b"\x4E\xB9\x00\x00\xD4\x06"
    for site in (0x120, 0x240, 0x360):
        rom[site:site + len(target)] = target
    assert MODULE.direct_call_sites(bytes(rom)) == [0x120, 0x240, 0x360]
    assert MODULE.RESIDUAL[0x02DB40]["source_blocker"] == (
        "D406_POST_SOURCE_NOT_ROM_PROVEN")
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    try:
        MODULE.build_report(bytes(rom))
    except ValueError as error:
        assert "D406 call count mismatch" in str(error)
    else:
        raise AssertionError("synthetic call census must fail closed")
    print("re_m12_gfx_d406_residual_census_test: pass")


if __name__ == "__main__":
    main()
