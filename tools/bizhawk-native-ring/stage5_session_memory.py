"""RAM-backed Stage 5 FLOW session with ephemeral decoder I/O only."""
from __future__ import annotations

from dataclasses import dataclass, field
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import tracemalloc
from typing import Any

from cartographer import Cartographer
from identity import ROM_SHA, ROM_SIZE
from live_forward_scaling_audit import RECORD


def _sha(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _decode_batch_ephemeral(decoder: Path, rom: Path, pcs: set[int]) -> dict[int, dict[str, Any]]:
    """Call the existing exact decoder without leaving persistent .pcs/.tsv files."""
    with tempfile.TemporaryDirectory(prefix="thor-stage5-decoder-") as directory:
        root = Path(directory)
        source, result = root / "input.pcs.txt", root / "output.tsv"
        source.write_text("".join(f"0x{pc:08x}\n" for pc in sorted(pcs)), encoding="ascii")
        env = os.environ.copy()
        dll_dirs = [str(decoder.resolve().parent)]
        mingw = Path(r"C:\Dev\SegaThorTools\mingw64\bin")
        if mingw.is_dir():
            dll_dirs.append(str(mingw))
        env["PATH"] = os.pathsep.join(dll_dirs + [env.get("PATH", "")])
        completed = subprocess.run([str(decoder.resolve()), str(rom.resolve()),
                                    str(source), str(result)], capture_output=True,
                                   text=True, check=False, env=env)
        if completed.returncode:
            raise RuntimeError("ROM range decoder failed: " + completed.stderr[-3000:])
        values: dict[int, dict[str, Any]] = {}
        for line in result.read_text(encoding="ascii").splitlines():
            fields = line.split("\t")
            if len(fields) != 9:
                raise ValueError("STOP_ROM_LINK_DECODER_OUTPUT_INVALID")
            raw_pc = int(fields[0], 0)
            if raw_pc in values:
                raise ValueError("STOP_ROM_LINK_DECODER_DUPLICATE_PC")
            values[raw_pc] = {
                "cpu_address": int(fields[1], 0), "memory_region": fields[2],
                "rom_offset": int(fields[3], 0) if fields[3] else None,
                "rom_sha256": fields[4], "decode_status": fields[5],
                "opcode": int(fields[6], 0) if fields[6] else None,
                "length": int(fields[7]), "bytes": bytes.fromhex(fields[8]),
            }
        if set(values) != pcs:
            raise ValueError("STOP_ROM_LINK_DECODER_PC_SET_MISMATCH")
        return values


def _lineage(segment: dict[str, Any], index: int, row: tuple[int, ...], offset: int) -> dict[str, Any]:
    return {"profile": "FLOW_V1", "run_id": int(segment["run_id"]),
            "epoch": int(segment["epoch"]), "worker_id": int(segment["worker_id"]),
            "capture_id": int(segment["capture_id"]), "generation": int(segment["generation"]),
            "segment_sha256": str(segment["segment_sha256"]),
            "records_sha256": str(segment["records_sha256"]), "record_index": index,
            "stream_sequence": int(row[0]), "instruction_sequence": int(row[1]),
            "raw_cpu_pc": int(row[3]), "flow_opcode": int(row[5] & 0xFFFF), "flags": int(row[6]),
            "auxiliary": int(row[7]), "raw_records_offset": offset + index * RECORD.size}


def _touch(facts: dict[tuple[Any, ...], dict[str, Any]], key: tuple[Any, ...],
           evidence: dict[str, Any]) -> None:
    current = facts.get(key)
    if current is None:
        evidence.update({"occurrence_count": 1, "witness_count": 1, "segment_count": 1})
        facts[key] = {"lineage": evidence, "occurrence_count": 1,
                      "workers": {evidence["worker_id"]},
                      "segments": {evidence["segment_sha256"]}}
        return
    current["occurrence_count"] += 1
    current["workers"].add(evidence["worker_id"])
    current["segments"].add(evidence["segment_sha256"])
    first = current["lineage"]
    first["first_stream_sequence"] = min(first.get("first_stream_sequence", first["stream_sequence"]),
                                          evidence["stream_sequence"])
    first["last_stream_sequence"] = max(first.get("last_stream_sequence", first["stream_sequence"]),
                                         evidence["stream_sequence"])
    first.update({"occurrence_count": current["occurrence_count"],
                  "witness_count": len(current["workers"]),
                  "segment_count": len(current["segments"])})


@dataclass
class Stage5SessionMemory:
    """Semantic Stage 5 state; the graph is an SQLite :memory: database."""

    run_id: int
    rom_sha256: str
    segments_total: int
    segments_processed: int
    records_processed: int
    instruction_occurrences: dict[tuple[int, int], dict[str, Any]]
    executed_next: dict[tuple[tuple[int, int], tuple[int, int]], dict[str, Any]]
    terminal_facts: dict[tuple[tuple[int, int], int], dict[str, Any]]
    graph: Cartographer
    flow_bytes_read: int
    source_sha256: str
    ram_peak_bytes: int = 0
    conflict_count: int = 0

    @classmethod
    def from_receipt(cls, receipt: dict[str, Any], rom_path: Path, decoder: Path,
                     progress: Any = None) -> "Stage5SessionMemory":
        spool = receipt.get("raw_segment_spool", {})
        raw, index = Path(spool.get("raw_path", "")), Path(spool.get("index_path", ""))
        if not raw.is_file() or not index.is_file():
            archive = raw.parent.parent / "raw-evidence-archive"
            if (archive / raw.name).is_file() and (archive / index.name).is_file():
                raw, index = archive / raw.name, archive / index.name
            else:
                raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:ordered FLOW spool is missing")
        rom = rom_path.read_bytes()
        if len(rom) != ROM_SIZE or _sha(rom) != ROM_SHA:
            raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
        if decoder is None or not decoder.is_file():
            default_decoder = Path(__file__).parents[2] / "build" / "oasis_re_rom_range_decode.exe"
            if default_decoder.is_file():
                decoder = default_decoder
            else:
                raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:exact-range decoder is missing")
        run_id = int(receipt.get("runtime", {}).get("run_id", 0))
        identity = str(receipt.get("instrumentation_identity", ""))
        if run_id <= 0 or len(identity) != 64:
            raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:runtime identity is incomplete")
        instruction: dict[tuple[int, int], dict[str, Any]] = {}
        next_facts: dict[tuple[tuple[int, int], tuple[int, int]], dict[str, Any]] = {}
        terminals: dict[tuple[tuple[int, int], int], dict[str, Any]] = {}
        seen: set[tuple[int, int, int, int, int]] = set()
        previous_entry, segments, records, flow_bytes = -1, 0, 0, 0
        source_run: int | None = None
        tracemalloc.start()
        with index.open("r", encoding="utf-8") as index_stream, raw.open("rb") as raw_stream:
            for line in index_stream:
                item = json.loads(line)
                segment = item.get("segment", {})
                positive = ("run_id", "capture_id", "generation", "entry_stream_sequence",
                            "exit_stream_sequence", "record_count")
                if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True or \
                        any(not isinstance(segment.get(key), int) or segment[key] <= 0 for key in positive) or \
                        any(not isinstance(segment.get(key), int) or segment[key] < 0 for key in ("epoch", "worker_id")):
                    raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:segment identity")
                key = tuple(int(segment[name]) for name in ("run_id", "epoch", "worker_id", "capture_id", "generation"))
                if key in seen:
                    raise ValueError("STOP_POSTRUN_MAP_IDENTITY_CONFLICT")
                seen.add(key)
                source_run = source_run if source_run is not None else int(segment["run_id"])
                if source_run != int(segment["run_id"]) or source_run != run_id or \
                        int(segment["entry_stream_sequence"]) <= previous_entry:
                    raise ValueError("STOP_POSTRUN_MAP_IDENTITY_CONFLICT")
                previous_entry = int(segment["entry_stream_sequence"])
                offset, length = int(item["raw_offset"]), int(item["raw_length"])
                raw_stream.seek(offset)
                blob = raw_stream.read(length)
                if len(blob) != length or _sha(blob) != item.get("raw_sha256"):
                    raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:segment hash")
                rows = list(RECORD.iter_unpack(blob))
                if len(rows) != int(segment["record_count"]) or length != len(rows) * RECORD.size or \
                        any(right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
                    raise ValueError("STOP_POSTRUN_MAP_COUNT_MISMATCH:record ordering")
                instructions = [(i, row) for i, row in enumerate(rows) if (row[6] & 1) and row[7] == 0]
                for i, row in instructions:
                    _touch(instruction, (int(row[3]), int(row[5] & 0xFFFF)), _lineage(segment, i, row, offset))
                for (left_i, left), (right_i, right) in zip(instructions, instructions[1:]):
                    if right[1] == left[1] + 1:
                        _touch(next_facts, ((int(left[3]), int(left[5] & 0xFFFF)), (int(right[3]), int(right[5] & 0xFFFF))),
                               _lineage(segment, left_i, left, offset))
                if instructions:
                    i, row = instructions[-1]
                    _touch(terminals, ((int(row[3]), int(row[5] & 0xFFFF)), int(row[4])), _lineage(segment, i, row, offset))
                segments += 1
                records += len(rows)
                flow_bytes += len(blob)
                if progress and segments % 256 == 0:
                    progress.update(segments, detail=f"validated {segments:,} FLOW segments in RAM")
        pcs = {pc for pc, _ in instruction}
        resolutions = _decode_batch_ephemeral(decoder, rom_path, pcs)
        graph = Cartographer.in_memory(ROM_SHA, source_owned_bytes=0)
        scope = "rom:" + ROM_SHA
        nodes: dict[str, dict[str, Any]] = {}
        edges: dict[str, dict[str, Any]] = {}
        ids: dict[tuple[int, int], str] = {}
        for (pc, opcode), fact in instruction.items():
            resolution = resolutions[pc]
            if resolution["memory_region"] != "ROM" or resolution["decode_status"] != "DECODED" or \
                    resolution["opcode"] != opcode or resolution["rom_offset"] is None:
                raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
            offset, length, data = int(resolution["rom_offset"]), int(resolution["length"]), resolution["bytes"]
            if length < 2 or len(data) != length or not 0 <= offset < offset + length <= len(rom) or \
                    rom[offset:offset + length] != data or int.from_bytes(data[:2], "big") != opcode:
                raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
            source = {"kind": "M68K_INSTRUCTION", "key": f"{pc:08X}:{opcode:04X}",
                      "scope": scope, "status": "OBSERVED", "attributes": {"cpu": "M68K", "pc": pc,
                      "opcode": opcode, "record_kind": "instruction"}, "lineage": [fact["lineage"]]}
            range_node = {"kind": "ROM_INSTRUCTION_RANGE", "key": f"{offset:08X}:{offset + length:08X}:{data.hex().upper()}",
                          "scope": ROM_SHA, "status": "OBSERVED", "attributes": {"rom_sha256": ROM_SHA,
                          "start_offset": offset, "end_offset_exclusive": offset + length, "length": length,
                          "opcode": opcode, "bytes_hex": data.hex().upper(), "bytes_sha256": _sha(data)}, "lineage": []}
            source_id, range_id = graph._node_id(source), graph._node_id(range_node)
            ids[(pc, opcode)] = source_id
            nodes.update({source_id: source, range_id: range_node})
            edge = {"source": source_id, "target": range_id, "relation": "EXECUTED_FROM_ROM", "scope": ROM_SHA,
                    "status": "OBSERVED", "rule": "FLOW_V1 opcode matched exact canonical ROM instruction bytes",
                    "assumptions": [], "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        for (source, target), fact in next_facts.items():
            edge = {"source": ids[source], "target": ids[target], "relation": "EXECUTED_NEXT", "scope": scope,
                    "status": "OBSERVED", "rule": "adjacent FLOW_V1 instruction records", "assumptions": [],
                    "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        for (source, address), fact in terminals.items():
            target = {"kind": "M68K_TARGET_ADDRESS", "key": f"{address:08X}", "scope": scope,
                      "status": "OBSERVED", "attributes": {"cpu": "M68K", "raw_next_pc": address,
                      "meaning": "observed terminal FLOW_V1 next_pc only"}, "lineage": [fact["lineage"]]}
            target_id = graph._node_id(target)
            nodes[target_id] = target
            edge = {"source": ids[source], "target": target_id, "relation": "OBSERVED_NEXT_PC", "scope": ROM_SHA,
                    "status": "OBSERVED", "rule": "terminal next_pc address fact", "assumptions": [],
                    "lineage": [fact["lineage"]]}
            edges[graph._edge_id(edge)] = edge
        graph._put_meta("live_forward_session_schema", "oasis.m12.live-forward-session.v1")
        graph._put_meta("live_forward_session_id", f"postrun-{run_id}")
        graph._put_meta("live_forward_instrumentation_identity", identity)
        graph._put_meta("live_forward_session_state", "CLOSED")
        graph._put_meta("live_forward_run_id", str(run_id))
        graph.db.commit()
        graph.merge({"nodes": list(nodes.values()), "edges": list(edges.values()),
                     "frontiers": [], "resolves_frontiers": []},
                    f"stage5:{run_id}:{spool.get('raw_sha256', '')}", str(spool.get("raw_sha256", "")),
                    compute_graph_hash=False, compute_components=False)
        graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                         ("live_forward_graph_sha256", graph.graph_hash()))
        graph.db.commit()
        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        source_sha = _sha(_canonical({"session": graph.graph_hash(), "run_id": run_id}).encode())
        return cls(run_id, ROM_SHA, int(spool.get("segments", segments)), segments, records,
                   instruction, next_facts, terminals, graph, flow_bytes, source_sha, peak)

    def export_bundle(self) -> dict[str, list[dict[str, Any]]]:
        return self.graph.export_bundle()

    def close(self) -> None:
        self.graph.close()


__all__ = ["Stage5SessionMemory"]
