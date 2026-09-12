"""Payload-free static join from the frame scheduler to the A372 SAT producer."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def hx(value: int) -> str:
    return f"0x{value:06X}"


def read16(rom: bytes, address: int) -> int:
    return int.from_bytes(rom[address : address + 2], "big")


def read32(rom: bytes, address: int) -> int:
    return int.from_bytes(rom[address : address + 4], "big")


def require_bytes(rom: bytes, address: int, expected: str) -> None:
    actual = rom[address : address + len(bytes.fromhex(expected))].hex().upper()
    if actual != expected.replace(" ", "").upper():
        raise ValueError(f"ROM bytes mismatch at {hx(address)}: {actual} != {expected}")


def bsr_target(rom: bytes, address: int) -> int:
    opcode = read16(rom, address)
    if opcode & 0xFF00 != 0x6100:
        raise ValueError(f"not BSR at {hx(address)}")
    if opcode & 0xFF:
        displacement = opcode & 0xFF
        if displacement & 0x80:
            displacement -= 0x100
        return address + 2 + displacement
    displacement = int.from_bytes(rom[address + 2 : address + 4], "big", signed=True)
    return address + 2 + displacement


def jsr_long_target(rom: bytes, address: int) -> int:
    if read16(rom, address) != 0x4EB9:
        raise ValueError(f"not absolute-long JSR at {hx(address)}")
    return read32(rom, address + 2)


def direct_call(rom: bytes, address: int, expected: int) -> dict:
    opcode = read16(rom, address)
    if opcode & 0xFF00 == 0x6100:
        target = bsr_target(rom, address)
        kind = "BSR"
    elif opcode == 0x4EB9:
        target = jsr_long_target(rom, address)
        kind = "JSR_ABS_L"
    else:
        raise ValueError(f"unsupported direct-call form at {hx(address)}")
    if target != expected:
        raise ValueError(f"call target mismatch at {hx(address)}: {hx(target)}")
    return {"pc": hx(address), "kind": kind, "target": hx(target)}


def raw_direct_calls_to(rom: bytes, start: int, end: int, target: int) -> list[str]:
    """Return opcode-level direct-call hits in an already classified code range."""
    hits: list[str] = []
    for address in range(start, end, 2):
        opcode = read16(rom, address)
        if opcode & 0xFF00 == 0x6100 and bsr_target(rom, address) == target:
            hits.append(hx(address))
        elif opcode == 0x4EB9 and jsr_long_target(rom, address) == target:
            hits.append(hx(address))
    return hits


def parse_join(rom: bytes) -> dict:
    if len(rom) != ROM_SIZE:
        raise ValueError("canonical ROM size mismatch")
    digest = hashlib.sha256(rom).hexdigest()
    if digest != ROM_SHA256:
        raise ValueError("canonical ROM SHA-256 mismatch")

    edges = [
        direct_call(rom, 0x008B2E, 0x00557A),
        direct_call(rom, 0x008B42, 0x008E90),
        direct_call(rom, 0x008B86, 0x00A196),
        direct_call(rom, 0x00A19C, 0x00A342),
        direct_call(rom, 0x00A6A0, 0x00A342),
        direct_call(rom, 0x03C6EA, 0x008E90),
        direct_call(rom, 0x03C6F0, 0x00A196),
        direct_call(rom, 0x0000428A, 0x00A6A0),
        direct_call(rom, 0x00004A5A, 0x00A6A0),
    ]
    require_bytes(rom, 0x00A196, "51F900FF1651610001A44A3900FF1996")
    require_bytes(rom, 0x00A342, "4BF900FF13CCDAF900FF188C")
    require_bytes(rom, 0x00A372, "2AC2")
    require_bytes(rom, 0x03C6E4, "50F900FF18584EB900008E904EB90000A196")
    require_bytes(rom, 0x03C328, "50F900FF1858")
    require_bytes(rom, 0x008B22, "610000886100006661006C5C")

    c262_calls_a196 = raw_direct_calls_to(rom, 0x03C262, 0x03C454, 0x00A196)
    return {
        "schema": "oasis.m68k.m12-a372-caller-join.v1",
        "status": "STATIC_SCHEDULER_TO_SHADOW_SAT_JOIN_PROVEN",
        "canonical_rom_sha256": digest,
        "evidence_class": "STATIC_CONSUMER_BACKTRACE_AND_CROSS_SUBSYSTEM_JOIN",
        "bounded_ranges": {
            "main_scheduler": [hx(0x008B22), hx(0x008C42)],
            "a196_caller": [hx(0x00A196), hx(0x00A216)],
            "a372_producer": [hx(0x00A342), hx(0x00A438)],
            "c5b6_caller": [hx(0x03C5B6), hx(0x03C75E)],
            "c262_neighbor": [hx(0x03C262), hx(0x03C454)],
        },
        "known_exact_edges": edges,
        "scheduler_order": [
            {"pc": hx(0x008B2E), "target": hx(0x00557A), "role": "earlier direct call"},
            {"pc": hx(0x008B42), "target": hx(0x008E90), "role": "active-entity movement call"},
            {"pc": hx(0x008B86), "target": hx(0x00A196), "role": "scheduler call"},
        ],
        "a196_to_a372": {
            "entry": hx(0x00A196),
            "pre_call_write": {"pc": hx(0x00A196), "ram": hx(0x00FF1651), "operation": "SF.B"},
            "call": {"pc": hx(0x00A19C), "target": hx(0x00A342)},
            "post_call_read": {"pc": hx(0x00A1A0), "ram": hx(0x00FF1996), "operation": "TST.B"},
            "meaning": "A196 is an enclosing scheduler caller of the proven A342 producer",
        },
        "ff1858_selector_join": {
            "writer": {"pc": hx(0x03C6E4), "operation": "ST.B", "ram": hx(0x00FF1858)},
            "following_calls": [hx(0x008E90), hx(0x00A196)],
            "result": "C5B6 path selects the A372 root before movement/scheduler calls",
            "semantic_limit": "boolean control is proven; root meaning is not",
        },
        "a6a0_family": {
            "callers": [hx(0x0000428A), hx(0x00004A5A)],
            "call": {"pc": hx(0x00A6A0), "target": hx(0x00A342)},
            "result": "separate direct caller family also reaches A342",
        },
        "negative_evidence": {
            "neighbor": hx(0x03C262),
            "writer": {"pc": hx(0x03C328), "operation": "ST.B", "ram": hx(0x00FF1858)},
            "direct_calls_to_a196_in_bounded_range": c262_calls_a196,
            "interpretation": "no opcode-level direct A196 edge occurs in this classified neighbor range; indirect/local BSR effects remain unresolved",
        },
        "join": "0x008B22 -> 0x008E90 -> 0x00A196 -> 0x00A342 -> 0x00A372 -> 0x00FF13CC -> DMA 0x27EC -> SAT VRAM 0xD000",
        "semantic_result": {
            "object": "NOT_PROVEN",
            "animation": "NOT_PROVEN",
            "frame": "NOT_PROVEN",
            "sprite_piece": "NOT_PROVEN",
            "classification": "SCHEDULER_TO_SHADOW_SAT_PROVEN_SEMANTICS_NEUTRAL",
        },
        "ownership": {
            "source_owned_before": 1475368,
            "source_owned_after": 1475368,
            "rom_bytes_added": 0,
        },
    }


def write_report(path: Path, result: dict) -> None:
    path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = parse_join(args.rom.read_bytes())
    if args.output:
        write_report(args.output, result)
    else:
        print(json.dumps(result, indent=2, sort_keys=True))
