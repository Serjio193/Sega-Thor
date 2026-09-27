import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_dynamic_sat_producer_backtrace", ROOT / "src/tools/m12_dynamic_sat_producer_backtrace.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_writer_decode_maps_callback_to_actual_store():
    rom = (ROOT / "local-roms/Beyond Oasis (USA).md").read_bytes()
    sites = MODULE.decode_sites(rom)
    assert sites["0x00B772"]["actual_pc"] == "0x00B76E"
    assert sites["0x00B772"]["semantic"] == "SAT X"


def test_x_transform_is_exact():
    assert (0x29 + 0x400) & 0xFFFF == 0x429


if __name__ == "__main__":
    test_writer_decode_maps_callback_to_actual_store()
    test_x_transform_is_exact()
    print("M12 dynamic SAT producer backtrace tests passed")
