import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_d3b2_census", ROOT / "src/tools/re_m12_gfx_d3b2_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_exact_call_and_immediate_setup():
    rom = bytearray(b"\0" * 0x40)
    site = 0x20
    rom[site - 8:site - 4] = bytes.fromhex("303c0023")
    rom[site - 4:site] = bytes.fromhex("323c4000")
    rom[site:site + 6] = MODULE.CALL_BYTES
    assert MODULE.direct_call_sites(bytes(rom)) == [site]
    setup = MODULE.parse_setup(bytes(rom), site)
    assert setup["selector_d0"] == 0x23
    assert setup["vram_destination_d1"] == 0x4000


def test_table_and_manifest_child_contract():
    rom = bytearray(b"\0" * 0x100)
    MODULE.TABLE_START = 0x20
    MODULE.TABLE_ENTRIES = 3
    MODULE.TABLE_END = MODULE.TABLE_START + MODULE.TABLE_ENTRIES * 4
    rom[0x24:0x28] = (0x40).to_bytes(4, "big")
    rom[0x28:0x2C] = (0x50).to_bytes(4, "big")
    records = MODULE.table_records(bytes(rom))
    assert records[0]["pointer"] == "0x000000"
    assert records[1]["pointer"] == "0x000040"
    manifest = {"entries": [
        {"kind": "LOCAL_ROM_DERIVED_ASSET", "start": 0x40, "end": 0x50,
         "source": "test", "ownership_reason": "resource table index 1"},
        {"kind": "LOCAL_ROM_DERIVED_ASSET", "start": 0x50, "end": 0x60,
         "source": "test", "ownership_reason": "resource table index 2"},
    ]}
    owned = MODULE.owned_resource_spans(manifest)
    assert sorted(owned) == [1, 2]


def test_canonical_guard_is_independent_of_manifest_ownership():
    rom = bytes(0x100)
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    MODULE.TABLE_START = 0x20
    MODULE.TABLE_ENTRIES = 1
    MODULE.TABLE_END = MODULE.TABLE_START + 4
    try:
        MODULE.build_report(rom, {"entries": []})
    except ValueError as error:
        assert "D3B2 direct-call set mismatch" in str(error)
    else:
        raise AssertionError("missing exact direct-call guard")


if __name__ == "__main__":
    test_exact_call_and_immediate_setup()
    test_table_and_manifest_child_contract()
    test_canonical_guard_is_independent_of_manifest_ownership()
