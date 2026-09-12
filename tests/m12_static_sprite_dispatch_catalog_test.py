import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_static_sprite_dispatch_catalog",
    ROOT / "src/tools/m12_static_sprite_dispatch_catalog.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_read_longwords_is_big_endian_and_bounded():
    data = bytes.fromhex("0003AAAE0017A790")
    assert MODULE.read_longwords(data, 0, 2) == [0x0003AAAE, 0x0017A790]
    try:
        MODULE.read_longwords(data, 6, 1)
    except ValueError as error:
        assert "exceeds input" in str(error)
    else:
        raise AssertionError("out-of-bounds table was accepted")


def test_overlap_contracts_are_half_open():
    assert MODULE.overlap_ranges(0x10, 0x20, 0x1C, 0x30) == {
        "start": 0x1C,
        "end": 0x20,
    }
    assert MODULE.overlap_ranges(0x10, 0x20, 0x20, 0x30) is None


def test_descriptor_address_classification_is_fail_closed():
    assert MODULE.classify_entry_address(MODULE.DESCRIPTOR_START) == (
        "DESCRIPTOR_TABLE_ADDRESS"
    )
    assert MODULE.classify_entry_address(MODULE.DESCRIPTOR_END) == (
        "STATIC_TARGET_ADDRESS"
    )


if __name__ == "__main__":
    test_read_longwords_is_big_endian_and_bounded()
    test_overlap_contracts_are_half_open()
    test_descriptor_address_classification_is_fail_closed()
    print("M12 static sprite dispatch catalog tests passed")
