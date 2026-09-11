import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes, optional_path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_direct_loader_stream_promote",
    ROOT / "src/tools/re_m12_direct_loader_stream_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_direct_loader_contract():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert len(contract["streams"]) == 3
    assert contract["streams"][0]["pointer_address"] == 0x03CA32


def test_census_boundaries():
    census = optional_path(
        ROOT / "build/m12-auto50-full-graphics-census.json",
        "AUTO50 graphics census")
    if census is None:
        return
    rows = MODULE.verify_census(census)
    assert rows[0]["end"] == 0x191F09
    assert rows[-1]["end"] == 0x19EC4C


if __name__ == "__main__":
    test_direct_loader_contract()
    test_census_boundaries()
    print("M12 direct-loader stream helper tests passed")
