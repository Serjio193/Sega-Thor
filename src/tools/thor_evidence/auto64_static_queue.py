"""Consume the remaining AUTO64 queue using bounded static evidence only."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def next_open(graph: dict[str, Any]) -> dict[str, Any] | None:
    items = [item for item in graph["nodes"].values() if item["status"] == "OPEN"]
    return sorted(items, key=lambda item: (-item.get("priority", 0), item["id"]))[0] if items else None


def static_facts(report: dict[str, Any]) -> dict[str, Any]:
    instructions = report.get("instructions", [])
    by_address = {int(item["address"], 16): item for item in instructions}
    return {
        "af02": by_address.get(0xAF02, {}).get("bytes"),
        "af18": by_address.get(0xAF18, {}).get("bytes"),
        "af2a": by_address.get(0xAF2A, {}).get("bytes"),
        "af2e": by_address.get(0xAF2E, {}).get("bytes"),
        "a0_increment_candidates": [
            {"pc": f"0x{pc:04X}", "bytes": by_address.get(pc, {}).get("bytes"),
             "mnemonic": by_address.get(pc, {}).get("mnemonic"),
             "immediates": by_address.get(pc, {}).get("immediate_constants", [])}
            for pc in (0xAF6E, 0xAF70, 0xAFF8)
        ],
        "dataflow_proven": False,
    }


def consume(graph: dict[str, Any], report: dict[str, Any], report_path: Path) -> dict[str, Any]:
    facts = static_facts(report)
    while True:
        item = next_open(graph)
        if item is None:
            break
        if item["id"] == "INV-AUTO64-D3":
            reason = "AF02 loads D3 from inherited A6+0x18 and later compares it; bounded slice lacks semantic producer/dataflow proof"
        elif item["id"].startswith("INV-AUTO64-GAP-"):
            reason = "static slice has ADDQ candidates on A0 but no proven record domain/branch-to-gap relation; stride remains a hypothesis"
        else:
            reason = "no static closure for this queue item"
        item["status"] = "BOUNDED_UNRESOLVED"
        item["resolution"] = reason
        item["block_reason"] = "static evidence class exhausted; runtime premise would require a materially new question"
        item["raw_witnesses"].append({"class": "STATIC", "path": str(report_path), "facts": facts})
        graph["queue_history"].append({"frontier": item["id"], "action": "STATIC_BOUNDED",
                                        "required_evidence": "STATIC", "next": next_open(graph)["id"] if next_open(graph) else None})
    root = graph["nodes"].get("INV-AUTO64-A6")
    if root and not next_open(graph) and root["status"] == "IN_PROGRESS":
        root["status"] = "BLOCKED"
        root["resolution"] = "all high-value current frontiers reached justified fixed points"
        root["block_reason"] = "upstream A6 source and D3/gap semantics remain unresolved; no terminal root claimed"
    return graph


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--static", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    graph = json.loads(args.graph.read_text(encoding="utf-8"))
    report = json.loads(args.static.read_text(encoding="utf-8"))
    graph = consume(graph, report, args.static)
    args.graph.write_text(json.dumps(graph, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    result = {"schema": "oasis.m12.auto64.static-queue.v1",
              "next_frontier": next_open(graph)["id"] if next_open(graph) else None,
              "queue_history": graph["queue_history"]}
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"next_frontier": result["next_frontier"],
                      "consumed": [entry["frontier"] for entry in graph["queue_history"]]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
