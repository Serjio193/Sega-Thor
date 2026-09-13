"""AUTO64 static provenance frontier and persistent investigation queue."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


BASELINE = "d9d080a3a0c152d5b131f246a1ab2a3f9a435178"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
TARGET_START = 0xAF00
TARGET_END = 0xAF23
AF20_ADDRESSES = [
    0x167DD8, 0x167DDE, 0x167DE6, 0x167DEC, 0x167DF2, 0x167DF8,
    0x167DFE, 0x167E04, 0x167E0A, 0x167E10, 0x167E18, 0x167E1E,
    0x167E24, 0x167E2A, 0x167E30, 0x167E36, 0x167E3C, 0x167E42,
]


def hex_address(value: int) -> str:
    return f"0x{value:06X}"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def direct_callers(rom: bytes, target_start: int, target_end: int) -> list[dict[str, Any]]:
    """Find only statically encoded BSR/JSR/JMP references; never call them callers."""
    hits: list[dict[str, Any]] = []
    for pc in range(0, len(rom) - 1, 2):
        word = (rom[pc] << 8) | rom[pc + 1]
        target = None
        kind = None
        width = 2
        if word & 0xFF00 == 0x6100:
            kind = "BSR"
            displacement = word & 0xFF
            if displacement == 0:
                if pc + 4 <= len(rom):
                    displacement = int.from_bytes(rom[pc + 2:pc + 4], "big", signed=True)
                    width = 4
            else:
                displacement = displacement - 0x100 if displacement & 0x80 else displacement
            target = pc + width + displacement
        elif word == 0x4EB9 and pc + 6 <= len(rom):
            kind = "JSR_ABS_L"
            target = int.from_bytes(rom[pc + 2:pc + 6], "big")
            width = 6
        elif word == 0x4EF9 and pc + 6 <= len(rom):
            kind = "JMP_ABS_L"
            target = int.from_bytes(rom[pc + 2:pc + 6], "big")
            width = 6
        elif word in (0x4EB8, 0x4EF8) and pc + 4 <= len(rom):
            kind = "JSR_ABS_W" if word == 0x4EB8 else "JMP_ABS_W"
            target = int.from_bytes(rom[pc + 2:pc + 4], "big", signed=True) & 0xFFFFFF
            width = 4
        if target is not None and target_start <= target < target_end:
            hits.append({"pc": hex_address(pc), "kind": kind,
                         "target": hex_address(target), "width": width})
    return hits


def static_contract(slice_path: Path | None) -> dict[str, Any]:
    if not slice_path or not slice_path.exists():
        return {"available": False, "reason": "missing AF00 bounded slice"}
    report = json.loads(slice_path.read_text(encoding="utf-8"))
    instructions = report.get("instructions", [])
    wanted = {"0x0000af02": "MOVE.W (A6+0x18),D3",
              "0x0000af06": "MOVEA.L (A6+0x1A),A0",
              "0x0000af20": "MOVE.W (A0),D6",
              "0x0000af22": "MOVE.W D6,D4"}
    found = {item["address"].lower(): item for item in instructions
             if item.get("address", "").lower() in wanted}
    return {
        "available": True,
        "entry": hex_address(TARGET_START),
        "instructions": {key: {"contract": wanted[key], "bytes": found[key].get("bytes")}
                         for key in wanted if key in found},
        "a6_definition_inside_slice": False,
        "a6_definition_basis": "no decoded A6 destination in AF00 bounded slice; entry A6 remains inherited",
        "unresolved_register_memory": sum(len(item.get("unresolved_memory_references", []))
                                           for item in instructions),
    }


def node(node_id: str, parent: str | None, obligation: str, question: str,
         priority: int, evidence_classes: list[str]) -> dict[str, Any]:
    return {"id": node_id, "parent": parent, "obligation": obligation,
            "question": question, "known": [], "missing": [],
            "evidence_classes": evidence_classes, "status": "OPEN",
            "children": [], "resolution": None, "raw_witnesses": [],
            "derived_relations": [], "priority": priority,
            "fingerprint": f"{node_id}|{obligation}|{question}|{','.join(evidence_classes)}"}


def initial_graph(static: dict[str, Any], callers: list[dict[str, Any]],
                  coverage: dict[str, Any]) -> dict[str, Any]:
    frontiers = coverage.get("derived_frontiers", [])
    if not frontiers:
        raise ValueError("coverage imported no unresolved frontier; scheduler must not invent one")
    frontier = frontiers[0]
    root = node(frontier["id"], "INV-AUTO63-AF22", frontier["question"],
                frontier["question"], 100, ["STATIC", "RUNTIME"])
    root["created_from"] = frontier["created_from"]
    root["entity"] = frontier["entity"]
    root["why_open"] = frontier["why_open"]
    root["known"] = ["AF06 reads 0x1A(A6)", "runtime A6=0x00FF1CD8",
                      "runtime A0=0x00167DAC", "AF00 slice has no decoded A6 write"]
    root["missing"] = ["caller edge", "previous A6 definition PC", "A6 source class"]
    root["derived_relations"] = [{"type": "STATIC_DIRECT_CALLERS", "count": len(callers)}]
    if not callers:
        root["raw_witnesses"].append({"class": "STATIC", "result": "no direct BSR/JSR/JMP to AF00..AF22"})
    graph = {"schema": "oasis.m12.auto64.provenance-graph.v1", "baseline": BASELINE,
             "rom": {"sha256": ROM_SHA256, "size": ROM_SIZE},
             "coverage_schema": coverage.get("schema"),
             "coverage_frontiers": coverage.get("derived_frontiers", []),
             "nodes": {root["id"]: root}, "queue_history": []}
    children = [
        node("INV-AUTO64-A6-CALLER", root["id"], "A6 caller provenance",
             "Which dynamic caller reaches AF00 and what is the return/previous PC?", 95,
             ["STATIC", "RUNTIME"]),
        node("INV-AUTO64-D3", root["id"], "D3 definition and use",
             "Does D3=512 define a bounded structure or only a loop/control value?", 80,
             ["STATIC", "RUNTIME"]),
        node("INV-AUTO64-GAP-1", root["id"], "AF20 +8 gap at 0x167DDE..0x167DE6",
             "What independent static/runtime evidence explains the first +8 transition?", 65,
             ["STATIC", "RUNTIME"]),
        node("INV-AUTO64-GAP-2", root["id"], "AF20 +8 gap at 0x167E10..0x167E18",
             "What independent static/runtime evidence explains the second +8 transition?", 64,
             ["STATIC", "RUNTIME"]),
    ]
    for child in children:
        child["known"].append("AUTO63 observed 18 AF20 addresses and two +8 gaps")
        graph["nodes"][child["id"]] = child
        root["children"].append(child["id"])
    return graph


def load_or_create(path: Path, static: dict[str, Any], callers: list[dict[str, Any]],
                   coverage: dict[str, Any]) -> dict[str, Any]:
    if path.exists():
        graph = json.loads(path.read_text(encoding="utf-8"))
        graph.setdefault("queue_history", [])
        return graph
    return initial_graph(static, callers, coverage)


def next_open(graph: dict[str, Any]) -> dict[str, Any] | None:
    candidates = [item for item in graph["nodes"].values() if item["status"] == "OPEN"]
    return sorted(candidates, key=lambda item: (-item.get("priority", 0), item["id"]))[0] if candidates else None


def make_request(graph: dict[str, Any], static: dict[str, Any], callers: list[dict[str, Any]]) -> dict[str, Any]:
    selected = next_open(graph)
    request = {
        "schema": "oasis.m12.auto64.evidence-request.v1", "baseline": BASELINE,
        "rom_sha256": ROM_SHA256, "frontier": selected["id"] if selected else None,
        "mode": "STATIC_FIRST" if selected and "STATIC" in selected["evidence_classes"] else "NONE",
        "target": {"entry": hex_address(TARGET_START), "contract_end": hex_address(TARGET_END),
                   "watched_pcs": [hex_address(value) for value in (0xAF00, 0xAF02, 0xAF06, 0xAF20, 0xAF22)]},
        "static": {"direct_callers": callers, "contract": static,
                   "a6_write_policy": "fail-closed; no A6 origin from register equality"},
        "runtime_if_needed": {
            "scenario": "QuickSave1 exact state, settle 3 frames, 20 frames Right-first",
            "hooks": ["target entry", "previous PC ring", "A7 return long", "A6 version changes"],
            "provenance_rule": "same epoch/frame order; definition-before-use; no cross-run splice",
            "max_instruction_context": 256,
        },
        "queue": {"open_count": sum(item["status"] == "OPEN" for item in graph["nodes"].values()),
                  "selected_obligation": selected["obligation"] if selected else None,
                  "do_not_repeat": [item["fingerprint"] for item in graph["nodes"].values()
                                    if item["status"] in ("BLOCKED", "EXHAUSTED", "RESOLVED")]},
    }
    return request


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--slice", type=Path)
    parser.add_argument("--coverage", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--request", type=Path, required=True)
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if len(rom) != ROM_SIZE or sha256_file(args.rom) != ROM_SHA256:
        raise SystemExit("canonical ROM identity mismatch")
    callers = direct_callers(rom, TARGET_START, TARGET_END)
    static = static_contract(args.slice)
    coverage_path = getattr(args, "coverage", None)
    coverage = json.loads(coverage_path.read_text(encoding="utf-8")) if coverage_path else {}
    fresh = not args.graph.exists()
    graph = load_or_create(args.graph, static, callers, coverage)
    if fresh:
        root = graph["nodes"][coverage["derived_frontiers"][0]["id"]]
        root["status"] = "IN_PROGRESS"
        root["resolution"] = "static phase completed; precise upstream runtime question delegated to child"
        root["raw_witnesses"].append({"class": "STATIC", "direct_callers": callers,
                                      "contract": static})
        graph["queue_history"].append({"frontier": root["id"], "action": "STATIC_FIRST",
                                        "created_children": root["children"]})
    request = make_request(graph, static, callers)
    args.graph.parent.mkdir(parents=True, exist_ok=True)
    args.request.parent.mkdir(parents=True, exist_ok=True)
    args.graph.write_text(json.dumps(graph, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    args.request.write_text(json.dumps(request, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"frontier": request["frontier"], "direct_callers": len(callers),
                      "graph": str(args.graph), "request": str(args.request)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
