#!/usr/bin/env python3
"""Rank manual-realtime GPGX unknown PCs for bounded static investigation."""

from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

ANCHORS = (0x3820, 0x62CC, 0x9BF2, 0xA8DA, 0xD3B2, 0x6121A)
TRUSTED = {"ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED", "CODE_VERIFIED"}


def number(value: object) -> int:
    return int(value, 16) if isinstance(value, str) and value.lower().startswith("0x") else int(value)


def overlap(start: int, end: int, left: int, right: int) -> bool:
    return start < right and end > left


def address_list(value: object) -> list[int]:
    return [number(item) for item in value] if isinstance(value, list) else []


def load(path: str) -> dict:
    return json.loads(Path(path).read_text(encoding="utf-8"))


def parse_ghidra_range(value: str) -> tuple[int, int] | None:
    match = re.fullmatch(r"(0x[0-9A-Fa-f]+)\.\.(0x[0-9A-Fa-f]+)", value or "")
    return (number(match.group(1)), number(match.group(2))) if match else None


def validate_inputs(evidence: dict, rom: str, decoded: dict) -> None:
    actual = hashlib.sha256(Path(rom).read_bytes()).hexdigest()
    if actual != evidence["canonical_rom_sha256"]:
        raise ValueError("canonical ROM SHA-256 does not match M11.19 evidence")
    addresses = {number(item) for item in evidence["executed_addresses"]}
    decoded_addresses = {number(item["address"]) for item in decoded["instructions"]}
    if addresses != decoded_addresses:
        raise ValueError("exact-decoder report address set differs from M11.19 artifact")
    if any(address & 1 for address in addresses):
        raise ValueError("odd executed PC in evidence")


def make_regions(decoded: dict) -> list[dict]:
    unknown = sorted(
        (item for item in decoded["instructions"] if item["classification"] in {"UNKNOWN", "RUNTIME_EXECUTED_UNKNOWN"}),
        key=lambda item: number(item["address"]),
    )
    regions: list[dict] = []
    for item in unknown:
        address = number(item["address"])
        if not regions or address != regions[-1]["last"] + 2:
            regions.append({"start": address, "last": address, "instructions": [item]})
        else:
            regions[-1]["last"] = address
            regions[-1]["instructions"].append(item)
    for region in regions:
        region["end"] = region.pop("last")
        region["byte_span"] = region["end"] - region["start"] + 2
        region["observed"] = len(region["instructions"])
        region["decoded"] = sum(item["status"] == "DECODED" for item in region["instructions"])
        region["unsupported"] = region["observed"] - region["decoded"]
    return regions


def static_ranges(audit: dict) -> list[dict]:
    result = []
    for record in audit.get("records", []):
        result.append({
            "start": number(record["start"]), "end": number(record["end"]),
            "classification": record.get("new_classification", record.get("current_classification", "UNKNOWN")),
            "incoming": record.get("incoming_xrefs", []),
            "concerns": record.get("unresolved_concerns", []),
        })
    return result


def candidate_ranges(candidate_map: dict) -> list[dict]:
    result = []
    for item in candidate_map.get("candidates", []):
        span = item.get("ghidra_range") or {}
        if not span.get("start") or not span.get("end"):
            continue
        result.append({
            "start": number(span["start"]), "end": number(span["end"]),
            "entry": item.get("entry", ""), "classification": item.get("classification", "UNKNOWN"),
            "incoming": address_list(item.get("ghidra_called_by")) + address_list(item.get("known_direct_call_sites")),
            "outgoing": address_list(item.get("ghidra_calls")),
            "score": item.get("ranking_score", 0),
            "conflict": bool(item.get("code_data_conflict") or item.get("boundary_conflict")),
        })
    return result


def ghidra_ranges(ghidra: dict) -> list[dict]:
    result = []
    for item in ghidra.get("functions", []):
        span = parse_ghidra_range(item.get("range", ""))
        if span:
            result.append({"start": span[0], "end": span[1], "entry": item.get("entry", ""),
                           "incoming": address_list(item.get("called_by")), "outgoing": address_list(item.get("calls"))})
    return result


def static_details(region: dict, audits: list[dict], candidates: list[dict], ghidra: list[dict], explorer: dict) -> dict:
    start, end = region["start"], region["end"] + 2
    audit_hits = [item for item in audits if overlap(start, end, item["start"], item["end"])]
    candidate_hits = [item for item in candidates if overlap(start, end, item["start"], item["end"])]
    ghidra_hits = [item for item in ghidra if overlap(start, end, item["start"], item["end"])]
    explorer_hits = [item for item in explorer.get("address_map", [])
                     if overlap(start, end, number(item["start"]), number(item["end"]))]
    incoming = set()
    outgoing = set()
    for item in candidate_hits + ghidra_hits:
        incoming.update(item["incoming"])
        outgoing.update(item["outgoing"])
    for edge in explorer.get("edges", []):
        source, target = number(edge["source_pc"]), number(edge["target"])
        if start <= target < end:
            incoming.add(source)
        if start <= source < end:
            outgoing.add(target)
    trusted_incoming = [address for address in incoming if any(item["start"] <= address < item["end"] and item["classification"] in TRUSTED for item in audits)]
    strong = [item for item in audit_hits if item["classification"] in TRUSTED]
    strong += [item for item in candidate_hits if item["classification"] in {"CONFIRMED", "STATIC_SUPPORTED"}]
    conflict = any(item["classification"].startswith("DATA_") for item in audit_hits) or any(item["conflict"] for item in candidate_hits)
    nearest = None
    nearest_distance = None
    for item in audits:
        if item["classification"] not in TRUSTED:
            continue
        distance = 0 if overlap(start, end, item["start"], item["end"]) else min(abs(start - item["end"]), abs(item["start"] - end))
        if nearest_distance is None or distance < nearest_distance:
            nearest_distance, nearest = distance, item
    return {"audit": audit_hits, "candidates": candidate_hits, "ghidra": ghidra_hits,
            "explorer": explorer_hits, "incoming": sorted(incoming), "outgoing": sorted(outgoing),
            "trusted_incoming": sorted(set(trusted_incoming)), "strong": strong, "conflict": conflict,
            "nearest": nearest, "nearest_distance": nearest_distance}


def score(region: dict, details: dict) -> tuple[int, list[str]]:
    points = 0
    reasons = []
    def add(value: int, reason: str) -> None:
        nonlocal points
        points += value
        if value:
            reasons.append(f"{value:+d} {reason}")
    add(min(region["observed"] * 2, 40), f"{region['observed']} observed PCs")
    add(min(region["decoded"], 25), f"{region['decoded']} decoded PCs")
    add(min(len(details["trusted_incoming"]) * 10, 20), "trusted incoming xref")
    add(min(len(details["outgoing"]) * 5, 10), "known outgoing edge")
    add(12 if details["strong"] else 0, "static corroboration")
    add(8 if details["candidates"] else 0, "candidate-map overlap")
    add(10 if details["ghidra"] else 0, "Ghidra overlap")
    add(8 if details["explorer"] else 0, "explorer overlap")
    distance = details["nearest_distance"]
    add(8 if distance is not None and distance <= 0x1000 else 4 if distance is not None and distance <= 0x10000 else 0, "near trusted range")
    add(-min(region["unsupported"] * 4, 16), "decoder unsupported")
    add(-8 if region["observed"] <= 2 else 0, "isolated tiny fragment")
    add(-10 if details["conflict"] else 0, "data/conflict overlap")
    return points, reasons


def mnemonic(item: dict) -> str:
    return item.get("decoded_instruction", "").strip().split(" ", 1)[0].lower()


def direct_target(item: dict) -> int | None:
    match = re.search(r"loc_([0-9A-Fa-f]+)", item.get("decoded_instruction", ""))
    return int(match.group(1), 16) if match else None


def bounded_slice(region: dict) -> dict:
    start, end = number(region["start"]), number(region["end"]) + 2
    local = {number(item["address"]): item for item in region["instructions"]}
    queue, visited, instructions, edges, stops = [start], set(), [], [], []
    while queue and len(instructions) < 64:
        address = queue.pop(0)
        if address in visited:
            continue
        visited.add(address)
        item = local.get(address)
        if item is None:
            stops.append({"address": f"0x{address:06X}", "reason": "observed-gap-or-region-boundary"})
            continue
        instructions.append({"address": item["address"], "decoded": item["decoded_instruction"], "status": item["status"]})
        if item["status"] != "DECODED":
            stops.append({"address": item["address"], "reason": "DECODE_UNSUPPORTED"})
            continue
        op, target = mnemonic(item), direct_target(item)
        if op in {"rts", "rte", "rtr"}:
            stops.append({"address": item["address"], "reason": op.upper()})
            continue
        is_call = op.startswith("jsr") or op.startswith("bsr")
        is_jump = op.startswith("jmp") or op == "bra" or op.startswith("bra.")
        is_branch = op.startswith("b") or op.startswith("db")
        if (is_call or is_jump or is_branch) and target is None:
            stops.append({"address": item["address"], "reason": "indirect-or-unresolved-control-flow"})
        elif target is not None and (is_call or is_jump or is_branch):
            kind = "call" if is_call else "jump" if is_jump else "branch"
            edges.append({"source": item["address"], "target": f"0x{target:06X}", "kind": kind})
            if start <= target < end:
                queue.append(target)
            else:
                stops.append({"address": item["address"], "reason": "direct-control-flow-leaves-region"})
        fallthrough = address + number(item.get("instruction_length", 2))
        if not is_jump and fallthrough < end and fallthrough in local:
            queue.append(fallthrough)
        elif not is_call and not is_branch and fallthrough >= end:
            stops.append({"address": item["address"], "reason": "region-end"})
    return {"start": f"0x{start:06X}", "end": f"0x{number(region['end']):06X}",
            "instructions": instructions, "edges": edges, "stops": stops}


def anchor_checks(regions: list[dict], decoded_by_address: dict[int, dict]) -> list[dict]:
    checks = []
    for anchor in ANCHORS:
        exact = decoded_by_address.get(anchor)
        nearest = min(regions, key=lambda item: 0 if number(item["start"]) <= anchor <= number(item["end"]) else min(abs(anchor - number(item["start"])), abs(anchor - number(item["end"])))) if regions else None
        distance = 0 if nearest and number(nearest["start"]) <= anchor <= number(nearest["end"]) else (min(abs(anchor - number(nearest["start"])), abs(anchor - number(nearest["end"]))) if nearest else None)
        checks.append({"address": f"0x{anchor:06X}", "exact_classification": exact["classification"] if exact else "NOT_OBSERVED",
                       "exact_status": exact["status"] if exact else "NOT_OBSERVED", "in_unknown_region": bool(nearest and distance == 0),
                       "nearest_unknown_region": f"{nearest['start']}-{nearest['end']}" if nearest else None, "distance": distance})
    return checks


def enrich(regions: list[dict], audits: list[dict], candidates: list[dict], ghidra: list[dict], explorer: dict) -> None:
    for region in regions:
        details = static_details(region, audits, candidates, ghidra, explorer)
        points, reasons = score(region, details)
        region.update({"start": f"0x{region['start']:06X}", "end": f"0x{region['end']:06X}",
                       "observed_pcs": [item["address"] for item in region["instructions"]],
                       "decoded_percent": round(100.0 * region["decoded"] / region["observed"], 3),
                       "static_support": "RUNTIME_EXECUTED_STATIC_CORROBORATED" if details["strong"] else "NONE",
                       "incoming_xrefs": [f"0x{item:06X}" for item in details["incoming"]],
                       "outgoing_edges": [f"0x{item:06X}" for item in details["outgoing"]],
                       "trusted_incoming_xrefs": [f"0x{item:06X}" for item in details["trusted_incoming"]],
                       "candidate_overlap": [item["entry"] for item in details["candidates"]],
                       "ghidra_overlap": [item["entry"] for item in details["ghidra"]],
                       "explorer_overlap": [item["start"] for item in details["explorer"]],
                       "nearest_trusted_code": (f"0x{details['nearest']['start']:06X}-0x{details['nearest']['end']:06X}" if details["nearest"] else None),
                       "nearest_distance": details["nearest_distance"], "anchors": [f"0x{a:06X}" for a in ANCHORS if number(region["start"]) <= a <= number(region["end"])],
                       "terminators": sorted({mnemonic(item) for item in region["instructions"] if mnemonic(item) in {"rts", "rte", "rtr", "jsr", "bsr", "jmp", "bra"} or mnemonic(item).startswith("b") or mnemonic(item).startswith("db")}),
                       "score": points, "reason": "; ".join(reasons), "_details": details})


def render(result: dict) -> str:
    lines = ["# Runtime-Executed Unknown Priority", "", f"Decision: `{result['decision']}`", "",
             f"Canonical ROM SHA-256: `{result['canonical_rom_sha256']}`", f"Total unknown PCs: **{result['total_unknown_pcs']}**",
             f"Total `RUNTIME_EXECUTED_REGION`s: **{result['total_regions']}**", "",
             "## Ranking method", "", result["ranking_method"], "",
             "## Top 20", "", "| Rank | Region | PCs | Decoded | Static support | Incoming | Outgoing | Nearest trusted | Score | Reason |", "|---:|---|---:|---:|---|---:|---:|---|---:|---|"]
    for rank, item in enumerate(result["top20"], 1):
        lines.append(f"| {rank} | {item['start']}-{item['end']} | {item['observed']} | {item['decoded_percent']:.3f}% | {item['static_support']} | {len(item['incoming_xrefs'])} | {len(item['outgoing_edges'])} | {item['nearest_trusted_code'] or '-'} | {item['score']} | {item['reason']} |")
    lines += ["", "## Runtime + static corroboration", "", f"Regions: **{result['static_corroborated_count']}**; showing the top 20 by the same score.", ""]
    for item in result["static_corroborated"][:20]:
        lines.append(f"- `{item['start']}-{item['end']}` score={item['score']} support={item['candidate_overlap'] or item['ghidra_overlap']}")
    lines += ["", "## Top 5 bounded static slices", "", "Slices follow only observed local direct control flow; they do not define functions or promote trust.", ""]
    for rank, item in enumerate(result["top5_slices"], 1):
        lines += [f"### {rank}. {item['start']}-{item['end']}", "", f"Instructions: {len(item['instructions'])}; edges: {len(item['edges'])}", ""]
        for edge in item["edges"]: lines.append(f"- {edge['kind']}: `{edge['source']}` -> `{edge['target']}`")
        for stop in item["stops"]: lines.append(f"- stop `{stop['address']}`: {stop['reason']}")
    lines += ["", "## Anchor checks", "", "| Anchor | Exact classification | In unknown region | Nearest unknown region | Distance |", "|---|---|---|---|---:|"]
    for item in result["anchor_checks"]:
        lines.append(f"| {item['address']} | {item['exact_classification']} | {'yes' if item['in_unknown_region'] else 'no'} | {item['nearest_unknown_region'] or '-'} | {item['distance'] if item['distance'] is not None else '-'} |")
    lines += ["", "## Systemic pattern", "", result["systemic_pattern"], "", "## Recommended single next target", "", f"`{result['recommended_target']}`"]
    return "\n".join(lines) + "\n"


def build(evidence_path: str, rom: str, decoded_path: str, audit_path: str, candidate_path: str, ghidra_path: str, explore_path: str) -> dict:
    evidence, decoded = load(evidence_path), load(decoded_path)
    validate_inputs(evidence, rom, decoded)
    regions = make_regions(decoded)
    audits, candidates = static_ranges(load(audit_path)), candidate_ranges(load(candidate_path))
    ghidra, explorer = ghidra_ranges(load(ghidra_path)), load(explore_path)
    enrich(regions, audits, candidates, ghidra, explorer)
    regions.sort(key=lambda item: (-item["score"], -item["observed"], -item["decoded"], number(item["start"])))
    decoded_by_address = {number(item["address"]): item for item in decoded["instructions"]}
    for item in regions:
        item["instructions"] = item.pop("instructions")
    static = [item for item in regions if item["static_support"] == "RUNTIME_EXECUTED_STATIC_CORROBORATED"]
    top20 = []
    for rank, item in enumerate(regions[:20], 1):
        clean = {key: value for key, value in item.items() if key != "_details" and key != "instructions"}
        clean["rank"] = rank
        top20.append(clean)
    slices = [bounded_slice(item) for item in regions[:5]]
    unknown_in_ghidra = sum(1 for item in regions if item["ghidra_overlap"])
    pattern = (f"{unknown_in_ghidra} of {len(regions)} regions overlap Ghidra ranges, while only "
               f"{len(static)} have corroborating trusted/static candidate evidence. The dominant gap is "
               "boundary/trust corroboration, not a proven decoder failure; no mass classifier repair is applied.")
    result = {"schema": "oasis.gpgx.runtime.unknown.priority.v1", "canonical_rom_sha256": evidence["canonical_rom_sha256"],
              "source": "GPGX_MANUAL_REALTIME", "total_unknown_pcs": sum(item["observed"] for item in regions),
              "total_regions": len(regions), "ranking_method": "score = min(2*observed,40) + min(decoded,25) + trusted incoming (10 each, cap 20) + outgoing edges (5 each, cap 10) + static corroboration 12 + candidate overlap 8 + Ghidra overlap 10 + explorer overlap 8 + nearest trusted range 8/4 - unsupported (4 each, cap 16) - tiny fragment 8 - conflict 10; repeated-hit data was unavailable, so it contributes 0.",
              "top20": top20, "static_corroborated_count": len(static),
              "static_corroborated": [{key: value for key, value in item.items() if key not in {"_details", "instructions"}} for item in static[:20]],
              "anchor_checks": anchor_checks(regions, decoded_by_address),
              "top5_slices": slices, "systemic_pattern": pattern,
              "recommended_target": f"{top20[0]['start']}-{top20[0]['end']} (score {top20[0]['score']})" if top20 else "none",
              "decision": "RUNTIME_UNKNOWN_PRIORITIZATION_HIGH_VALUE" if top20 and static else "RUNTIME_UNKNOWN_PRIORITIZATION_NEEDS_FIXUPS"}
    return result


def self_test() -> int:
    decoded = {"instructions": [{"address": "0x000100", "classification": "UNKNOWN", "status": "DECODED", "decoded_instruction": "rts", "instruction_length": 2},
                                 {"address": "0x000102", "classification": "UNKNOWN", "status": "DECODED", "decoded_instruction": "moveq #0,D0", "instruction_length": 2}]}
    assert len(make_regions(decoded)) == 1
    region = make_regions(decoded)[0]
    assert region["observed"] == 2 and region["decoded"] == 2 and region["byte_span"] == 4
    assert bounded_slice(region)["stops"][0]["reason"] == "RTS"
    return 0


def main(argv: list[str]) -> int:
    if argv == ["--self-test"]:
        return self_test()
    if len(argv) != 9:
        print("usage: gpgx_unknown_priority.py <evidence.json> <canonical.rom> <exact_decoder.json> <audit.json> <candidate-map.json> <ghidra.json> <explore.json> <output.json> <report.md>", file=sys.stderr)
        return 2
    try:
        result = build(*argv[:7])
        Path(argv[7]).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        Path(argv[8]).write_text(render(result), encoding="utf-8")
        print(f"ranked {result['total_unknown_pcs']} unknown PCs in {result['total_regions']} regions; decision={result['decision']}")
        return 0
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
