import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_sibling_census", ROOT / "src/tools/re_m12_gfx_sibling_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    target = 0xD950
    sites = (0x120, 0x240)
    encoding = b"\x4E\xB9" + target.to_bytes(4, "big")
    for site in sites:
        rom[site:site + len(encoding)] = encoding
    assert MODULE.direct_call_sites(bytes(rom), target) == list(sites)
    assert MODULE.TARGETS[0x2CBC]["source_blocker"] == (
        "NO_ROM_SOURCE_OPERAND_IN_BOUNDED_BODY")
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    try:
        MODULE.build_report(bytes(rom))
    except ValueError as error:
        assert "unexpected body bytes" in str(error)
    else:
        raise AssertionError("synthetic body must fail closed")
    print("re_m12_gfx_sibling_census_test: pass")


if __name__ == "__main__":
    main()
