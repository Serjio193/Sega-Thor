"""Validate targeted runtime source provenance without promoting decoded payloads."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from m12_gfx_ancient import deterministic_decode


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = "0x00003820"
DYNAMIC_CALLERS = {
    "0x0000D54A", "0x0000D650", "0x0002DB52", "0x0002F6A0",
    "0x0003B236", "0x0003B28A", "0x0003B2FE", "0x0003C07C",
    "0x0003D5AE", "0x0003E61A",
}
RUNTIME_CLOSED = {"0x0003B236", "0x0003B28A", "0x0003B2FE"}
REMAINING = sorted(DYNAMIC_CALLERS - RUNTIME_CLOSED)


def number(value: str) -> int:
    return int(value, 0)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def owner(entries: list[dict], start: int, end: int) -> dict:
    matches = [item for item in entries
               if int(item["start"]) <= start and end <= int(item["end"])]
    if len(matches) != 1:
        raise ValueError(f"source range has {len(matches)} manifest owners")
    return matches[0]


def validate_capture(payload: dict, rom: bytes, entries: list[dict]) -> dict:
    if payload.get("schema") != "oasis.m68k.m12-gfx-runtime-provenance.v1":
        raise ValueError("unexpected runtime provenance schema")
    if payload.get("canonical_rom_sha256") != ROM_SHA256:
        raise ValueError("capture canonical ROM identity mismatch")
    if payload.get("target") != TARGET or payload.get("writes_emitted") is not False:
        raise ValueError("capture target/write contract failed")
    paired = []
    for item in payload.get("captures", []):
        caller = item.get("caller")
        if caller not in DYNAMIC_CALLERS:
            continue
        registers = item.get("registers", {})
        addresses = registers.get("a", [])
        if len(addresses) < 2:
            raise ValueError(f"capture lacks A0/A1 for {caller}")
        source = number(addresses[0])
        destination = number(addresses[1])
        if not 0 <= source < len(rom):
            raise ValueError(f"runtime source is not ROM: {caller} {source:#x}")
        decoded = deterministic_decode(rom, source)
        item_owner = owner(entries, source, decoded.end)
        if item_owner.get("kind") != "LOCAL_ROM_DERIVED_ASSET":
            raise ValueError(f"runtime source is not owned: {source:#x}")
        paired.append({
            "caller": caller,
            "frame": int(item["frame"]),
            "source": source,
            "destination": destination,
            "end": decoded.end,
            "compressed_bytes": decoded.consumed,
            "decompressed_bytes": len(decoded.output),
            "mode": decoded.mode,
            "output_sha256": hashlib.sha256(decoded.output).hexdigest(),
            "manifest_owner": [int(item_owner["start"]), int(item_owner["end"])],
        })
    if not paired:
        raise ValueError("no dynamic caller was paired with 0x3820")
    observed = {item["caller"] for item in paired}
    if observed != RUNTIME_CLOSED:
        raise ValueError(f"runtime caller closure mismatch: {sorted(observed)}")
    return {
        "frames_executed": int(payload["frames_executed"]),
        "target_hits": len(payload.get("captures", [])),
        "paired_hits": len(paired),
        "paired": paired,
        "closed_callers": sorted(observed),
    }


def run(args: argparse.Namespace) -> None:
    rom = args.rom.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    manifest = load(args.manifest)
    first = load(args.capture_a)
    second = load(args.capture_b)
    first_bytes = args.capture_a.read_bytes()
    second_bytes = args.capture_b.read_bytes()
    if first_bytes != second_bytes:
        raise ValueError("runtime captures are not byte-identical")
    evidence = validate_capture(first, rom, manifest["entries"])
    if evidence != validate_capture(second, rom, manifest["entries"]):
        raise ValueError("runtime capture validation differs between replays")
    result = {
        "schema": "oasis.m68k.m12-gfx-runtime-provenance-report.v1",
        "canonical_rom_sha256": ROM_SHA256,
        "capture_sha256": hashlib.sha256(first_bytes).hexdigest(),
        "capture_byte_identical": True,
        "evidence": evidence,
        "closed_callers": sorted(RUNTIME_CLOSED),
        "remaining_dynamic_callers": REMAINING,
        "promotion": {"bytes": 0, "reason": "all runtime-closed streams already manifest-owned"},
        "fixed_point": False,
    }
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--capture-a", type=Path, required=True)
    parser.add_argument("--capture-b", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
