"""Validate and summarize the bounded, payload-free A372 runtime capture."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def validate_capture(capture: dict) -> None:
    if capture.get("schema") != "oasis.m68k.m12-a372-runtime.v1":
        raise ValueError("unexpected runtime capture schema")
    if capture.get("canonical_rom_sha256") != ROM_SHA256:
        raise ValueError("capture ROM identity mismatch")
    if capture.get("expected_rom_sha256") != ROM_SHA256:
        raise ValueError("capture expected ROM identity mismatch")
    if capture.get("state_writes_emitted") is not False:
        raise ValueError("runtime probe must not emit state writes")
    for item in capture.get("events", []):
        if "source_bytes" in item or "rom_payload" in item:
            raise ValueError("payload field present in runtime capture")


def summarize(capture: dict, rom: bytes | None = None) -> dict:
    validate_capture(capture)
    if rom is not None:
        if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
            raise ValueError("canonical ROM identity mismatch")
    events = capture.get("events", [])
    a342 = [item for item in events if item.get("kind") == "A342"]
    a372 = [item for item in events if item.get("kind") == "A372"]
    roots = {item["root"]: 0 for item in a342}
    for item in a342:
        roots[item["root"]] += 1
    return {
        "schema": "oasis.m68k.m12-a372-runtime-report.v1",
        "status": "RUNTIME_A372_REGISTER_ROOT_CAPTURE_OBSERVED"
        if a342 and a372
        else "RUNTIME_A372_TARGET_NOT_REACHED",
        "canonical_rom_sha256": ROM_SHA256,
        "emulator": capture.get("emulator"),
        "version": capture.get("version"),
        "frames_executed": capture.get("frames_executed"),
        "hook_counts": capture.get("hook_counts", {}),
        "root_counts_from_events": roots,
        "root_counts_reported": capture.get("root_counts", {}),
        "a342_samples": a342[:32],
        "a372_samples": a372[:32],
        "source_write_count": len(capture.get("source_writes", [])),
        "source_write_cap": capture.get("write_cap"),
        "evidence_class": "RUNTIME_REGISTER_AND_RAM_CONTEXT",
        "join": "A196 -> A342 -> A372 -> FF13CC -> DMA 0x27EC -> SAT VRAM 0xD000",
        "semantic_result": {
            "object": "NOT_PROVEN",
            "animation": "NOT_PROVEN",
            "frame": "NOT_PROVEN",
            "sprite_piece": "NOT_PROVEN",
        },
        "ownership": {"source_owned_before": 1475368, "source_owned_after": 1475368, "rom_bytes_added": 0},
    }


def write_report(path: Path, result: dict) -> None:
    path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    capture = json.loads(args.capture.read_text(encoding="utf-8"))
    result = summarize(capture, args.rom.read_bytes() if args.rom else None)
    if args.output:
        write_report(args.output, result)
    else:
        print(json.dumps(result, indent=2, sort_keys=True))
