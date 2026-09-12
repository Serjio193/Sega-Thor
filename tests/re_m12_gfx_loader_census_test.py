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
    assert report["screen_descriptor_missing_direct_calls"][0]["verified_continuation"] is None
    assert report["screen_descriptor_unresolved_blockers"][0]["unresolved_blocker"]["reason"] == "NO_VERIFIED_CONTINUATION"
    assert report["unmatched_descriptor_shaped_candidates"] == []

    other = 0x40
    rom[other + 4:other + 8] = (0x90).to_bytes(4, "big")
    rom[other + 8:other + 12] = bytes((5, 6, 7, 8))
    rom[other + 0x1A:other + 0x1A + len(call)] = call
    MODULE.ROM_SHA256 = hashlib.sha256(bytes(rom)).hexdigest()
    report = MODULE.build_report(bytes(rom), screen)
    assert report["unmatched_descriptor_shaped_candidates"][0]["record_start"] == "0x000040"
    assert report["unmatched_descriptor_shaped_candidates"][0]["promotion"] is False


def test_known_continuation_decoder_requires_exact_call_and_preserves_a1():
    call = MODULE.CALL_BYTES
    rom = bytearray(b"\0" * 0x40)
    rom[0x10:0x12] = bytes.fromhex("7400")
    rom[0x12:0x12 + len(call)] = call
    assert MODULE.verify_continuation(bytes(rom), 0x10, 0x12)["a1_written_before_call"] is False
    assert MODULE.verify_continuation(bytes(rom), 0x10, 0x14) is None


def test_extended_continuation_requires_exact_cfg_report_and_hash():
    call = MODULE.CALL_BYTES
    start = 0x20
    target = start + 2
    rom = bytearray(b"\0" * 0x80)
    rom[start:start + 2] = bytes.fromhex("7400")
    rom[target:target + len(call)] = call
    MODULE.EXTENDED_SCREEN_CONTINUATIONS = {
        start: {"call": target, "pre_call_sha256": hashlib.sha256(
            bytes(rom[start:target])).hexdigest()}}
    report = {
        "schema": "oasis.m68k.re-slice.v1",
        "entry_point": f"0x{start:08X}",
        "instructions": [
            {"address": f"0x{start:08X}", "bytes": "7400",
             "supported": True, "flow": "none", "mnemonic": "moveq"},
            {"address": f"0x{target:08X}", "bytes": call.hex(),
             "supported": True, "flow": "direct_call", "mnemonic": "jsr"},
        ],
        "direct_control_flow": [{"source": f"0x{target:08X}",
                                  "target": "0x0000D406"}],
    }
    result = MODULE.verify_slice_continuation(bytes(rom), report, start, target)
    assert result["classification"] == "VERIFIED_SCREEN_DESCRIPTOR_EXTENDED_CONTINUATION"
    assert result["a1_written_before_call"] is False


if __name__ == "__main__":
    test_exact_direct_call_scan_and_screen_split()
    test_screen_duplicate_use_is_retained_but_unique_count_is_exact()
    test_bounded_adjacency_and_descriptor_shape_are_candidates_only()
    test_known_continuation_decoder_requires_exact_call_and_preserves_a1()
