"""Build an exact direct-caller census for the 0x0036D4 graphics wrapper."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-36d4-census.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = 0x0036D4
CALL_BYTES = b"\x4E\xB9\x00\x00\x36\xD4"
BODY_START = 0x0036D4
BODY_END = 0x00372A
DECOMPRESSOR = 0x003820
POSTPROCESSOR = 0x002CBC
RAM_DESTINATION = 0x00FF2FA8
EXPECTED_CALLS = {
    0x03D25E: 0x0020,
    0x03D2D8: 0x4000,
    0x03D512: 0x0020,
    0x03DD70: 0x0020,
    0x03DF14: 0x0020,
    0x03E544: 0x0020,
}
CALLER_SETUP_SUFFIX = bytes.fromhex("2A3C60000003")


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def direct_call_sites(rom: bytes) -> list[int]:
    """Return every exact absolute-long JSR to the proven wrapper."""
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def parse_caller_setup(rom: bytes, call_site: int) -> dict:
    """Parse the exact D0/A5 setup immediately before a direct wrapper call."""
    setup_start = call_site - 10
    if setup_start < 0:
        raise ValueError(f"caller setup underflow at {_hex(call_site)}")
    move_d0 = rom[setup_start:setup_start + 4]
    move_a5 = rom[setup_start + 4:call_site]
    if move_d0[:2] != bytes.fromhex("303C"):
        raise ValueError(f"missing immediate D0 setup at {_hex(call_site)}")
    if move_a5 != CALLER_SETUP_SUFFIX:
        raise ValueError(f"unexpected caller setup at {_hex(call_site)}")
    if rom[call_site:call_site + len(CALL_BYTES)] != CALL_BYTES:
        raise ValueError(f"unexpected 36D4 encoding at {_hex(call_site)}")
    return {
        "call_site": _hex(call_site),
        "setup_start": _hex(setup_start),
        "d0": int.from_bytes(move_d0[2:4], "big"),
        "a5_literal": "0x60000003",
        "classification": "EXACT_CALLER_SETUP_RAM_MEDIATED_SOURCE",
    }


def _bsr_target(rom: bytes, address: int) -> int:
    if rom[address:address + 2] != bytes.fromhex("6100"):
        raise ValueError(f"expected BSR.W at {_hex(address)}")
    displacement = int.from_bytes(rom[address + 2:address + 4], "big", signed=True)
    return address + 2 + displacement


def analyze_body(rom: bytes) -> dict:
    """Verify the exact bounded wrapper body and its source-proof limit."""
    body = rom[BODY_START:BODY_END]
    if len(body) != BODY_END - BODY_START:
        raise ValueError("ROM is shorter than the 36D4 wrapper body")
    if body[:4] != bytes.fromhex("48E707E2") or body[-2:] != bytes.fromhex("4E75"):
        raise ValueError("unexpected 36D4 body boundary bytes")
    decompressor_call = 0x0036EA
    postprocessor_call = 0x003720
    if _bsr_target(rom, decompressor_call) != DECOMPRESSOR:
        raise ValueError("36D4 body does not call the proven 0x3820 routine")
    if _bsr_target(rom, postprocessor_call) != POSTPROCESSOR:
        raise ValueError("36D4 body does not call the proven 0x2CBC helper")
    destination_bytes = bytes.fromhex("43F900FF2FA8")
    destination_site = 0x0036E2
    if rom[destination_site:destination_site + len(destination_bytes)] != destination_bytes:
        raise ValueError("36D4 body does not set the proven RAM destination")
    pre_call = rom[BODY_START:decompressor_call]
    if bytes.fromhex("2079") in pre_call or bytes.fromhex("41F9") in pre_call:
        raise ValueError("36D4 source classification unexpectedly gained a local A0 load")
    return {
        "range": [_hex(BODY_START), _hex(BODY_END)],
        "size": BODY_END - BODY_START,
        "decompressor_call": _hex(DECOMPRESSOR),
        "postprocessor_call": _hex(POSTPROCESSOR),
        "ram_destination": _hex(RAM_DESTINATION),
        "source_classification": "CALLER_A0_NOT_ROM_PROVEN",
        "proof_limit": "A0 is inherited at wrapper entry; no canonical ROM source or exact stream boundary is established",
    }


def build_report(rom: bytes) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    calls = direct_call_sites(rom)
    expected_sites = sorted(EXPECTED_CALLS)
    if calls != expected_sites:
        raise ValueError(
            f"36D4 direct-call set mismatch: expected {expected_sites}, got {calls}")
    setups = []
    for site in calls:
        setup = parse_caller_setup(rom, site)
        if setup["d0"] != EXPECTED_CALLS[site]:
            raise ValueError(f"unexpected D0 at {_hex(site)}")
        setups.append(setup)
    body = analyze_body(rom)
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "rom_size": len(rom),
        "wrapper": _hex(TARGET),
        "direct_call_count": len(calls),
        "direct_call_sites": [_hex(site) for site in calls],
        "caller_setups": setups,
        "body": body,
        "promotion": {
            "performed": False,
            "new_source_owned_bytes": 0,
            "reason": "wrapper source A0 is caller-provided and not ROM-proven",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.rom.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        "direct_calls": report["direct_call_count"],
        "wrapper_bytes": report["body"]["size"],
        "promoted_bytes": report["promotion"]["new_source_owned_bytes"],
        "source_classification": report["body"]["source_classification"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
