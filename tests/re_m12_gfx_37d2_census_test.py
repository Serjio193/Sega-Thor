import importlib.util
from pathlib import Path


ROOT = Path(__file__).parents[1]
MODULE_PATH = ROOT / "src" / "tools" / "re_m12_gfx_37d2_census.py"
SPEC = importlib.util.spec_from_file_location("re_m12_gfx_37d2_census", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_exact_direct_call_scan_and_setup():
    rom = bytearray(0x100)
    rom[0x20:0x26] = MODULE.CALL_BYTES
    rom[0x40:0x56] = bytes.fromhex(
        "41F90000006043F900FF2FA8303C12344EB9000037D2")
    assert MODULE.direct_call_sites(rom) == [0x20, 0x50]
    parsed = MODULE.parse_direct_setup(bytes(rom), 0x50, 0x40)
    assert parsed["source"] == 0x60
    assert parsed["destination"] == "0x00FF2FA8"
    assert parsed["d0"] == 0x1234


def test_owner_span_requires_one_source_owned_entry():
    manifest = {"entries": [
        {"start": 0, "end": 16, "kind": "UNKNOWN"},
        {"start": 16, "end": 32, "kind": "LOCAL_ROM_DERIVED_ASSET"},
    ]}
    assert MODULE.owner_span(manifest, 16, 32)["source_owned"]
    assert not MODULE.owner_span(manifest, 8, 24)["source_owned"]


if __name__ == "__main__":
    test_exact_direct_call_scan_and_setup()
    test_owner_span_requires_one_source_owned_entry()
    print("re_m12_gfx_37d2_census_test: pass")
