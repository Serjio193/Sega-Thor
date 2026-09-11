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
        "screen_descriptor_missing_direct_calls": [
            {**use, "expected_call_site": _hex(use["expected_call_site"]),
             "descriptor": _hex(use["descriptor"]), "stream": _hex(use["stream"])}
            for use in missing
        ],
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
