import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes, optional_path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_static_stream_pointer_promote",
    ROOT / "src/tools/re_m12_static_stream_pointer_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_pointer_contract_and_stream_count():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["stream_count"] == 9
    assert contract["streams"][0]["pointer_addresses"] == (0x03B99C,)
    assert contract["streams"][5]["pointer_addresses"] == (0x005248, 0x007A5E)


def test_census_boundaries():
    census = optional_path(
        ROOT / "build/m12-auto50-full-graphics-census.json",
        "AUTO50 graphics census")
    if census is None:
        return
    rows = MODULE.verify_census(census)
    assert rows[0]["end"] == 0x1768C5
    assert rows[-1]["end"] == 0x165834


if __name__ == "__main__":
    test_pointer_contract_and_stream_count()
    test_census_boundaries()
    print("M12 static stream pointer helper tests passed")
