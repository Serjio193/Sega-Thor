"""Streaming Stage 5 consumer for the R7 in-memory FLOW handoff."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
from typing import Any, Iterable

from cartographer import Cartographer
from identity import ROM_SHA, ROM_SIZE
from stage5_session_memory import Stage5SessionMemory, _lineage, _sha, _touch
from flow_stream import FlowChunk
from live_forward_scaling_audit import RECORD


@dataclass
class Stage5StreamConsumer:
    """Consume chunks once, retaining compact facts rather than raw FLOW."""

    rom_path: Path
    decoder: Path
    identity: str
    progress: Any = None
    run_id: int | None = None
    segments: int = 0
    records: int = 0
    flow_bytes: int = 0
    terminal_observations: int = 0
    instruction: dict[tuple[int, int], dict[str, Any]] = None
    next_facts: dict[tuple[tuple[int, int], tuple[int, int]], dict[str, Any]] = None
    terminals: dict[tuple[tuple[int, int], int], dict[str, Any]] = None
    _seen: set[tuple[int, int, int, int, int]] = None
    _previous_entry: int = -1
    _raw_offset: int = 0
    raw_sha256: str = "in-memory"

    def __post_init__(self) -> None:
        self.instruction = {}
        self.next_facts = {}
        self.terminals = {}
        self._seen = set()

    def accept(self, chunk: FlowChunk) -> None:
        segment, rows, blob = chunk.segment, chunk.rows, chunk.records_blob
        positive = ("run_id", "capture_id", "generation", "entry_stream_sequence",
                    "exit_stream_sequence", "record_count")
        if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
            raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:segment readiness")
        if any(not isinstance(segment.get(key), int) or segment[key] <= 0 for key in positive):
            raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:segment identity")
        if any(not isinstance(segment.get(key), int) or segment[key] < 0
               for key in ("epoch", "worker_id")):
            raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:segment identity")
        key = tuple(int(segment[name]) for name in
                    ("run_id", "epoch", "worker_id", "capture_id", "generation"))
        if key in self._seen:
            raise ValueError("STOP_POSTRUN_MAP_IDENTITY_CONFLICT")
        self._seen.add(key)
        source_run = int(segment["run_id"])
        if self.run_id is None:
            self.run_id = source_run
        if source_run != self.run_id or int(segment["entry_stream_sequence"]) <= self._previous_entry:
            raise ValueError("STOP_POSTRUN_MAP_IDENTITY_CONFLICT")
        self._previous_entry = int(segment["entry_stream_sequence"])
        expected = int(segment["record_count"])
        if len(rows) != expected or len(blob) != expected * RECORD.size:
            raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:record count")
        if hashlib.sha256(blob).hexdigest() != str(segment.get("records_sha256")):
            raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:record hash")
        instructions = [(i, row) for i, row in enumerate(rows) if (row[6] & 1) and row[7] == 0]
        for index, row in instructions:
            _touch(self.instruction, (int(row[3]), int(row[5] & 0xFFFF)),
                   _lineage(segment, index, row, self._raw_offset))
        for left_index, left, right_index, right in (
                (a, b, c, d) for (a, b), (c, d) in zip(instructions, instructions[1:])):
            if right[1] == left[1] + 1:
                _touch(self.next_facts, ((int(left[3]), int(left[5] & 0xFFFF)),
                                         (int(right[3]), int(right[5] & 0xFFFF))),
                       _lineage(segment, left_index, left, self._raw_offset))
        if instructions:
            index, row = instructions[-1]
            _touch(self.terminals, ((int(row[3]), int(row[5] & 0xFFFF)), int(row[4])),
                   _lineage(segment, index, row, self._raw_offset))
            self.terminal_observations += 1
        self.segments += 1
        self.records += expected
        self.flow_bytes += len(blob)
        self._raw_offset += len(blob)
        if self.progress and self.segments % 256 == 0:
            self.progress.update(self.segments,
                                 detail=f"validated {self.segments:,} FLOW segments in RAM")

    def finish(self) -> Stage5SessionMemory:
        if self.run_id is None:
            raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:empty FLOW stream")
        rom = self.rom_path.read_bytes()
        if len(rom) != ROM_SIZE or _sha(rom) != ROM_SHA:
            raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
        if self.decoder is None or not self.decoder.is_file():
            default_decoder = Path(__file__).parents[2] / "build" / "oasis_re_rom_range_decode.exe"
            if default_decoder.is_file():
                self.decoder = default_decoder
            else:
                raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:exact-range decoder is missing")
        from stage5_session_memory import _decode_batch_ephemeral
        resolutions = _decode_batch_ephemeral(self.decoder, self.rom_path,
                                              {pc for pc, _ in self.instruction})
        graph = Cartographer.in_memory(ROM_SHA, source_owned_bytes=0)
        scope = "rom:" + ROM_SHA
        nodes: dict[str, dict[str, Any]] = {}
        edges: dict[str, dict[str, Any]] = {}
        ids: dict[tuple[int, int], str] = {}
        for (pc, opcode), fact in self.instruction.items():
            resolution = resolutions[pc]
            if resolution["memory_region"] != "ROM" or resolution["decode_status"] != "DECODED" or \
                    resolution["opcode"] != opcode or resolution["rom_offset"] is None:
                raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
            offset, length, data = int(resolution["rom_offset"]), int(resolution["length"]), resolution["bytes"]
            if length < 2 or rom[offset:offset + length] != data or int.from_bytes(data[:2], "big") != opcode:
                raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
            source = {"kind": "M68K_INSTRUCTION", "key": f"{pc:08X}:{opcode:04X}",
                      "scope": scope, "status": "OBSERVED", "attributes":
                      {"cpu": "M68K", "pc": pc, "opcode": opcode, "record_kind": "instruction"},
                      "lineage": [fact["lineage"]]}
            range_node = {"kind": "ROM_INSTRUCTION_RANGE",
                          "key": f"{offset:08X}:{offset + length:08X}:{data.hex().upper()}",
                          "scope": ROM_SHA, "status": "OBSERVED", "attributes":
                          {"rom_sha256": ROM_SHA, "start_offset": offset,
                           "end_offset_exclusive": offset + length, "length": length,
                           "opcode": opcode, "bytes_hex": data.hex().upper(),
                           "bytes_sha256": _sha(data)}, "lineage": []}
            source_id, range_id = graph._node_id(source), graph._node_id(range_node)
            ids[(pc, opcode)] = source_id
            nodes.update({source_id: source, range_id: range_node})
            edge = {"source": source_id, "target": range_id, "relation": "EXECUTED_FROM_ROM",
                    "scope": ROM_SHA, "status": "OBSERVED",
                    "rule": "FLOW_V1 opcode matched exact canonical ROM instruction bytes",
                    "assumptions": [], "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        for (source, target), fact in self.next_facts.items():
            edge = {"source": ids[source], "target": ids[target], "relation": "EXECUTED_NEXT",
                    "scope": scope, "status": "OBSERVED", "rule": "adjacent FLOW_V1 instruction records",
                    "assumptions": [], "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        for (source, address), fact in self.terminals.items():
            target = {"kind": "M68K_TARGET_ADDRESS", "key": f"{address:08X}", "scope": scope,
                      "status": "OBSERVED", "attributes": {"cpu": "M68K", "raw_next_pc": address,
                      "meaning": "observed terminal FLOW_V1 next_pc only"}, "lineage": [fact["lineage"]]}
            target_id = graph._node_id(target)
            nodes[target_id] = target
            edge = {"source": ids[source], "target": target_id, "relation": "OBSERVED_NEXT_PC",
                    "scope": ROM_SHA, "status": "OBSERVED", "rule": "terminal next_pc address fact",
                    "assumptions": [], "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        graph._put_meta("live_forward_session_schema", "oasis.m12.live-forward-session.v1")
        graph._put_meta("live_forward_session_id", f"postrun-{self.run_id}")
        graph._put_meta("live_forward_instrumentation_identity", self.identity)
        graph._put_meta("live_forward_session_state", "CLOSED")
        graph._put_meta("live_forward_run_id", str(self.run_id))
        graph.db.commit()
        graph.merge({"nodes": list(nodes.values()), "edges": list(edges.values()),
                     "frontiers": [], "resolves_frontiers": []},
                    f"stage5:{self.run_id}:{self.raw_sha256}", self.raw_sha256,
                    compute_graph_hash=False, compute_components=False)
        graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                         ("live_forward_graph_sha256", graph.graph_hash()))
        graph.db.commit()
        source_sha = _sha(json.dumps({"session": graph.graph_hash(), "run_id": self.run_id},
                                     sort_keys=True, separators=(",", ":")).encode())
        memory = Stage5SessionMemory(self.run_id, ROM_SHA, self.segments, self.segments,
                   self.records, self.instruction, self.next_facts, self.terminals,
                   graph, self.flow_bytes, source_sha, 0)
        memory.terminal_observations = self.terminal_observations
        memory.handoff_mode = "IN_MEMORY_STREAM"
        return memory


__all__ = ["Stage5StreamConsumer"]
