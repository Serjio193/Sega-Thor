"""Classify the five residual non-screen direct 0x00D406 callers."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-d406-residual-census.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CALL_BYTES = bytes.fromhex("4EB90000D406")
EXPECTED_TOTAL_CALLS = 173
RESIDUAL = {
    0x02DB40: {
        "body_start": 0x02DB3C,
        "body_end": 0x02DBB6,
        "body_sha256": "d3e7ac760f5606cd199dc59acb33b0b305be286587cf921f274494b4cc52055a",
        "classification": "D406_POST_SOURCE_THEN_3820_CONTINUATION",
        "edges": [0x003820, 0x00D7C0],
        "source_blocker": "D406_POST_SOURCE_NOT_ROM_PROVEN",
        "proof": "D406 writes the post-source field at 0x00FF17AA; the caller reloads it for 0x3820 at 0x02DB52",
    },
    0x02DD8C: {
        "body_start": 0x02DD80,
        "body_end": 0x02DD9C,
        "body_sha256": "ce292e39f5c044d6ef1d82ec7b4df79b07ef593cd6eb0666d9cd1ad215cd943f",
        "classification": "INHERITED_A2_ZERO_SELECTOR_WRAPPER",
        "edges": [],
        "source_blocker": "CALLER_A2_AND_A1_NOT_ROM_PROVEN",
        "proof": "MOVEA.L A2,A0 and MOVEQ #0,D0 precede D406; only RAM flag 0x00FF17C2 is written after return",
    },
    0x02DE58: {
        "body_start": 0x02DE4C,
        "body_end": 0x02DE68,
        "body_sha256": "ce292e39f5c044d6ef1d82ec7b4df79b07ef593cd6eb0666d9cd1ad215cd943f",
        "classification": "INHERITED_A2_ZERO_SELECTOR_WRAPPER",
        "edges": [],
        "source_blocker": "CALLER_A2_AND_A1_NOT_ROM_PROVEN",
        "proof": "Exact body duplicate of 0x02DD80..0x02DD9C; MOVEA.L A2,A0 and MOVEQ #0,D0 precede D406",
    },
    0x02DFC2: {
        "body_start": 0x02DFC0,
        "body_end": 0x02DFCA,
        "body_sha256": "27722d76869040f2204549b26474454c5bb94c92589d8b23fc546db3a9163854",
        "classification": "ZERO_SELECTOR_INHERITED_ARGUMENT_WRAPPER",
        "edges": [],
        "source_blocker": "CALLER_A0_AND_A1_NOT_ROM_PROVEN",
        "proof": "Only MOVEQ #0,D0 is local before D406; A0 and A1 are inherited arguments",
    },
    0x02E084: {
        "body_start": 0x02E080,
        "body_end": 0x02E08C,
        "body_sha256": "b13bf5130c00236a7f4d6d6ea7580947b6d9392dfa64dbe5ef0c1b78f34f2fac",
        "classification": "ZERO_D0_TO_A0_WRAPPER",
        "edges": [],
        "source_blocker": "ZERO_SOURCE_AND_INHERITED_A1",
        "proof": "MOVEA.L D0,A0 follows MOVEQ #0,D0 before D406; A1 is inherited and no ROM source is defined",
    },
}


def direct_call_sites(rom: bytes) -> list[int]:
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def body_record(rom: bytes, site: int) -> dict:
    contract = RESIDUAL[site]
    body = rom[contract["body_start"]:contract["body_end"]]
    digest = hashlib.sha256(body).hexdigest()
    if digest != contract["body_sha256"]:
        raise ValueError(f"unexpected residual body at 0x{site:06X}: {digest}")
    return {
        "start": f"0x{contract['body_start']:06X}",
        "end": f"0x{contract['body_end']:06X}",
        "bytes": contract["body_end"] - contract["body_start"],
        "sha256": digest,
        "direct_edges": [f"0x{edge:06X}" for edge in contract["edges"]],
    }


def build_report(rom: bytes) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    calls = direct_call_sites(rom)
    if len(calls) != EXPECTED_TOTAL_CALLS:
        raise ValueError(f"D406 call count mismatch: {len(calls)}")
    rows = []
    for site, contract in RESIDUAL.items():
        if site not in calls:
            raise ValueError(f"residual call is missing: 0x{site:06X}")
        rows.append({
            "call_site": f"0x{site:06X}",
            "classification": contract["classification"],
            "body": body_record(rom, site),
            "proof": contract["proof"],
            "source_proven": False,
            "source_blocker": contract["source_blocker"],
            "promotion": "blocked; residual caller classification only",
        })
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "target": "0x00D406",
        "total_direct_call_count": len(calls),
        "residual_call_count": len(rows),
        "residual_call_sites": [row["call_site"] for row in rows],
        "rows": rows,
        "promotion": {"performed": False, "bytes": 0,
                       "reason": "all five residual callers lack a closed ROM source contract"},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.rom.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"d406_calls": report["total_direct_call_count"],
                      "residual_calls": report["residual_call_count"],
                      "promoted_bytes": report["promotion"]["bytes"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
