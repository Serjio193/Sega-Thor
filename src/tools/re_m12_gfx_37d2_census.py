"""Census the direct 0x37D2 graphics wrapper family without promoting data."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-37d2-census.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = 0x37D2
CALL_BYTES = bytes.fromhex("4EB9000037D2")
SOURCE_OWNED = {
    "CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
    "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
    "LOCAL_ROM_DERIVED_ASSET",
}

DIRECT_CONTRACTS = {
    0x00D9B2: {
        "classification": "INHERITED_A6_SOURCE",
        "source_origin": "A6 inherited by 0x00D9A4",
        "destination": "0x00FF2FA8",
        "blocker": "CALLER_SOURCE_LITERAL_REQUIRED",
    },
    0x02F6B6: {
        "classification": "D406_POST_SOURCE_CONTINUATION",
        "source_origin": "A0 inherited from 0x00D406 post-source state",
        "destination": "0x00FF2FA8",
        "blocker": "D406_POST_SOURCE_NOT_ROM_PROVEN",
    },
    0x03C0DE: {
        "classification": "DIRECT_ROM_SOURCE",
        "setup_start": 0x03C0CE,
        "source": 0x1744EE,
        "destination": "0x00FF2FA8",
        "d0": 0x4B00,
    },
    0x03C2BE: {
        "classification": "DIRECT_ROM_SOURCE",
        "setup_start": 0x03C2AE,
        "source": 0x18CCD0,
        "destination": "0x00FF2FA8",
        "d0": 0x7080,
    },
    0x03C632: {
        "classification": "DIRECT_ROM_SOURCE",
        "setup_start": 0x03C622,
        "source": 0x18EC26,
        "destination": "0x00FF2FA8",
        "d0": 0x6A40,
    },
    0x03CA40: {
        "classification": "DIRECT_ROM_SOURCE",
        "setup_start": 0x03CA30,
        "source": 0x1911EA,
        "destination": "0x00FF2FA8",
        "d0": 0x2580,
    },
    0x03CD54: {
        "classification": "DIRECT_ROM_SOURCE",
        "setup_start": 0x03CD44,
        "source": 0x19911A,
        "destination": "0x00FF2FA8",
        "d0": 0x6400,
    },
}


def direct_call_sites(rom: bytes) -> list[int]:
    """Return every exact absolute-long JSR to 0x37D2."""
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def parse_direct_setup(rom: bytes, call_site: int, setup_start: int) -> dict:
    """Require the exact source/destination/immediate setup before a wrapper."""
    setup = rom[setup_start:call_site + len(CALL_BYTES)]
    if len(setup) != 0x16:
        raise ValueError("unexpected 0x37D2 setup length")
    if setup[0:2] != b"\x41\xF9" or setup[6:8] != b"\x43\xF9":
        raise ValueError("0x37D2 setup does not define A0/A1 with LEA abs.l")
    if setup[12:14] != b"\x30\x3C" or setup[16:22] != CALL_BYTES:
        raise ValueError("0x37D2 setup has an unexpected instruction form")
    source = int.from_bytes(setup[2:6], "big")
    destination = int.from_bytes(setup[8:12], "big")
    d0 = int.from_bytes(setup[14:16], "big")
    return {
        "setup_start": f"0x{setup_start:06X}",
        "setup_end": f"0x{call_site + len(CALL_BYTES):06X}",
        "setup_sha256": hashlib.sha256(setup).hexdigest(),
        "source": source,
        "destination": f"0x{destination:08X}",
        "d0": d0,
    }


def owner_span(manifest: dict, start: int, end: int) -> dict:
    entries = sorted(manifest["entries"], key=lambda item: int(item["start"]))
    covering = [entry for entry in entries
                if int(entry["start"]) < end and int(entry["end"]) > start]
    exact = [entry for entry in covering
             if int(entry["start"]) <= start and int(entry["end"]) >= end]
    return {
        "entry_count": len(covering),
        "exact_entry": len(exact) == 1,
        "source_owned": bool(exact) and exact[0]["kind"] in SOURCE_OWNED,
        "owner_kind": exact[0]["kind"] if exact else None,
        "owner_start": int(exact[0]["start"]) if exact else None,
        "owner_end": int(exact[0]["end"]) if exact else None,
    }


def build_report(rom: bytes, ancient_report: dict, manifest: dict) -> dict:
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    records = {int(item["start"]): item
               for item in ancient_report["ancient_sweep"]["records"]}
    sites = direct_call_sites(rom)
    if sites != sorted(DIRECT_CONTRACTS):
        raise ValueError(f"unexpected direct 0x37D2 call census: {sites}")
    rows = []
    for site in sites:
        contract = dict(DIRECT_CONTRACTS[site])
        row = {"call_site": f"0x{site:06X}", **contract}
        if contract["classification"] == "DIRECT_ROM_SOURCE":
            setup = parse_direct_setup(rom, site, contract["setup_start"])
            if setup["source"] != contract["source"] or setup["d0"] != contract["d0"]:
                raise ValueError(f"direct setup mismatch at 0x{site:06X}")
            record = records.get(contract["source"])
            if record is None or not record["deterministic"]:
                raise ValueError(f"missing deterministic source at 0x{contract['source']:06X}")
            if int(record["end"]) != contract["source"] + int(record["compressed_bytes"]):
                raise ValueError("Ancient record boundary is inconsistent")
            row.update(setup)
            row.update({
                "source": f"0x{contract['source']:06X}",
                "source_end": f"0x{int(record['end']):06X}",
                "compressed_bytes": int(record["compressed_bytes"]),
                "decompressed_bytes": int(record["decompressed_bytes"]),
                "mode": record["mode"],
                "output_sha256": record["output_sha256"],
                "owner": owner_span(manifest, contract["source"], int(record["end"])),
                "promotion": "already_source_owned",
            })
        else:
            row["promotion"] = "blocked"
        rows.append(row)
    return {
        "schema": SCHEMA,
        "rom_sha256": ROM_SHA256,
        "target": f"0x{TARGET:06X}",
        "encoding": CALL_BYTES.hex(),
        "direct_call_count": len(sites),
        "direct_call_sites": [f"0x{site:06X}" for site in sites],
        "direct_literal_count": sum(row["classification"] == "DIRECT_ROM_SOURCE"
                                     for row in rows),
        "already_source_owned_bytes": sum(row.get("compressed_bytes", 0)
                                           for row in rows),
        "promoted_bytes": 0,
        "rows": rows,
        "promotion": {
            "performed": False,
            "reason": "all five direct wrapper streams are already owned; two sites inherit blocked source state",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("ancient_report", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.rom.read_bytes(),
                          json.loads(args.ancient_report.read_text()),
                          json.loads(args.manifest.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        "direct_calls": report["direct_call_count"],
        "direct_literal": report["direct_literal_count"],
        "already_source_owned_bytes": report["already_source_owned_bytes"],
        "promoted_bytes": report["promoted_bytes"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
