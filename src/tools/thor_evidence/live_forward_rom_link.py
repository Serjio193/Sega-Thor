"""Post-run exact ROM-byte linkage for audited FLOW_V1 instruction records."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
from typing import Any

try:
    from .cartographer import canonical
    from .identity import ROM_SHA, ROM_SIZE
except ImportError:
    from cartographer import canonical
    from identity import ROM_SHA, ROM_SIZE


RECORD_SIZE = 48
FLAG_INSTRUCTION = 1
SCHEMA = "oasis.m12.live-forward-rom-ranges.v1"


def _sha(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _file_sha(path: Path) -> str:
    return _sha(path.read_bytes())


class LiveForwardRomLinker:
    """Stage audited records cheaply, then decode and merge after EmuHawk exits."""

    def __init__(self, rom_sha256: str = ROM_SHA):
        if rom_sha256 != ROM_SHA:
            raise ValueError("STOP_ROM_IDENTITY_MISMATCH")
        self.rom_sha256 = rom_sha256
        self._segments: list[tuple[dict[str, Any], bytes]] = []
        self._identities: set[tuple[int, int, int, int]] = set()

    def stage(self, segment: dict[str, object], rows: list[tuple[int, ...]],
              records_blob: bytes) -> None:
        """Retain only the immutable, already-audited callback payload."""
        if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
            raise ValueError("ROM linker accepts only host-audited FLOW_V1 segments")
        count = int(segment["record_count"])
        if count != len(rows) or len(records_blob) != count * RECORD_SIZE:
            raise ValueError("FLOW_V1 callback bytes and rows do not reconcile")
        identity = tuple(int(segment[key]) for key in
                         ("run_id", "worker_id", "capture_id", "generation"))
        if identity in self._identities:
            raise ValueError("STOP_ROM_LINK_DUPLICATE_SEGMENT_IDENTITY")
        self._identities.add(identity)
        self._segments.append((dict(segment), bytes(records_blob)))

    @staticmethod
    def _decode_batch(decoder: Path, rom: Path, pcs: set[int], output: Path) -> dict[int, dict[str, Any]]:
        input_path = output.with_suffix(".pcs.txt")
        result_path = output.with_suffix(".tsv")
        input_path.write_text("".join(f"0x{pc:08x}\n" for pc in sorted(pcs)), encoding="ascii")
        completed = subprocess.run([str(decoder.resolve()), str(rom.resolve()),
                                    str(input_path), str(result_path)],
                                   capture_output=True, text=True, check=False)
        if completed.returncode:
            raise RuntimeError("ROM range decoder failed: " + completed.stderr[-3000:])
        results: dict[int, dict[str, Any]] = {}
        for line in result_path.read_text(encoding="ascii").splitlines():
            fields = line.split("\t")
            if len(fields) != 9:
                raise ValueError("STOP_ROM_LINK_DECODER_OUTPUT_INVALID")
            raw_pc = int(fields[0], 0)
            item = {"cpu_address": int(fields[1], 0), "memory_region": fields[2],
                    "rom_offset": int(fields[3], 0) if fields[3] else None,
                    "rom_sha256": fields[4], "decode_status": fields[5],
                    "opcode": int(fields[6], 0) if fields[6] else None,
                    "length": int(fields[7]), "bytes": bytes.fromhex(fields[8])}
            if raw_pc in results:
                raise ValueError("STOP_ROM_LINK_DECODER_DUPLICATE_PC")
            results[raw_pc] = item
        if results.keys() != pcs:
            raise ValueError("STOP_ROM_LINK_DECODER_PC_SET_MISMATCH")
        return results

    @staticmethod
    def _instruction_rows(records_blob: bytes) -> list[tuple[int, tuple[int, ...]]]:
        import struct
        record = struct.Struct("<QQQIIIHBBHHI")
        return [(index, row) for index, row in enumerate(record.iter_unpack(records_blob))
                if row[6] & FLAG_INSTRUCTION]

    def project(self, graph: Any, rom_path: Path, decoder: Path,
                evidence_dir: Path) -> dict[str, Any]:
        rom_path, decoder, evidence_dir = rom_path.resolve(), decoder.resolve(), evidence_dir.resolve()
        rom = rom_path.read_bytes()
        if len(rom) != ROM_SIZE or _sha(rom) != ROM_SHA:
            raise ValueError("STOP_ROM_IDENTITY_MISMATCH")
        if not decoder.is_file():
            raise FileNotFoundError(f"native exact-range decoder missing: {decoder}")
        evidence_dir.mkdir(parents=True, exist_ok=True)
        raw_path = evidence_dir / "flow-v1-records.bin"
        index_path = evidence_dir / "flow-v1-segments.jsonl"
        if raw_path.exists() or index_path.exists():
            raise FileExistsError("refusing to overwrite staged FLOW_V1 evidence")

        offsets: list[tuple[dict[str, Any], bytes, int]] = []
        raw_offset = 0
        with raw_path.open("xb") as raw_file, index_path.open("x", encoding="utf-8", newline="\n") as index:
            for segment, blob in self._segments:
                encoded = (canonical(segment) + "\n").encode("utf-8")
                raw_file.write(blob)
                index.write(json.dumps({"segment": segment, "raw_offset": raw_offset,
                    "raw_length": len(blob), "raw_sha256": _sha(blob)},
                    sort_keys=True, separators=(",", ":")) + "\n")
                offsets.append((segment, blob, raw_offset))
                raw_offset += len(blob)

        pc_values = {row[3] for _, blob, _ in offsets
                     for _, row in self._instruction_rows(blob)}
        decoded = self._decode_batch(decoder, rom_path, pc_values, evidence_dir / "rom-decode")
        decoder_identity = _sha(decoder.read_bytes())
        source_owned_before = int(graph.db.execute(
            "SELECT value FROM map_meta WHERE key='source_owned_bytes'").fetchone()[0])
        range_nodes: dict[str, dict[str, Any]] = {}
        structural_edges: dict[str, dict[str, Any]] = {}
        intervals: set[tuple[int, int]] = set()
        range_evidence_count: dict[str, int] = {}
        observed_bytes = unique_occurrences = unresolved = 0
        unsupported_decode = 0
        terminal_count = 0
        lineage_db = graph.db
        lineage_db.execute("CREATE TABLE rom_link_pending_lineage("
            "edge_id TEXT NOT NULL,lineage_key TEXT NOT NULL,lineage_json TEXT NOT NULL,"
            "PRIMARY KEY(edge_id,lineage_key))")

        def add_lineage(edge: dict[str, Any], lineage: dict[str, Any]) -> None:
            edge_id = graph._edge_id(edge)
            structural_edges[edge_id] = edge
            encoded = canonical(lineage)
            lineage_db.execute("INSERT OR IGNORE INTO rom_link_pending_lineage VALUES (?,?,?)",
                (edge_id, _sha(encoded.encode("utf-8")), encoded))

        for segment, blob, raw_offset in offsets:
            instructions = self._instruction_rows(blob)
            for record_index, row in instructions:
                (stream_seq, instruction_seq, _, raw_pc, next_pc, flow_opcode,
                 flags, _, _, _, _, auxiliary) = row
                resolution = decoded[raw_pc]
                if resolution["memory_region"] == "ROM":
                    if resolution["rom_sha256"] != ROM_SHA:
                        raise ValueError("STOP_ROM_LINK_IDENTITY_CONFLICT")
                    if resolution["rom_offset"] is None or not 0 <= resolution["cpu_address"] < len(rom) or \
                            not 0 <= resolution["rom_offset"] < len(rom):
                        raise ValueError("STOP_ROM_LINK_FALSE_MAPPING")
                elif resolution["rom_offset"] is not None or resolution["rom_sha256"]:
                    raise ValueError("STOP_ROM_LINK_FALSE_MAPPING")
                if resolution["memory_region"] == "ROM" and resolution["opcode"] is not None and \
                        resolution["opcode"] != flow_opcode:
                    raise ValueError("STOP_ROM_LINK_FLOW_OPCODE_MISMATCH")
                source_item = {"kind": "M68K_INSTRUCTION", "key": f"{raw_pc:08X}:{flow_opcode:04X}",
                               "scope": "rom:" + ROM_SHA}
                source_id = graph._node_id(source_item)
                if lineage_db.execute("SELECT 1 FROM map_node WHERE node_id=?", (source_id,)).fetchone() is None:
                    raise ValueError("STOP_ROM_LINK_SOURCE_INSTRUCTION_MISSING")
                lineage = {"profile": "FLOW_V1", "run_id": int(segment["run_id"]),
                    "epoch": int(segment["epoch"]), "worker_id": int(segment["worker_id"]),
                    "capture_id": int(segment["capture_id"]), "generation": int(segment["generation"]),
                    "segment_sha256": str(segment["segment_sha256"]),
                    "records_sha256": str(segment["records_sha256"]),
                    "record_index": record_index, "stream_sequence": stream_seq,
                    "instruction_sequence": instruction_seq, "raw_cpu_pc": raw_pc,
                    "flow_opcode": flow_opcode, "flags": flags, "auxiliary": auxiliary,
                    "raw_records_offset": raw_offset + record_index * RECORD_SIZE,
                    "decoder_sha256": decoder_identity, "rom_sha256": ROM_SHA}
                if resolution["memory_region"] == "ROM" and resolution["decode_status"] == "DECODED":
                    bytecode = resolution["bytes"]
                    offset, length = resolution["rom_offset"], resolution["length"]
                    if offset is None or length < 2 or len(bytecode) != length or offset + length > len(rom):
                        raise ValueError("STOP_ROM_LINK_RANGE_OUT_OF_BOUNDS")
                    if int.from_bytes(bytecode[:2], "big") != flow_opcode or rom[offset:offset + length] != bytecode:
                        raise ValueError("STOP_ROM_LINK_BYTE_OR_OPCODE_MISMATCH")
                    end = offset + length
                    byte_sha = _sha(bytecode)
                    key = f"{offset:08X}:{end:08X}:{bytecode.hex().upper()}"
                    item = {"kind": "ROM_INSTRUCTION_RANGE", "key": key,
                        "scope": ROM_SHA, "status": "OBSERVED", "attributes": {
                            "rom_sha256": ROM_SHA, "start_offset": offset,
                            "end_offset_exclusive": end, "length": length,
                            "opcode": flow_opcode, "bytes_hex": bytecode.hex().upper(),
                            "bytes_sha256": byte_sha}, "lineage": []}
                    range_id = graph._node_id(item)
                    old = range_nodes.get(range_id)
                    if old is not None and old != item:
                        raise ValueError("STOP_ROM_LINK_IDENTITY_CONFLICT")
                    range_nodes[range_id] = item
                    range_evidence_count[range_id] = range_evidence_count.get(range_id, 0) + 1
                    edge = {"source": source_id, "target": range_id,
                        "relation": "EXECUTED_FROM_ROM", "scope": ROM_SHA,
                        "status": "OBSERVED", "rule": "FLOW_V1 opcode matched exact canonical ROM instruction bytes",
                        "assumptions": [], "lineage": []}
                    lineage.update({"cpu_address": resolution["cpu_address"],
                        "memory_region": resolution["memory_region"], "rom_offset": offset,
                        "instruction_length": length, "instruction_bytes_sha256": byte_sha,
                        "range_node_id": range_id})
                    add_lineage(edge, lineage)
                    intervals.add((offset, end))
                    observed_bytes += length
                    unique_occurrences += 1
                else:
                    unresolved += 1
                    reason = (resolution["decode_status"] if resolution["memory_region"] == "ROM"
                              else "NOT_ROM_BACKED")
                    if reason == "DECODE_UNSUPPORTED":
                        unsupported_decode += 1
                    missing = {"kind": "ROM_LINK_UNRESOLVED",
                        "key": f"{raw_pc:08X}:{flow_opcode:04X}:{reason}", "scope": ROM_SHA,
                        "status": "UNRESOLVED", "attributes": {
                            "raw_cpu_pc": raw_pc, "cpu_address": resolution["cpu_address"],
                            "memory_region": resolution["memory_region"], "reason": reason},
                        "lineage": []}
                    missing_id = graph._node_id(missing)
                    range_nodes[missing_id] = missing
                    edge = {"source": source_id, "target": missing_id,
                        "relation": "ROM_LINK_UNRESOLVED", "scope": ROM_SHA,
                        "status": "UNRESOLVED", "rule": "explicit resolver or bounded decoder did not prove a ROM instruction range",
                        "assumptions": [], "lineage": []}
                    lineage.update({"cpu_address": resolution["cpu_address"],
                        "memory_region": resolution["memory_region"], "reason": reason})
                    add_lineage(edge, lineage)

            if instructions:
                record_index, row = instructions[-1]
                target = {"kind": "M68K_TARGET_ADDRESS", "key": f"{row[4]:08X}",
                    "scope": "rom:" + ROM_SHA, "status": "OBSERVED",
                    "attributes": {"cpu": "M68K", "raw_next_pc": row[4],
                                   "meaning": "observed terminal FLOW_V1 next_pc only"},
                    "lineage": []}
                target_id = graph._node_id(target)
                range_nodes[target_id] = target
                source_id = graph._node_id({"kind": "M68K_INSTRUCTION",
                    "key": f"{row[3]:08X}:{row[5]:04X}", "scope": "rom:" + ROM_SHA})
                edge = {"source": source_id, "target": target_id,
                    "relation": "OBSERVED_NEXT_PC", "scope": ROM_SHA, "status": "OBSERVED",
                    "rule": "terminal instruction next_pc address fact; target execution is not asserted",
                    "assumptions": [], "lineage": []}
                terminal_lineage = {"run_id": int(segment["run_id"]), "epoch": int(segment["epoch"]),
                    "worker_id": int(segment["worker_id"]), "capture_id": int(segment["capture_id"]),
                    "generation": int(segment["generation"]), "segment_sha256": str(segment["segment_sha256"]),
                    "record_index": record_index, "stream_sequence": row[0],
                    "instruction_sequence": row[1], "raw_cpu_pc": row[3], "flow_opcode": row[5],
                    "raw_next_pc": row[4], "raw_records_offset": raw_offset + record_index * RECORD_SIZE}
                add_lineage(edge, terminal_lineage)
                terminal_count += 1

        lineage_db.commit()
        projection_sha = _sha("".join(str(segment["segment_sha256"]) for segment, _, _ in offsets).encode()
            + decoder_identity.encode() + ROM_SHA.encode())
        run_id = lineage_db.execute("SELECT value FROM map_meta WHERE key='live_forward_run_id'").fetchone()
        if run_id is None:
            raise ValueError("STOP_ROM_LINK_RUNTIME_RUN_ID_MISSING")
        delta = graph.merge({"nodes": list(range_nodes.values()), "edges": list(structural_edges.values()),
                             "frontiers": [], "resolves_frontiers": []},
                    f"live-forward-rom-link:{run_id[0]}:{projection_sha}", projection_sha,
                    compute_graph_hash=False, compute_components=False)
        if delta.new_conflicts:
            raise ValueError("STOP_ROM_LINK_IDENTITY_CONFLICT")
        cursor = lineage_db.execute("SELECT edge_id,lineage_json FROM rom_link_pending_lineage "
                                     "ORDER BY edge_id,lineage_json")
        current, values = None, []

        def save_lineage(edge_id: str, additions: list[dict[str, Any]]) -> None:
            existing = lineage_db.execute("SELECT lineage FROM map_edge WHERE edge_id=?",
                                          (edge_id,)).fetchone()
            if existing is None:
                raise ValueError("STOP_ROM_LINK_EXECUTION_CHAIN_LOSS")
            combined = json.loads(existing[0])
            for value in additions:
                if value not in combined:
                    combined.append(value)
            lineage_db.execute("UPDATE map_edge SET lineage=? WHERE edge_id=?",
                (canonical(sorted(combined, key=canonical)), edge_id))

        for edge_id, encoded in cursor:
            if current is not None and current != edge_id:
                save_lineage(current, values)
                values = []
            current = edge_id
            values.append(json.loads(encoded))
        if current is not None:
            save_lineage(current, values)
        lineage_db.execute("DROP TABLE rom_link_pending_lineage")
        lineage_db.commit()

        source_owned_after = int(lineage_db.execute(
            "SELECT value FROM map_meta WHERE key='source_owned_bytes'").fetchone()[0])
        if source_owned_after != source_owned_before:
            raise ValueError("STOP_ROM_LINK_SOURCE_OWNED_CHANGED")
        merged: list[tuple[int, int]] = []
        for start, end in sorted(intervals):
            if merged and start <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], end))
            else:
                merged.append((start, end))
        range_rows = []
        for node_id, item in sorted(range_nodes.items(), key=lambda pair: pair[1]["key"]):
            if item["kind"] != "ROM_INSTRUCTION_RANGE":
                continue
            range_rows.append({"range_node_id": node_id, **item["attributes"],
                               "evidence_count": range_evidence_count[node_id]})
        export = {"schema": SCHEMA, "rom_sha256": ROM_SHA,
            "decoder_sha256": decoder_identity, "interval_convention": "[start_offset,end_offset_exclusive)",
            "status": "STOP_ROM_LINK_DECODE_UNSUPPORTED" if unsupported_decode else
                      "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE",
            "metrics": {"instruction_occurrences": unique_occurrences,
                "observed_executed_rom_bytes": observed_bytes,
                "unique_instruction_ranges": len(range_rows),
                "unique_instruction_bytes": sum(end - start for start, end in merged),
                "unresolved_instruction_occurrences": unresolved,
                "unsupported_decode_occurrences": unsupported_decode,
                "terminal_next_pc_facts": terminal_count}, "ranges": range_rows}
        export_path = evidence_dir / "rom_execution_ranges.json"
        export_path.write_text(json.dumps(export, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return {"status": export["status"], **export["metrics"],
            "rom_sha256": ROM_SHA, "decoder_sha256": decoder_identity,
            "source_owned_before": source_owned_before, "source_owned_after": source_owned_after,
            "flow_records_path": str(raw_path), "flow_segments_path": str(index_path),
            "ranges_path": str(export_path), "flow_records_sha256": _file_sha(raw_path),
            "segment_count": len(offsets), "projection_sha256": projection_sha}
