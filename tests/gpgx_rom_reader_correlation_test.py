import importlib.util
import json
import tempfile
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "src/tools/oasis_gpgx_rom_reader_correlation.py"
SPEC = importlib.util.spec_from_file_location("gpgx_reader_correlation", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def fixture(root):
    bitmap = bytearray(MODULE.ROM_SIZE // 8)
    for address in range(0x100, 0x106):
        bitmap[address >> 3] |= 1 << (address & 7)
    ranges = root / "ranges.json"
    ranges.write_text(json.dumps([
        {"start": "0x000100", "end": "0x000101", "bytes_observed": 2,
         "first_reader_pc": "0x000200", "access_width": 2,
         "reader_executed": True},
        {"start": "0x000102", "end": "0x000103", "bytes_observed": 2,
         "first_reader_pc": "0x000204", "access_width": 2,
         "reader_executed": True},
        {"start": "0x000104", "end": "0x000105", "bytes_observed": 2,
         "first_reader_pc": "0x000200", "access_width": 2,
         "reader_executed": True},
    ]), encoding="utf-8")
    return bytes(bitmap), ranges


def test_known_reader_and_unknown_reader_grouping():
    with tempfile.TemporaryDirectory() as directory:
        bitmap, ranges_path = fixture(Path(directory))
        regions = MODULE.read_regions(ranges_path, bitmap)
        groups = MODULE.reader_group(
            regions, {0x200, 0x204}, {0x200: "CODE_STATIC_SUPPORTED"}, [])
        assert groups[0]["classification"] == "CODE_STATIC_SUPPORTED"
        assert groups[0]["region_count"] == 2
        assert groups[0]["total_unique_rom_bytes"] == 4
        assert groups[1]["classification"] == "RUNTIME_EXECUTED_UNKNOWN"


def test_duplicate_regions_are_not_double_counted():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        bitmap = bytearray(MODULE.ROM_SIZE // 8)
        bitmap[0x100 >> 3] |= 1 << (0x100 & 7)
        path = root / "duplicate.json"
        item = {"start": "0x100", "end": "0x100", "bytes_observed": 1,
                "first_reader_pc": "0x200", "access_width": 1,
                "reader_executed": True}
        path.write_text(json.dumps([item, item]), encoding="utf-8")
        assert len(MODULE.read_regions(path, bytes(bitmap))) == 1


def test_first_reader_only_is_explicit():
    with tempfile.TemporaryDirectory() as directory:
        bitmap, ranges_path = fixture(Path(directory))
        regions = MODULE.read_regions(ranges_path, bitmap)
        groups = MODULE.reader_group(regions, {0x200, 0x204}, {}, [])
        assert len(groups) == 2
        assert all(item["classification"] == "RUNTIME_EXECUTED_UNKNOWN" for item in groups)


def test_mismatched_byte_count_fails():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        bitmap = bytearray(MODULE.ROM_SIZE // 8)
        bitmap[0x100 >> 3] |= 1 << (0x100 & 7)
        path = root / "mismatch.json"
        path.write_text(json.dumps([{
            "start": "0x100", "end": "0x100", "bytes_observed": 2,
            "first_reader_pc": "0x200", "access_width": 1,
            "reader_executed": True,
        }]), encoding="utf-8")
        try:
            MODULE.read_regions(path, bytes(bitmap))
        except ValueError:
            return
        raise AssertionError("mismatched region byte count was accepted")


def test_bad_provenance_fails():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        rom = root / "bad.rom"
        bitmap = root / "bitmap.bin"
        meta = root / "meta.json"
        rom.write_bytes(bytes(3145728))
        bitmap.write_bytes(bytes(MODULE.ROM_SIZE // 8))
        meta.write_text(json.dumps({"rom_size": MODULE.ROM_SIZE,
                                    "gpgx_sha": "27426f00aa68f9f358c86919e8a40985326fa05b"}),
                        encoding="utf-8")
        try:
            MODULE.validate_provenance(rom, meta, bitmap,
                                       {"canonical_rom_sha256": MODULE.CANONICAL_SHA256,
                                        "source": "GPGX_MANUAL_REALTIME"})
        except ValueError:
            return
        raise AssertionError("bad ROM provenance was accepted")


def test_json_is_deterministic():
    value = {"reader_groups": [{"reader_pc": "0x000200", "bytes": 2}],
             "regions": [{"region_start": "0x000100"}]}
    first = json.dumps(value, indent=2, sort_keys=True) + "\n"
    second = json.dumps(value, indent=2, sort_keys=True) + "\n"
    assert first == second


if __name__ == "__main__":
    for test in (test_known_reader_and_unknown_reader_grouping,
                 test_duplicate_regions_are_not_double_counted,
                 test_first_reader_only_is_explicit,
                 test_mismatched_byte_count_fails,
                 test_bad_provenance_fails,
                 test_json_is_deterministic):
        test()
    print("GPGX reader correlation tests passed")
