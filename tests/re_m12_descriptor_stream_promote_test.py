import importlib.util
from pathlib import Path
from m12_local_inputs import optional_bytes, optional_path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_descriptor_stream_promote",
    ROOT / "src/tools/re_m12_descriptor_stream_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_descriptor_pointer_contract():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["record_count"] == 10
    assert contract["record_size"] == 0x16
    assert contract["stream_count"] == 10
    assert contract["unproven_adjacent_stream"] == "0x1F66A0..0x1F683C"


def test_census_requires_exact_end():
    census = ROOT / "build/m12-auto48-census-1ED5EC-200009.json"
    census = optional_path(census, "AUTO48 census")
    if census is None:
        return
    records = MODULE.verify_census(census)
    assert records[0]["start"] == 0x1ED5EC
    assert records[-1]["end"] == 0x1FF10E


if __name__ == "__main__":
    test_descriptor_pointer_contract()
    test_census_requires_exact_end()
    print("M12 descriptor-backed stream helper tests passed")
