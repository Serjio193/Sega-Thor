"""Classify proven graphics-adjacent sibling helper xrefs fail-closed."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-sibling-census.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CALL_PREFIX = b"\x4E\xB9"
TARGETS = {
    0x00D950: {
        "name": "VDP_TRANSFER_HELPER",
        "body_start": 0x00D950,
        "body_end": 0x00D9A4,
        "body_sha256": "f67208ba4ca7d67d5b1cbabd8104ee876f8444878a76f7de9bd54ba228248f61",
        "direct_edges": [0x00D962],
        "hardware_literals": [0x00C00000, 0x00800000, 0x00000700, 0x00004000],
        "proof": "MOVEM saves/restores D0-D7/A2; bounded body addresses VDP port space through A2=0x00C00000 and has no ROM source operand",
        "source_blocker": "NO_ROM_SOURCE_OPERAND_IN_BOUNDED_BODY",
    },
    0x002CBC: {
        "name": "VDP_FILL_HELPER",
        "body_start": 0x002CBC,
        "body_end": 0x002CE4,
        "body_sha256": "102fb54528c35d21e6c51d1f7a46735e972fa90ea6744d8bb0f75746190aa0da",
        "direct_edges": [],
        "hardware_literals": [0x00C00000, 0x00800000, 0x00000700],
        "proof": "bounded body writes VDP port space through A5=0x00C00000 and has no ROM source operand or nested loader call",
        "source_blocker": "NO_ROM_SOURCE_OPERAND_IN_BOUNDED_BODY",
    },
    0x002E1E: {
        "name": "RAM_STATE_HELPER",
        "body_start": 0x002E1E,
        "body_end": 0x002E78,
        "body_sha256": "2664ff395aa9c559efddeccd13d62d6736670a882f0bab5d086a4f2c4d309806",
        "direct_edges": [0x002F6E],
        "hardware_literals": [],
        "proof": "bounded entry saves/restores address/data registers, reads/writes RAM state, calls local 0x002F6E, and has no ROM source operand or decompressor edge",
        "source_blocker": "NOT_A_ROM_SOURCE_LOADER",
    },
}


def direct_call_sites(rom: bytes, target: int) -> list[int]:
    """Return every exact absolute-long JSR to target."""
    encoding = CALL_PREFIX + target.to_bytes(4, "big")
    return [offset for offset in range(len(rom) - len(encoding) + 1)
            if rom[offset:offset + len(encoding)] == encoding]


def body_record(rom: bytes, target: int) -> dict:
    """Verify the exact bounded body fingerprint and report its contract."""
    contract = TARGETS[target]
    start = contract["body_start"]
    end = contract["body_end"]
    body = rom[start:end]
    digest = hashlib.sha256(body).hexdigest()
    if digest != contract["body_sha256"]:
        raise ValueError(f"unexpected body bytes at 0x{target:06X}: {digest}")
    return {
        "start": f"0x{start:06X}",
        "end": f"0x{end:06X}",
        "bytes": end - start,
        "sha256": digest,
        "direct_edges": [f"0x{edge:06X}" for edge in contract["direct_edges"]],
        "hardware_literals": [f"0x{value:08X}" for value in contract["hardware_literals"]],
    }


def build_report(rom: bytes) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    rows = []
    for target, contract in TARGETS.items():
        sites = direct_call_sites(rom, target)
        rows.append({
            "target": f"0x{target:06X}",
            "classification": contract["name"],
            "direct_call_count": len(sites),
            "direct_call_sites": [f"0x{site:06X}" for site in sites],
            "body": body_record(rom, target),
            "proof": contract["proof"],
            "source_proven": False,
            "source_blocker": contract["source_blocker"],
            "promotion": "blocked; helper classification only",
        })
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "targets": rows,
        "direct_call_count": sum(row["direct_call_count"] for row in rows),
        "promotion": {
            "performed": False,
            "bytes": 0,
            "reason": "sibling helpers do not establish a ROM source or exact resource boundary",
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
        "targets": len(report["targets"]),
        "direct_calls": report["direct_call_count"],
        "promoted_bytes": report["promotion"]["bytes"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
