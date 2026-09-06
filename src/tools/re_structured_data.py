"""Classify a small set of ROM data structures with exact, conservative proofs."""
import argparse
import hashlib
import json
from pathlib import Path


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
VECTOR = (0x000000, 0x000100)
HEADER = (0x000100, 0x000200)
TABLES = (
    {"id": "terrain_state", "start": 0x96E8, "end": 0x96F8, "width": 1,
     "count": 16, "consumer": "0x9D00/0x938E terrain-state lookup",
     "references": ["0x96E8", "0x9D00", "0x938E"],
     "expected": [0xFF, 0, 2, 1, 4, 0xFF, 3, 0xFF, 6, 7, 0xFF, 0xFF, 5, 0xFF, 0xFF, 0xFF]},
    {"id": "terrain_behavior", "start": 0x96F8, "end": 0x9708, "width": 1,
     "count": 16, "consumer": "0x9AD6 height/behavior lookup",
     "references": ["0x96F8", "0x9AD6"],
     "expected": [0xFF, 0, 2, 1, 4, 4, 3, 3, 6, 6, 6, 6, 5, 5, 5, 5]},
    {"id": "screen_group_pointers", "start": 0xC92C, "end": 0xC980, "width": 4,
     "count": 21, "consumer": "src/game/world/screen_descriptor.cpp",
     "references": ["0xC92C", "0xC8F0"], "parser": "big-endian ROM pointers"},
    {"id": "compressed_resource_pointers", "start": 0x5CE96, "end": 0x5D046,
     "width": 4, "count": 108, "consumer": "0xD3B2 indexed reader and 0x3820",
     "references": ["0x5CE96", "0xD3B2", "0x3820"],
     "parser": "D0*4 indexed big-endian pointer reader",
     "unresolved_fields": ["entry 0 is null; compressed payload size boundaries remain unresolved"]},
)
DESCRIPTORS = (
    (0x2CF82, "screen_id 0x0009"), (0x2D3E8, "screen_id 0x000C"),
    (0x32144, "screen_id 0x0704"), (0x3285C, "screen_id 0x0705"),
)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def parse_words(data, start, end, width):
    """Parse every fixed-width element, rejecting a partial final element."""
    size = end - start
    if size <= 0 or size % width:
        raise ValueError("range is not divisible by element width")
    return [int.from_bytes(data[offset:offset + width], "big")
            for offset in range(start, end, width)]


def overlap(left, right):
    return left[0] < right[1] and right[0] < left[1]


def code_conflicts(start, end, code_ranges):
    return [{"start": f"0x{s:06X}", "end": f"0x{e:06X}", "classification": level}
            for s, e, level in code_ranges if overlap((start, end), (s, e))]


def _common(candidate, rom, code_ranges):
    start, end = candidate["start"], candidate["end"]
    if end > len(rom) or start < 0:
        raise ValueError("range outside ROM")
    conflicts = code_conflicts(start, end, code_ranges)
    result = {"id": candidate["id"], "start": f"0x{start:06X}", "end": f"0x{end:06X}",
            "size": end - start, "source_artifact": "canonical_local_rom",
            "rom_sha256": sha256(rom), "byte_sha256": sha256(rom[start:end]),
            "references": candidate["references"], "consumer": candidate["consumer"],
            "element_width": candidate["width"], "record_count": candidate["count"],
            "termination_rule": f"fixed end 0x{end:06X}; count {candidate['count']}",
            "classification": "DATA_STRUCTURE_SUPPORTED", "confidence": "CONFIRMED",
            "conflicts": conflicts, "status": "CONFLICT" if conflicts else "ACCEPTED",
            "unresolved_fields": list(candidate.get("unresolved_fields", []))}
    if not candidate.get("references") or not candidate.get("consumer"):
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("missing consumer or reference evidence")
    return result


def classify_table(candidate, rom, code_ranges):
    result = _common(candidate, rom, code_ranges)
    values = parse_words(rom, candidate["start"], candidate["end"], candidate["width"])
    result["parsed_elements"] = values
    result["parser"] = candidate.get("parser", "fixed-width big-endian values")
    expected = candidate.get("expected")
    if expected is not None and values != expected:
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("byte values differ from documented table")
    if candidate["width"] == 4 and any(value >= len(rom) for value in values):
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("pointer target outside ROM")
    return result


def classify_vectors(rom, code_ranges):
    candidate = {"id": "system_vectors", "start": VECTOR[0], "end": VECTOR[1], "width": 4,
                 "count": 64, "consumer": "68000 reset/exception vector fetch",
                 "references": ["hardware reset", "vector table"],
                 "unresolved_fields": ["initial stack pointer is a RAM address"]}
    result = _common(candidate, rom, code_ranges)
    values = parse_words(rom, *VECTOR, 4)
    result["parsed_elements"] = values
    result["parser"] = "64 big-endian longwords; reset PC is element 1"
    valid_targets = all(value < len(rom) for value in values[1:])
    if not valid_targets or not (VECTOR[1] <= values[1] < len(rom)):
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("reset/exception target outside ROM")
    return result


def classify_header(rom, code_ranges):
    candidate = {"id": "rom_header", "start": HEADER[0], "end": HEADER[1], "width": 1,
                 "count": HEADER[1] - HEADER[0], "consumer": "ROM identity/header parser",
                 "references": ["ROM identity", "0x0100 header"],
                 "unresolved_fields": ["field-level semantics are intentionally not expanded"]}
    result = _common(candidate, rom, code_ranges)
    result["classification"] = "DATA_REGION_SUPPORTED"
    result["confidence"] = "BOUNDED"
    result["parsed_elements"] = {"console": rom[0x100:0x110].decode("ascii", "replace").rstrip()}
    result["parser"] = "bounded fixed 0x100-byte Genesis header region"
    if not rom[0x100:0x104] == b"SEGA":
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("missing SEGA header signature")
    return result


def classify_descriptor(start, label, rom, code_ranges):
    end = start + 26
    candidate = {"id": f"screen_descriptor_{start:06X}", "start": start, "end": end,
                 "width": 26, "count": 1, "consumer": "0xC8F0 and screen_descriptor parser",
                 "references": [label, "0xC8F0", "load_screen_descriptor"],
                 "unresolved_fields": ["descriptor payload names remain raw"]}
    result = _common(candidate, rom, code_ranges)
    raw = rom[start:end]
    pointer = int.from_bytes(raw[4:8], "big")
    result["parser"] = "26-byte descriptor: long pointer, 4 IDs, 4 signed bytes, 5 words"
    result["parsed_elements"] = {"pointer": pointer, "resource_ids": list(raw[8:12]),
                                  "signed_parameters": [x - 256 if x > 127 else x for x in raw[12:16]],
                                  "trailing_words": parse_words(raw, 16, 26, 2)}
    if pointer >= len(rom):
        result["status"] = "REJECTED"
        result["classification"] = "UNKNOWN"
        result["unresolved_fields"].append("primary stream pointer outside ROM")
    return result


def classify(rom, code_ranges):
    records = [classify_vectors(rom, code_ranges), classify_header(rom, code_ranges)]
    records.extend(classify_table(item, rom, code_ranges) for item in TABLES)
    records.extend(classify_descriptor(start, label, rom, code_ranges) for start, label in DESCRIPTORS)
    for record in records:
        if record["conflicts"]:
            record["status"] = "CONFLICT"
            record["classification"] = "CONFLICT"
    return records


def metrics(records, rom_size, code_metrics):
    accepted = [x for x in records if x["status"] == "ACCEPTED"]
    structures = [x for x in accepted if x["classification"] == "DATA_STRUCTURE_SUPPORTED"]
    regions = [x for x in accepted if x["classification"] == "DATA_REGION_SUPPORTED"]
    data_bytes = sum(x["size"] for x in structures + regions)
    asm = sum(code_metrics.values())
    return {"TOTAL_ROM_BYTES": rom_size, "ASM_ROUNDTRIP_BYTES": asm,
            "CODE_STATIC_SUPPORTED_BYTES": code_metrics.get("CODE_STATIC_SUPPORTED_BYTES", 0),
            "CODE_EXECUTED_BYTES": code_metrics.get("CODE_EXECUTED_BYTES", 0),
            "DATA_REGION_SUPPORTED_BYTES": sum(x["size"] for x in regions),
            "DATA_STRUCTURE_SUPPORTED_BYTES": sum(x["size"] for x in structures),
            "UNKNOWN_BYTES": rom_size - asm - data_bytes,
            "CONFLICT_BYTES": sum(x["size"] for x in records if x["status"] == "CONFLICT"),
            "data_ranges_attempted": len(records), "accepted": len(accepted),
            "rejected": sum(x["status"] == "REJECTED" for x in records),
            "conflicts": sum(x["status"] == "CONFLICT" for x in records)}


def load_code_metrics(path):
    levels = json.loads(path.read_text())["levels"]
    return {"ASM_ROUNDTRIP_BYTES": levels["ASM_ROUNDTRIP_EXACT"]["bytes"],
            "CODE_STATIC_SUPPORTED_BYTES": levels["CODE_STATIC_SUPPORTED"]["bytes"],
            "CODE_EXECUTED_BYTES": levels["CODE_EXECUTED"]["bytes"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=Path("local-roms/Beyond Oasis (USA).md"))
    parser.add_argument("--audit-report", type=Path, default=Path("build/m11-15/audit4/audit_report.json"))
    parser.add_argument("--output", type=Path, default=Path("build/m11-17/structured_data"))
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if len(rom) != ROM_SIZE or sha256(rom) != ROM_SHA256:
        raise SystemExit("canonical ROM identity mismatch")
    audit = json.loads(args.audit_report.read_text())
    code_ranges = [(x["start"], x["end"], x["new_classification"]) for x in audit["records"]]
    records = classify(rom, code_ranges)
    code_metrics = load_code_metrics(args.audit_report)
    report = {"schema": "oasis.m68k.structured-data-classification.v1",
              "rom_sha256": ROM_SHA256, "rom_size": len(rom), "baseline": "M11.15",
              "ranges": records, "metrics": metrics(records, len(rom), code_metrics),
              "promotion_safety": {"data_overlap_veto": True,
                                   "veto_classes": ["DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED"],
                                   "unknown_data_does_not_veto": True},
              "decision": "STRUCTURED_DATA_HIGH_VALUE" if sum(x["status"] == "ACCEPTED" and
                  x["classification"] == "DATA_STRUCTURE_SUPPORTED" for x in records) >= 3
                  else "STRUCTURED_DATA_NEEDS_FIXUPS",
              "next_recommendation": "A — return to the native vertical slice"}
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "classification_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
