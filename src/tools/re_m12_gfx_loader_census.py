"""Build an exact direct-xref census for the 0x00D406 graphics loader."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-loader-census.v1"
SCREEN_SCHEMA = "oasis.m68k.screen-resource-boundary.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = 0x00D406
CALL_BYTES = b"\x4E\xB9\x00\x00\xD4\x06"


def direct_call_sites(rom: bytes) -> list[int]:
    """Return every exact absolute-long JSR to TARGET, including data false positives."""
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def screen_sites(screen_report: dict) -> list[dict]:
    if screen_report.get("schema") != SCREEN_SCHEMA:
        raise ValueError("unexpected screen-resource report schema")
    result = []
    for record in screen_report.get("records", []):
        for use in record.get("uses", []):
            descriptor = int(use["descriptor"])
            result.append({
                "group": int(use["group"]),
                "index": int(use["index"]),
                "descriptor": descriptor,
                "expected_call_site": descriptor + 0x1A,
                "stream": int(record["start"]),
            })
    return result


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def nearby_call(expected: int, calls: list[int], window: int = 16) -> dict | None:
    """Report bounded forward adjacency without proving control-flow or A1."""
    candidates = [site for site in calls if expected < site <= expected + window]
    if not candidates:
        return None
    site = min(candidates)
    return {
        "site": _hex(site),
        "distance": site - expected,
        "classification": "BOUNDED_FORWARD_ADJACENCY_CANDIDATE",
        "proof_limit": "adjacency does not prove A1 provenance or unconditional control flow",
    }


def descriptor_shaped_calls(rom: bytes, calls: list[int], screen_calls: set[int]) -> list[dict]:
    """Find unmatched calls whose preceding 26 bytes match the D406 record shape."""
    result = []
    for site in calls:
        if site in screen_calls or site < 0x1A:
            continue
        record = site - 0x1A
        pointer = int.from_bytes(rom[record + 4:record + 8], "big")
        ids = tuple(rom[record + 8:record + 12])
        if pointer == 0 or pointer >= len(rom) or any(value >= 108 for value in ids):
            continue
        result.append({
            "call_site": _hex(site),
            "record_start": _hex(record),
            "record_end": _hex(site),
            "record_size": 0x1A,
            "rom_pointer": _hex(pointer),
            "resource_ids": list(ids),
            "classification": "D406_PRECEDING_DESCRIPTOR_SHAPED_CANDIDATE",
            "promotion": False,
            "proof_limit": "record shape and pointer do not prove caller A1 or exact family boundary",
        })
    return result


def build_report(rom: bytes, screen_report: dict) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    calls = direct_call_sites(rom)
    uses = screen_sites(screen_report)
    call_set = set(calls)
    expected = [use["expected_call_site"] for use in uses]
    expected_set = set(expected)
    matched = sorted(call_set & expected_set)
    unmatched = sorted(call_set - expected_set)
    missing = [use for use in uses if use["expected_call_site"] not in call_set]
    missing_records = []
    for use in missing:
        item = {**use}
        item["expected_call_site"] = _hex(use["expected_call_site"])
        item["descriptor"] = _hex(use["descriptor"])
        item["stream"] = _hex(use["stream"])
        item["nearby_direct_call"] = nearby_call(use["expected_call_site"], calls)
        missing_records.append(item)
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "rom_size": len(rom),
        "target": _hex(TARGET),
        "encoding": "JSR abs.l target bytes 4E B9 00 00 D4 06",
        "direct_call_count": len(calls),
        "direct_call_sites": [_hex(site) for site in calls],
        "screen_descriptor_use_count": len(uses),
        "screen_descriptor_unique_count": len({use["descriptor"] for use in uses}),
        "screen_descriptor_expected_call_count": len(expected),
        "screen_descriptor_expected_unique_call_count": len(expected_set),
        "screen_descriptor_direct_call_count": len(matched),
        "screen_descriptor_missing_direct_call_count": len(missing),
        "unmatched_direct_call_count": len(unmatched),
        "unmatched_direct_call_sites": [_hex(site) for site in unmatched],
        "screen_descriptor_direct_call_sites": [_hex(site) for site in matched],
        "screen_descriptor_missing_direct_calls": missing_records,
        "unmatched_descriptor_shaped_candidates": descriptor_shaped_calls(
            rom, unmatched, set(matched)),
        "classification": {
            "screen_descriptor_direct": len(matched),
            "screen_descriptor_missing_direct": len(missing),
            "unmatched_direct": len(unmatched),
        },
        "promotion": {
            "performed": False,
            "reason": "xref census does not prove A1 provenance, ownership, or resource boundaries",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("screen_report", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.rom.read_bytes(), json.loads(args.screen_report.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        "direct_calls": report["direct_call_count"],
        "screen_direct": report["screen_descriptor_direct_call_count"],
        "screen_missing": report["screen_descriptor_missing_direct_call_count"],
        "unmatched": report["unmatched_direct_call_count"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
