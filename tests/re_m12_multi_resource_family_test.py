import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_multi_resource_family_promote",
    ROOT / "src/tools/re_m12_multi_resource_family_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_resource_family_contracts():
    rom = (ROOT / "build/reference/Beyond Oasis (USA).bin").read_bytes()
    contract = MODULE.parse_contract(rom)
    assert contract["family_a"]["record_count"] == 10
    assert contract["family_b"]["record_count"] == 13
    assert contract["direct_stream_count"] == 3


def test_census_matches_direct_streams():
    census = ROOT / "build/m12-auto50-full-graphics-census.json"
    rows = MODULE.verify_census(census, MODULE.DIRECT_STREAMS)
    assert rows[0]["end"] == 0x2F7D35
    assert rows[-1]["end"] == 0x2FC681


if __name__ == "__main__":
    test_resource_family_contracts()
    test_census_matches_direct_streams()
    print("M12 multi-resource family helper tests passed")
