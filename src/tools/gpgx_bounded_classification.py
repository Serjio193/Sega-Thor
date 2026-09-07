#!/usr/bin/env python3
"""Bounded, provenance-checked classification for one GPGX code unit."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path
from typing import Any

START = 0x060BB6
END = 0x060BC4
BOUNDARY = "LIKELY_INTERNAL_BLOCK"
TARGET_EDGES = (
    (0x060BAE, 0x060BB6, "FALLTHROUGH"),
    (0x060BC2, 0x060B90, "DIRECT_JUMP"),
    (0x060BAA, 0x060BC4, "CONDITIONAL_BRANCH"),
    (0x060BCC, 0x0604BC, "DIRECT_CALL"),
)


def address(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def fmt(value: int) -> str:
    return f"0x{value:06X}"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def edge_key(item: dict[str, Any]) -> tuple[int, int, str]:
    source = item.get("source_pc", item.get("source"))
    return address(source), address(item["target"]), str(item["kind"])


def target_instructions(decoded: dict[str, Any]) -> list[dict[str, Any]]:
    result = []
    for item in decoded["instructions"]:
        pc = address(item["address"])
        if START <= pc <= END:
            result.append(item)
    return sorted(result, key=lambda item: address(item["address"]))


def verify_rom_bytes(rom: bytes, instructions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    checked = []
    for item in instructions:
        pc = address(item["address"])
        raw = bytes.fromhex(item["raw_bytes"])
        actual = rom[pc:pc + len(raw)]
        if actual != raw:
            raise ValueError(f"ROM bytes mismatch at {fmt(pc)}")
        checked.append({
            "address": fmt(pc),
            "raw_bytes": raw.hex().upper(),
            "decoded_instruction": item["decoded_instruction"],
            "instruction_length": int(item["instruction_length"]),
            "decode_status": item["status"],
            "previous_address_classification": item.get("classification", "UNKNOWN"),
        })
    return checked


def independently_decode_target(
    rom: bytes, bounded_decoder: dict[str, Any], supplied: list[dict[str, Any]]
) -> bool:
    """Prove starts/lengths from independent bounded exact-decoder output."""
    independent = [
        item for item in bounded_decoder.get("instructions", [])
        if START <= address(item["address"]) <= END
    ]
    independent.sort(key=lambda item: address(item["address"]))
    expected: list[int] = []
    next_pc = START
    reached_end = False
    for item in independent:
        pc = address(item["address"])
        raw = bytes.fromhex(item.get("bytes", ""))
        if pc != next_pc or not raw or len(raw) % 2 or not item.get("supported", False):
            return False
        if rom[pc:pc + len(raw)] != raw:
            return False
        expected.append(pc)
        next_pc = pc + len(raw)
        if pc == END:
            reached_end = True
            break
    supplied_starts = [address(item["address"]) for item in supplied]
    return reached_end and supplied_starts == expected


def classify_unit(
    rom: bytes,
    runtime: dict[str, Any],
    decoded: dict[str, Any],
    bounded_decoder: dict[str, Any],
    explorer: dict[str, Any],
    m1121_text: str,
    previous_range_classification: str = "UNKNOWN",
) -> dict[str, Any]:
    if hashlib.sha256(rom).hexdigest() != runtime["canonical_rom_sha256"]:
        raise ValueError("runtime evidence ROM hash does not match canonical ROM")
    if runtime.get("source") != "GPGX_MANUAL_REALTIME":
        raise ValueError("runtime evidence source is not manual realtime")
    if runtime.get("evidence_type") != "CODE_EXECUTED_AT_ADDRESS":
        raise ValueError("runtime evidence type is not address-level execution")
    if "RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD" not in m1121_text:
        raise ValueError("M11.21 structural report decision is missing")

    instructions = target_instructions(decoded)
    runtime_pcs = {address(item) for item in runtime["executed_addresses"]}
    checked = verify_rom_bytes(rom, instructions)
    observed = [item for item in checked if address(item["address"]) in runtime_pcs]
    independent_decode = independently_decode_target(rom, bounded_decoder, instructions)
    full_decode = (bool(checked) and independent_decode and
                   all(item["decode_status"] == "DECODED" for item in checked))
    full_runtime = bool(checked) and len(observed) == len(checked)

    decoder_pairs = {
        (address(item["source"]), address(item["target"]))
        for item in bounded_decoder.get("direct_control_flow", [])
    }
    explorer_edges = {edge_key(item) for item in explorer.get("edges", [])}
    edge_records = []
    for source, target, kind in TARGET_EDGES:
        pair_found = (source, target) in decoder_pairs
        explorer_found = (source, target, kind) in explorer_edges
        found = pair_found or explorer_found
        evidence = []
        if pair_found: evidence.append("EXACT_DECODER")
        if explorer_found: evidence.append("STATIC_PROVEN")
        edge_records.append({
            "source": fmt(source), "target": fmt(target), "edge_type": kind,
            "evidence": "+".join(evidence) if found else "MISSING",
            "observed_target": target in runtime_pcs,
        })
    edges_complete = all(item["evidence"] != "MISSING" for item in edge_records)
    provenance_complete = bool(explorer.get("bounded_control_pass"))
    upgrade = full_decode and full_runtime and edges_complete and provenance_complete

    previous = previous_range_classification
    return {
        "schema": "oasis.gpgx.bounded.classification.v1",
        "canonical_rom_sha256": hashlib.sha256(rom).hexdigest(),
        "unit": {
            "start": fmt(START),
            "end": fmt(END),
            "end_inclusive": True,
            "classification_before": previous,
            "classification_after": "CODE_STATIC_SUPPORTED" if upgrade else previous,
            "boundary_status": BOUNDARY,
            "routine_identity": None,
            "whole_routine_promotion": False,
            "instructions": checked,
            "observed_instruction_starts": len(observed),
            "decoded_instruction_starts": sum(item["decode_status"] == "DECODED" for item in checked),
            "address_level_execution_evidence": [
                {"address": item["address"], "evidence_type": "CODE_EXECUTED_AT_ADDRESS"}
                for item in observed
            ],
            "direct_edges": edge_records,
        },
        "provenance": {
            "runtime_evidence": {"path": "build/gpgx_runtime_execution_evidence.json"},
            "exact_decoder": {"path": "build/m11-20-global.json"},
            "bounded_decoder": {"path": "build/m11-21-window.json"},
            "structural_evidence": {"path": "build/m11-20-explore.json"},
            "m11_21_report": {"path": "docs/reports/RUNTIME_REGION_060BB6.md"},
        },
        "unrelated_range_changes": [],
        "decision": (
            "BOUNDED_REGION_060BB6_STATIC_SUPPORTED"
            if upgrade else "BOUNDED_REGION_060BB6_CLASSIFICATION_NEEDS_FIXUPS"
        ),
    }


def with_hashes(result: dict[str, Any], paths: dict[str, Path]) -> dict[str, Any]:
    for key, path in paths.items():
        result["provenance"][key]["sha256"] = sha256(path)
    return result


def write_deterministic(path: Path, result: dict[str, Any]) -> None:
    path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def self_test() -> bool:
    rom = bytearray(0x700000)
    raw = {
        0x060BB6: "4E71", 0x060BB8: "4E71", 0x060BBA: "4E71", 0x060BBC: "4E71",
        0x060BBE: "4E71", 0x060BC0: "4E71", 0x060BC2: "60CC",
        0x060BC4: "33FC000000A11100",
    }
    for pc, value in raw.items():
        rom[pc:pc + len(bytes.fromhex(value))] = bytes.fromhex(value)
    instructions = [{
        "address": fmt(pc), "raw_bytes": bytes.fromhex(value).hex().upper(),
        "decoded_instruction": "fixture", "instruction_length": len(bytes.fromhex(value)),
        "status": "DECODED", "classification": "UNKNOWN",
    } for pc, value in raw.items()]
    decoded = {"instructions": instructions}
    runtime = {
        "canonical_rom_sha256": hashlib.sha256(rom).hexdigest(),
        "source": "GPGX_MANUAL_REALTIME", "evidence_type": "CODE_EXECUTED_AT_ADDRESS",
        "executed_addresses": [fmt(pc) for pc in raw],
    }
    bounded_decoder = {"direct_control_flow": [
        {"source": fmt(source), "target": fmt(target)}
        for source, target, kind in TARGET_EDGES if kind != "FALLTHROUGH"
    ], "instructions": [
        {"address": fmt(pc), "bytes": value.lower(), "supported": True}
        for pc, value in raw.items()
    ]}
    explorer = {"bounded_control_pass": True, "edges": [
        {"source_pc": fmt(source), "target": fmt(target), "kind": kind}
        for source, target, kind in TARGET_EDGES
    ]}
    report = "RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD"
    good = classify_unit(bytes(rom), runtime, decoded, bounded_decoder, explorer, report)
    if good["decision"] != "BOUNDED_REGION_060BB6_STATIC_SUPPORTED": return False
    if good["unit"]["boundary_status"] != BOUNDARY: return False

    broken = json.loads(json.dumps(decoded))
    broken["instructions"][0]["status"] = "DECODE_UNSUPPORTED"
    if classify_unit(bytes(rom), runtime, broken, bounded_decoder, explorer, report)["decision"] != "BOUNDED_REGION_060BB6_CLASSIFICATION_NEEDS_FIXUPS": return False
    no_runtime = dict(runtime, executed_addresses=[])
    if classify_unit(bytes(rom), no_runtime, decoded, bounded_decoder, explorer, report, "UNKNOWN")["unit"]["classification_after"] != "UNKNOWN": return False
    first = json.dumps(good, sort_keys=True)
    if first != json.dumps(classify_unit(bytes(rom), runtime, decoded, bounded_decoder, explorer, report), sort_keys=True): return False
    return True


def main(argv: list[str]) -> int:
    if argv == ["--self-test"]:
        return 0 if self_test() else 1
    if len(argv) != 7:
        print("usage: gpgx_bounded_classification.py <rom> <runtime.json> <decoder.json> <bounded_decoder.json> <explore.json> <m11.21.md> <output.json>", file=sys.stderr)
        return 2
    rom_path, runtime_path, decoder_path, bounded_decoder_path, explore_path, report_path, output_path = map(Path, argv)
    try:
        result = classify_unit(
            rom_path.read_bytes(), load(runtime_path), load(decoder_path), load(bounded_decoder_path),
            load(explore_path), report_path.read_text(encoding="utf-8"),
        )
        result = with_hashes(result, {
            "runtime_evidence": runtime_path, "exact_decoder": decoder_path,
            "bounded_decoder": bounded_decoder_path,
            "structural_evidence": explore_path, "m11_21_report": report_path,
        })
        write_deterministic(output_path, result)
        print(result["decision"])
        return 0 if result["decision"] == "BOUNDED_REGION_060BB6_STATIC_SUPPORTED" else 1
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
