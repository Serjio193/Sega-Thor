"""Fail-closed, subsystem-neutral recursive ROM provenance closure.

The input is a normalized post-run corpus.  Decoders and subsystem stages may
contribute facts, but this module only understands instructions, values,
memory observations and calls.  No game address is encoded here.
"""

from __future__ import annotations

import hashlib
import json
from collections import defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


STATUSES = {"EXACT", "PARTIAL", "UNRESOLVED"}
EDGE_KINDS = {"DEFINES", "READS", "WRITES", "DERIVES", "CALLS", "RETURNS_TO",
              "BRANCHES_TO", "ADDRESSES", "INDEXES", "OBSERVED_NEXT_PC"}


@dataclass(frozen=True)
class Fact:
    pc: int
    op: str
    dst: str | None = None
    src: Any = None
    width: int = 4


@dataclass
class Value:
    value: int | None
    status: str
    chain: list[str] = field(default_factory=list)

    def extend(self, label: str, value: int | None = None) -> "Value":
        return Value(self.value if value is None else value, self.status,
                     [*self.chain, label])


def _hex(value: int) -> str:
    return f"0x{value:08X}"


def _node(kind: str, identity: str) -> str:
    return f"{kind}:{identity}"


class RecursiveClosure:
    """Analyze one deterministic normalized corpus until a fixpoint."""

    def __init__(self, corpus: dict[str, Any], rom: bytes | None = None) -> None:
        self.corpus = corpus
        self.rom = rom
        self.instructions = [Fact(int(row["pc"]), str(row["op"]).upper(),
                                  row.get("dst"), row.get("src"), int(row.get("width", 4)))
                             for row in corpus.get("instructions", []) if row.get("op")]
        self.by_pc = {item.pc: item for item in self.instructions}
        self.memory = {(int(row["address"]), int(row.get("width", 2))): int(row["value"])
                       for row in corpus.get("memory", [])
                       if row.get("operation", row.get("kind")) != "write"}
        self.edges: set[tuple[str, str, str]] = set()
        self.registers: dict[str, Value] = {}
        self.promotions: list[dict[str, Any]] = []
        self.map_operations: list[dict[str, Any]] = []
        self.unresolved: list[dict[str, Any]] = []
        self.gaps: defaultdict[str, int] = defaultdict(int)
        self.flow_metrics = {"instruction_records": 0, "bus_records": 0,
                             "unique_execution_pcs": 0, "unique_observed_transitions": 0,
                             "rejected_instruction_identities": 0}

    def edge(self, kind: str, left: str, right: str) -> None:
        if kind not in EDGE_KINDS:
            raise ValueError(f"unknown edge kind: {kind}")
        self.edges.add((kind, left, right))

    def _mark_unresolved(self, item: Fact, status: str) -> None:
        key = (_hex(item.pc), item.op)
        self.unresolved = [row for row in self.unresolved
                           if (row.get("pc"), row.get("op")) != key]
        self.unresolved.append({"pc": key[0], "op": key[1], "status": status})

    def _clear_unresolved(self, item: Fact) -> None:
        key = (_hex(item.pc), item.op)
        self.unresolved = [row for row in self.unresolved
                           if (row.get("pc"), row.get("op")) != key]

    def _source_value(self, source: Any, pc: int) -> Value:
        if isinstance(source, int):
            return Value(source, "EXACT", [_node("constant", str(source))])
        if isinstance(source, str) and source.upper() in self.registers:
            return self.registers[source.upper()]
        if isinstance(source, dict) and source.get("kind") == "memory":
            base = self.registers.get(str(source.get("base", "")).upper())
            index = self.registers.get(str(source.get("index", "")).upper())
            displacement = int(source.get("displacement", 0))
            scale = int(source.get("scale", 1))
            if base is None or base.status != "EXACT" or base.value is None:
                return Value(None, "UNRESOLVED", [])
            if source.get("index") and (index is None or index.status != "EXACT" or index.value is None):
                return Value(None, "UNRESOLVED", [])
            address = base.value + displacement + ((index.value if index else 0) * scale)
            width = int(source.get("width", 2))
            value = self.memory.get((address, width))
            memory_node = _node("memory_read", f"{_hex(address)}:{width}")
            inst_node = _node("instruction", _hex(pc))
            self.edge("READS", inst_node, memory_node)
            self.edge("ADDRESSES", _node("register_value", str(source.get("base")).upper()), memory_node)
            if source.get("index"):
                self.edge("INDEXES", _node("register_value", str(source.get("index")).upper()), memory_node)
            if value is None:
                self.gaps["missing_memory_reads"] += 1
                return Value(None, "UNRESOLVED", [memory_node])
            return Value(value, "EXACT", [memory_node])
        return Value(None, "UNRESOLVED", [])

    def _execute(self, item: Fact) -> None:
        node = _node("instruction", _hex(item.pc))
        op = item.op
        if op in {"LEA", "MOVEA", "MOVE", "MOVEQ", "ADD", "ADDA", "SUB", "SUBA",
                  "AND", "OR", "EOR", "EXT", "SWAP", "LSL", "LSR", "ASL", "ASR"}:
            source = self._source_value(item.src, item.pc)
            if source.status != "EXACT" or source.value is None or not item.dst:
                if item.dst:
                    self.registers[item.dst.upper()] = Value(None, source.status, source.chain)
                self._mark_unresolved(item, source.status)
                return
            self._clear_unresolved(item)
            value = source.value
            amount = int(item.src.get("amount", 0)) if isinstance(item.src, dict) else 0
            if op in {"ADD", "ADDA"}: value += amount
            if op in {"SUB", "SUBA"}: value -= amount
            self.registers[item.dst.upper()] = source.extend(node, value)
            self.edge("DEFINES", node, _node("register_value", item.dst.upper()))
            if source.chain:
                self.edge("DERIVES", node, source.chain[-1])
            return
        if op in {"JSR", "JMP", "RTS", "BRA", "Bcc", "RETURN"}:
            target = item.src
            if isinstance(target, int):
                self._clear_unresolved(item)
                kind = "CALLS" if op == "JSR" else "BRANCHES_TO"
                self.edge(kind, node, _node("instruction", _hex(target)))
            else:
                self.gaps["indirect_targets"] += 1
                self._mark_unresolved(item, "UNRESOLVED")

    def _observe_flow(self) -> None:
        """Add only canonical-opcode-verified, actually observed PC transitions."""
        pcs: set[int] = set()
        transitions: set[tuple[int, int]] = set()
        for row in self.corpus.get("records", []):
            if row.get("kind") != "instruction":
                if row.get("kind") == "bus":
                    self.flow_metrics["bus_records"] += 1
                continue
            if self.corpus.get("schema", "").endswith(".v2") and \
                    row.get("cpu_id") != "M68K":
                continue
            self.flow_metrics["instruction_records"] += 1
            pc, opcode, next_pc = int(row["pc"]), int(row["opcode"]), int(row["address"])
            pcs.add(pc)
            if self.rom is None or pc < 0 or pc + 2 > len(self.rom) or \
                    opcode > 0xFFFF or int.from_bytes(self.rom[pc:pc + 2], "big") != opcode:
                self.flow_metrics["rejected_instruction_identities"] += 1
                self.gaps["missing_instruction_identity"] += 1
                continue
            transitions.add((pc, next_pc))
        for row in self.corpus.get("calls", []):
            if row.get("cpu_id") != "M68K" or row.get("target") is None:
                continue
            source, target = int(row["pc"]), int(row["target"])
            self.edge("CALLS", _node("instruction", _hex(source)),
                      _node("instruction", _hex(target)))
        for row in self.corpus.get("returns", []):
            if row.get("cpu_id") == "M68K" and row.get("return_pc") is not None:
                self.edge("RETURNS_TO", _node("instruction", _hex(int(row["pc"]))),
                          _node("instruction", _hex(int(row["return_pc"]))))
        for row in self.corpus.get("memory", []):
            if row.get("pc") is None:
                continue
            identity = _node("instruction", _hex(int(row["pc"])))
            address = _node("memory", f"{_hex(int(row['address']))}:{int(row.get('width', 0))}")
            self.edge("WRITES" if row.get("operation") == "write" else "READS",
                      identity, address)
        for row in self.corpus.get("rom_reads", []):
            address = row.get("rom_address")
            if address is not None and row.get("consumer_pc") is not None:
                self.edge("READS", _node("instruction", _hex(int(row["consumer_pc"]))),
                          _node("rom_read", f"{_hex(int(address))}:{int(row.get('width', 0))}"))
        for source, target in sorted(transitions):
            self.edge("OBSERVED_NEXT_PC", _node("observed_pc", _hex(source)),
                      _node("observed_pc", _hex(target)))
        self.flow_metrics["unique_execution_pcs"] = len(pcs)
        self.flow_metrics["unique_observed_transitions"] = len(transitions)
        rejected = self.flow_metrics["rejected_instruction_identities"]
        if rejected:
            self.unresolved.append({"evidence": "normalized_instruction_records",
                                    "count": rejected, "status": "UNRESOLVED",
                                    "reason": "CANONICAL_ROM_OPCODE_IDENTITY_NOT_VERIFIED"})

    def _calls(self) -> list[dict[str, Any]]:
        rows = [{"caller": _hex(int(item["pc"])),
                 "call_instruction": _hex(int(item["pc"])),
                 "callee": _hex(int(item["target"])),
                 "stream_sequence": item.get("stream_sequence"),
                 "instruction_sequence": item.get("instruction_sequence"),
                 "run_id": item.get("run_id"), "epoch": item.get("epoch"),
                 "frame": item.get("frame"),
                 "input_registers": [], "modified_registers": [],
                 "returned_registers": []}
                for item in self.corpus.get("calls", [])
                if item.get("target") is not None]
        for kind, left, right in sorted(self.edges):
            if kind != "CALLS": continue
            caller, callee = left.split(":", 1)[1], right.split(":", 1)[1]
            if not any(row["caller"] == caller and row["callee"] == callee for row in rows):
                rows.append({"caller": caller, "call_instruction": caller, "callee": callee,
                         "input_registers": sorted(self.registers), "modified_registers": [],
                         "returned_registers": []})
        return rows

    def _discover_tables(self) -> list[dict[str, Any]]:
        grouped: defaultdict[tuple[int, int], list[dict[str, Any]]] = defaultdict(list)
        for row in self.corpus.get("memory", []):
            if row.get("kind") != "rom": continue
            base = int(row.get("base", row["address"]))
            stride = int(row.get("stride", 0))
            grouped[(base, stride)].append(row)
        tables = []
        for (base, stride), rows in sorted(grouped.items()):
            if len(rows) < 2 or stride <= 0: continue
            fields = sorted({int(row["address"]) - base for row in rows})
            tables.append({"base": _hex(base), "range": [_hex(min(int(r["address"]) for r in rows)),
                         _hex(max(int(r["address"]) + int(r.get("width", 2)) for r in rows))],
                         "record_stride": stride, "record_count_observed": len(rows),
                         "field_offsets": fields, "consumer_pcs": sorted(row.get("pc") for row in rows),
                         "runtime_indices": sorted({row["index"] for row in rows if "index" in row}),
                         "truth": "DERIVED_EXACT" if all(row.get("exact", False) for row in rows) else "HYPOTHESIS"})
        return tables

    def _promote_exact_ranges(self) -> None:
        """Promote only caller-supplied byte-identical ranges."""
        for row in self.corpus.get("rom_ranges", []):
            if not row.get("exact") or row.get("source_owned", False):
                continue
            start, end = int(row["start"]), int(row["end"])
            canonical = bytes.fromhex(str(row.get("canonical_bytes", "")))
            source = bytes.fromhex(str(row.get("source_bytes", "")))
            if end <= start or len(canonical) != end - start or canonical != source:
                self.unresolved.append({"range": [_hex(start), _hex(end)],
                                        "status": "UNRESOLVED", "reason": "ROUNDTRIP_MISMATCH"})
                continue
            self.promotions.append({"start": _hex(start), "end": _hex(end),
                                    "bytes": end - start, "truth": "DERIVED_EXACT",
                                    "roundtrip": "BYTE_EXACT"})
            self.map_operations.append({"op": "PROMOTE_SOURCE_OWNED", "start": start,
                                        "end_exclusive": end, "class": "UNKNOWN",
                                        "truth": "DERIVED_EXACT", "exact_extent": True,
                                        "exact_class": True, "exact_bytes": True,
                                        "exact_emission": True, "byte_roundtrip": True,
                                        "proof": "caller_supplied_exact_rom_range"})

    def run(self) -> dict[str, Any]:
        before = int(self.corpus.get("source_owned_before", 0))
        facts_before = 0
        iteration = 0
        per_iteration = []
        flow_processed = False
        while True:
            iteration += 1
            facts_before = len(self.edges)
            cfg_before = sum(edge[0] in {"CALLS", "BRANCHES_TO", "OBSERVED_NEXT_PC"}
                             for edge in self.edges)
            if not flow_processed:
                self._observe_flow()
                flow_processed = True
            for item in self.instructions: self._execute(item)
            new_facts = len(self.edges) - facts_before
            cfg_after = sum(edge[0] in {"CALLS", "BRANCHES_TO", "OBSERVED_NEXT_PC"}
                            for edge in self.edges)
            per_iteration.append({"iteration": iteration, "new_facts": max(0, new_facts),
                                  "new_cfg_edges": cfg_after - cfg_before,
                                  "new_tables": 0, "new_indirect_targets": 0,
                                  "new_exact_rom_ranges": 0, "asm_bytes_promoted": 0,
                                  "data_bytes_promoted": 0, "source_owned_delta": 0})
            if new_facts <= 0: break
        tables = self._discover_tables()
        self._promote_exact_ranges()
        queue = deque(self.unresolved)
        queue_rows = []
        while queue:
            item = queue.popleft()
            queue_rows.append({**item, "priority": 1 + self.gaps.get("missing_memory_reads", 0)})
        graph = {"schema": "oasis.m12.generic-provenance-graph.v1",
                 "nodes": sorted({node for _, left, right in self.edges for node in (left, right)}),
                 "edges": [{"kind": k, "from": a, "to": b} for k, a, b in sorted(self.edges)],
                 "register_values": {key: {"value": val.value, "status": val.status, "chain": val.chain}
                                     for key, val in sorted(self.registers.items())}}
        promoted_bytes = sum(int(item["bytes"]) for item in self.promotions)
        if per_iteration:
            per_iteration[-1]["new_exact_rom_ranges"] = len(self.promotions)
            per_iteration[-1]["data_bytes_promoted"] = promoted_bytes
            per_iteration[-1]["source_owned_delta"] = promoted_bytes
        flow_records = self.corpus.get("records", [])
        instruction_count = self.flow_metrics["instruction_records"]
        bus_count = self.flow_metrics["bus_records"]
        ranking = []
        rejected = self.flow_metrics["rejected_instruction_identities"]
        if rejected:
            ranking.append({"blocker_class": "INSTRUCTION_IDENTITY_OR_CPU_DOMAIN_UNRESOLVED",
                            "blocked_ranges": None, "blocked_bytes": None,
                            "blocked_cfg_edges": None, "affected_references": rejected,
                            "required_evidence_or_capability":
                            "preserve CPU/domain identity and verify opcode bytes against that address space",
                            "expected_rom_gain": "UNMEASURABLE_FROM_PRESERVED_FACTS"})
        if flow_records and not self.instructions:
            ranking.append({"blocker_class": "MISSING_REGISTER_SNAPSHOT_OR_DECODED_OPERANDS",
                            "blocked_ranges": None, "blocked_bytes": None,
                            "blocked_cfg_edges": None, "affected_references": instruction_count,
                            "required_evidence_or_capability":
                            "register snapshots or verified instruction-operand normalization",
                            "expected_rom_gain": "UNMEASURABLE_FROM_PRESERVED_FACTS"})
        if bus_count and not self.corpus.get("memory"):
            ranking.append({"blocker_class": "MISSING_MEMORY_WIDTH_DOMAIN_AND_CAUSAL_IDENTITY",
                            "blocked_ranges": None, "blocked_bytes": None,
                            "blocked_cfg_edges": None, "affected_references": bus_count,
                            "required_evidence_or_capability":
                            "preserve bus width, domain, value and ordered instruction identity",
                            "expected_rom_gain": "UNMEASURABLE_FROM_PRESERVED_FACTS"})
        if not self.corpus.get("rom_ranges"):
            ranking.append({"blocker_class": "EXACT_ROM_ROUNDTRIP_CLOSURE_GAP",
                            "blocked_ranges": None, "blocked_bytes": None,
                            "blocked_cfg_edges": None, "affected_references": None,
                            "required_evidence_or_capability":
                            "exact candidate boundaries and byte-identical source/canonical round trip",
                            "expected_rom_gain": "UNMEASURABLE_FROM_PRESERVED_FACTS"})
        ranking.sort(key=lambda row: (row["affected_references"] is None,
                                      0 if row["affected_references"] is None
                                      else -int(row["affected_references"]),
                                      row["blocker_class"]))
        for rank, row in enumerate(ranking, 1):
            row["rank"] = rank
        fact_counts = {kind: sum(edge[0] == kind for edge in self.edges)
                       for kind in EDGE_KINDS}
        provenance_metrics = {
            "GENERIC_FACTS": len(self.edges),
            "CFG_EDGES": sum(fact_counts[kind] for kind in
                             ("CALLS", "BRANCHES_TO", "OBSERVED_NEXT_PC")),
            "REGISTER_DEF_EDGES": fact_counts["DEFINES"],
            "MEMORY_CAUSAL_EDGES": fact_counts["READS"] + fact_counts["WRITES"],
            "ROM_READ_EDGES": sum(edge[0] == "READS" and
                                   edge[2].startswith("rom_read:") for edge in self.edges),
            "CALL_EDGES": fact_counts["CALLS"],
            "RETURN_EDGES": fact_counts["RETURNS_TO"],
            "INDIRECT_TARGETS": len(self.corpus.get("indirect_targets", [])),
            "UNRESOLVED": len(queue_rows) + sum(
                row.get("decoded_mnemonic") is None or
                row.get("register_snapshot_id") is None or
                row.get("opcode_verification") in {"MISMATCH", "UNRESOLVED"}
                for row in self.corpus.get("instructions", [])),
            "UNRESOLVED_CLOSURE_QUEUE": len(queue_rows)}
        return {"graph": graph, "calls": self._calls(),
                "returns": self.corpus.get("returns", []),
                "indirect_targets": self.corpus.get("indirect_targets", []),
                "tables": tables, "queue": queue_rows,
                "unresolved": self.unresolved, "iterations": per_iteration,
                "flow_metrics": self.flow_metrics,
                "provenance_metrics": provenance_metrics,
                "gap_ranking": ranking,
                "total_facts": len(self.edges),
                "cfg_edges": sum(edge[0] in {"CALLS", "BRANCHES_TO", "OBSERVED_NEXT_PC"}
                                 for edge in self.edges),
                "source_owned_before": before, "source_owned_after": before + promoted_bytes,
                "source_owned_delta": promoted_bytes, "promotions": self.promotions,
                "map_operations": self.map_operations,
                "fixpoint_reached": True,
                "status": "PASS" if graph["edges"] else "NO_DELTA"}


def emit_outputs(result: dict[str, Any], output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    payloads = {
        "postrun_provenance_graph.json": result["graph"],
        "postrun_recursive_closure.json": {"schema": "oasis.m12.recursive-closure.v1", **result},
        "postrun_rom_tables.json": {"tables": result["tables"]},
        "postrun_ram_structures.json": {"structures": []},
        "postrun_cfg_expansion.json": {"calls": result["calls"]},
        "postrun_indirect_targets.json": {"targets": []},
        "postrun_closure_queue.json": {"queue": result["queue"]},
        "postrun_capture_gap_ranking.json": {"schema": "oasis.m13.generic-gap-ranking.v1",
                                             "gaps": result.get("gap_ranking", result["unresolved"])},
    }
    for name, value in payloads.items():
        (output / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    hashes = {name: hashlib.sha256((output / name).read_bytes()).hexdigest() for name in payloads}
    receipt = {"schema": "oasis.m12.recursive-closure-receipt.v1", "status": result["status"],
               "iterations": result["iterations"], "source_owned_before": result["source_owned_before"],
               "source_owned_after": result["source_owned_after"], "source_owned_delta": result["source_owned_delta"],
               "output_hashes": hashes}
    (output / "postrun_recursive_closure_receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> None:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("corpus", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    corpus = json.loads(args.corpus.read_text(encoding="utf-8"))
    emit_outputs(RecursiveClosure(corpus).run(), args.output)


if __name__ == "__main__": main()
