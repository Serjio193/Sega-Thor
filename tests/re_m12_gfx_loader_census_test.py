import importlib.util
import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_loader_census", ROOT / "src/tools/re_m12_gfx_loader_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_exact_direct_call_scan_and_screen_split():
    call = MODULE.CALL_BYTES
    rom = bytearray(b"\0" * 0x40)
    rom[0x20:0x20 + len(call)] = call
    rom[0x28:0x28 + len(call)] = call
    screen = {
        "schema": MODULE.SCREEN_SCHEMA,
        "records": [{"start": 0x120, "uses": [{
                "group": 1, "index": 2, "descriptor": 6,
        }]}],
    }
    MODULE.ROM_SHA256 = hashlib.sha256(bytes(rom)).hexdigest()
    report = MODULE.build_report(bytes(rom), screen)
    assert report["direct_call_count"] == 2
    assert report["screen_descriptor_direct_call_count"] == 1
    assert report["unmatched_direct_call_sites"] == ["0x000028"]
    assert not report["promotion"]["performed"]


def test_screen_duplicate_use_is_retained_but_unique_count_is_exact():
    screen = {
        "schema": MODULE.SCREEN_SCHEMA,
        "records": [{"start": 0x200, "uses": [
            {"group": 0, "index": 1, "descriptor": 0x20},
            {"group": 0, "index": 2, "descriptor": 0x20},
        ]}],
    }
    uses = MODULE.screen_sites(screen)
    assert len(uses) == 2
    assert len({item["descriptor"] for item in uses}) == 1


def test_bounded_adjacency_and_descriptor_shape_are_candidates_only():
    call = MODULE.CALL_BYTES
    rom = bytearray(b"\0" * 0x100)
    descriptor = 0x06
    expected = descriptor + 0x1A
    rom[descriptor + 4:descriptor + 8] = (0x80).to_bytes(4, "big")
    rom[descriptor + 8:descriptor + 12] = bytes((1, 2, 3, 4))
    rom[expected + 6:expected + 6 + len(call)] = call
    MODULE.ROM_SHA256 = hashlib.sha256(bytes(rom)).hexdigest()
    screen = {
        "schema": MODULE.SCREEN_SCHEMA,
        "records": [{"start": 0x120, "uses": [{
            "group": 1, "index": 2, "descriptor": descriptor,
        }]}],
    }
    report = MODULE.build_report(bytes(rom), screen)
    assert report["screen_descriptor_missing_direct_calls"][0]["nearby_direct_call"]["distance"] == 6
    assert report["unmatched_descriptor_shaped_candidates"] == []

    other = 0x40
    rom[other + 4:other + 8] = (0x90).to_bytes(4, "big")
    rom[other + 8:other + 12] = bytes((5, 6, 7, 8))
    rom[other + 0x1A:other + 0x1A + len(call)] = call
    MODULE.ROM_SHA256 = hashlib.sha256(bytes(rom)).hexdigest()
    report = MODULE.build_report(bytes(rom), screen)
    assert report["unmatched_descriptor_shaped_candidates"][0]["record_start"] == "0x000040"
    assert report["unmatched_descriptor_shaped_candidates"][0]["promotion"] is False


if __name__ == "__main__":
    test_exact_direct_call_scan_and_screen_split()
    test_screen_duplicate_use_is_retained_but_unique_count_is_exact()
    test_bounded_adjacency_and_descriptor_shape_are_candidates_only()
