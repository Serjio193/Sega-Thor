"""Build and retain an observed MAP-1 execution graph for one live-forward run."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import struct
from typing import Any
import uuid
from itertools import groupby

try:
    from .cartographer import Cartographer, canonical
    from .rom_knowledge_map import runtime_occurrence_id
    from .runtime_occurrence_merge import occurrence_hash
except ImportError:
    from cartographer import Cartographer, canonical
    from rom_knowledge_map import runtime_occurrence_id
    from runtime_occurrence_merge import occurrence_hash


SESSION_SCHEMA = "oasis.m12.live-forward-session.v1"
FLOW_PROFILE = "FLOW_V1"
RECORD_SIZE = 48
FLAG_INSTRUCTION = 1
FLAG_FAULTED = 4
FLAG_CONTROL_FLOW = 8
FLAG_BRANCH_TAKEN = 16
FLAG_BRANCH_NOT_TAKEN = 32
FLAG_EXCEPTION_EVENT = 256
FLAG_EXCEPTION = 64
FLAG_ASYNCHRONOUS = 128
FLAG_CPU_STOP_EVENT = 1024
FLAG_EVENT = 0x8000
EVENT_SHIFT, EVENT_MASK = 11, 0x3800


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def _sha256(value: str, label: str) -> str:
    normalized = value.lower()
    if len(normalized) != 64 or any(char not in "0123456789abcdef" for char in normalized):
        raise ValueError(f"invalid {label} SHA-256")
    return normalized


def _instruction_kind(row: tuple[int, ...]) -> tuple[str, str, dict[str, Any]]:
    stream_sequence, instruction_sequence, master_time, pc, next_pc, opcode, flags, cpu_id, length_or_width, domain, reserved, auxiliary = row
    if flags & FLAG_FAULTED:
        raise ValueError("faulted FLOW_V1 record rejected by Cartographer")
    if flags & FLAG_INSTRUCTION:
        return ("M68K_INSTRUCTION", f"{pc:08X}:{opcode:04X}",
                {"cpu": "M68K", "pc": pc, "opcode": opcode, "record_kind": "instruction"})
    if flags & FLAG_EXCEPTION_EVENT and flags & FLAG_CPU_STOP_EVENT:
        raise ValueError("FLOW_V1 record has multiple non-instruction event kinds")
    if flags & FLAG_EXCEPTION_EVENT:
        return ("M68K_EXCEPTION_EVENT",
                f"{pc:08X}:{next_pc:08X}:V{opcode:04X}:A{auxiliary:08X}",
                {"cpu": "M68K", "pc": pc, "next_pc": next_pc, "vector": opcode,
                 "auxiliary": auxiliary, "record_kind": "exception_event"})
    if flags & FLAG_CPU_STOP_EVENT:
        return ("M68K_CPU_STOP_EVENT", f"{pc:08X}:{next_pc:08X}:{auxiliary:08X}",
                {"cpu": "M68K", "pc": pc, "next_pc": next_pc,
                 "auxiliary": auxiliary, "record_kind": "cpu_stop_event"})
    raise ValueError("untyped FLOW_V1 record rejected by Cartographer")


def _outcome(row: tuple[int, ...]) -> str:
    _, _, _, _, _, opcode, flags, _, _, _, _, _ = row
    if flags & FLAG_EXCEPTION_EVENT:
        return "exception_interrupt"
    if flags & FLAG_CPU_STOP_EVENT:
        return "cpu_stop"
    if flags & FLAG_INSTRUCTION:
        if opcode in (0x4E73, 0x4E75, 0x4E77):
            return "return"
        if opcode == 0x4EBA or opcode == 0x4EB9 or opcode & 0xFFC0 == 0x4E80:
            return "call"
        if opcode & 0xF000 == 0x6000 and (opcode >> 8) & 0xF == 1:
            return "call"
        if flags & FLAG_EXCEPTION:
            return "asynchronous_exception" if flags & FLAG_ASYNCHRONOUS else "exception"
    if flags & FLAG_BRANCH_TAKEN:
        return "branch_taken"
    if flags & FLAG_BRANCH_NOT_TAKEN:
        return "branch_not_taken"
    return "control_flow" if flags & FLAG_CONTROL_FLOW else "sequential"


class LiveForwardCartographer:
    """Own a fresh RAM Cartographer and save it only after runtime closure."""

    def __init__(self, rom_sha256: str, instrumentation_identity: str,
                 session_id: str | None = None):
        self.rom_sha256 = _sha256(rom_sha256, "ROM")
        self.instrumentation_identity = _sha256(instrumentation_identity, "instrumentation")
        self.session_id = session_id or str(uuid.uuid4())
        self.created_utc = _utc_now()
        self.graph = Cartographer.in_memory(self.rom_sha256, source_owned_bytes=0)
        if self.graph.path is not None:
            raise RuntimeError("live-forward Cartographer must be RAM-only")
        self.graph.db.execute(
            "CREATE TABLE live_forward_pending_lineage("
            "object_type TEXT NOT NULL, object_id TEXT NOT NULL, lineage_key TEXT NOT NULL, "
            "lineage_json TEXT NOT NULL, PRIMARY KEY(object_type, object_id, lineage_key))")
        self.graph.db.execute(
            "CREATE TABLE live_forward_runtime_occurrence("
            "occurrence_id TEXT PRIMARY KEY, event_json TEXT NOT NULL)")
        self.graph.db.commit()
        self._lineage_finalized = False
        self._meta("live_forward_session_schema", SESSION_SCHEMA)
        self._meta("live_forward_session_id", self.session_id)
        self._meta("live_forward_instrumentation_identity", self.instrumentation_identity)
        self._meta("live_forward_created_utc", self.created_utc)
        self._meta("live_forward_session_state", "OPEN")
        self.segments_admitted = 0
        self.segments_rejected = 0
        self.duplicate_structural_edges = 0
        self.run_id: int | None = None

    def _meta(self, key: str, value: str) -> None:
        self.graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?, ?)", (key, value))
        self.graph.db.commit()

    def admit(self, segment: dict[str, object], rows: list[tuple[int, ...]],
              records_blob: bytes) -> dict[str, Any]:
        try:
            return self._admit(segment, rows, records_blob)
        except Exception:
            self.segments_rejected += 1
            raise

    def _admit(self, segment: dict[str, object], rows: list[tuple[int, ...]],
               records_blob: bytes) -> dict[str, Any]:
        if self._lineage_finalized:
            raise ValueError("cannot admit a segment after session close")
        if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
            raise ValueError("segment is not READY_FOR_CARTOGRAPHER FLOW_V1")
        integer_fields = ("run_id", "epoch", "worker_id", "capture_id", "generation",
                          "entry_stream_sequence", "exit_stream_sequence",
                          "entry_instruction_sequence", "exit_instruction_sequence",
                          "record_count", "configured_depth")
        values = {key: segment.get(key) for key in integer_fields}
        if any(not isinstance(value, int) or value < 0 for value in values.values()):
            raise ValueError("FLOW_V1 segment has invalid identity or bounds")
        segment_hash = _sha256(str(segment.get("segment_sha256", "")), "segment")
        records_hash = _sha256(str(segment.get("records_sha256", "")), "records")
        source_raw_sha256 = segment.get("source_raw_sha256")
        source_index_sha256 = segment.get("source_index_sha256")
        raw_offset = segment.get("raw_offset")
        if source_raw_sha256 is not None:
            source_raw_sha256 = _sha256(str(source_raw_sha256), "raw source")
        if source_index_sha256 is not None:
            source_index_sha256 = _sha256(str(source_index_sha256), "index source")
        if raw_offset is not None and (not isinstance(raw_offset, int) or raw_offset < 0):
            raise ValueError("FLOW_V1 segment has invalid raw source offset")
        if any(value is not None for value in
               (source_raw_sha256, source_index_sha256, raw_offset)) and \
                any(value is None for value in
                    (source_raw_sha256, source_index_sha256, raw_offset)):
            raise ValueError("FLOW_V1 raw provenance is incomplete")
        if hashlib.sha256(records_blob).hexdigest() != records_hash:
            raise ValueError("FLOW_V1 segment record bytes changed after validation")
        record_count = int(values["record_count"])
        if record_count != len(rows) or len(records_blob) != record_count * RECORD_SIZE or not rows:
            raise ValueError("FLOW_V1 segment record count or byte length mismatch")
        if int(values["run_id"]) <= 0 or int(values["epoch"]) <= 0 or \
                int(values["capture_id"]) <= 0 or int(values["generation"]) <= 0:
            raise ValueError("FLOW_V1 segment identity must be positive")
        if int(values["exit_stream_sequence"]) <= int(values["entry_stream_sequence"]):
            raise ValueError("FLOW_V1 segment stream bounds are reversed")
        if rows[0][0] != values["entry_stream_sequence"] or \
                rows[-1][0] + 1 != values["exit_stream_sequence"]:
            raise ValueError("FLOW_V1 segment bounds differ from ordered records")
        if any(len(row) != 12 for row in rows) or any(
                right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
            raise ValueError("FLOW_V1 segment records are not an ordered stream")
        run_id = int(values["run_id"])
        if self.run_id is not None and self.run_id != run_id:
            raise ValueError("one RAM Cartographer session cannot mix runtime run IDs")
        if self.run_id is None:
            self.run_id = run_id
            self._meta("live_forward_run_id", str(run_id))
        scope = "rom:" + self.rom_sha256
        common = {"profile": FLOW_PROFILE, "run_id": run_id, "epoch": int(values["epoch"]),
                  "worker_id": int(values["worker_id"]), "capture_id": int(values["capture_id"]),
                  "generation": int(values["generation"]), "segment_sha256": segment_hash,
                  "session_id": self.session_id,
                  "instrumentation_identity": self.instrumentation_identity,
                  "records_sha256": records_hash,
                  "entry_stream_sequence": int(values["entry_stream_sequence"]),
                  "exit_stream_sequence": int(values["exit_stream_sequence"]),
                  "entry_instruction_sequence": int(values["entry_instruction_sequence"]),
                  "exit_instruction_sequence": int(values["exit_instruction_sequence"]),
                  "configured_depth": int(values["configured_depth"]),
                  "source_raw_sha256": source_raw_sha256,
                  "source_index_sha256": source_index_sha256,
                  "raw_offset": raw_offset}
        node_items: dict[str, dict[str, Any]] = {}
        node_lineages: dict[str, dict[str, Any]] = {}
        node_ids: list[str] = []
        semantic_rows: list[tuple[int, ...]] = []
        for index, row in enumerate(rows):
            if row[7] not in (0, 1, 255):
                raise ValueError("STOP_RUNTIME_OCCURRENCE_CPU_ID_INVALID")
            if row[6] & FLAG_EVENT:
                continue
            # Z80 instruction records remain scoped runtime occurrences. This
            # MAP-1 graph currently models M68K ROM instruction nodes only.
            if row[7] != 0:
                continue
            kind, key, attributes = _instruction_kind(row)
            item = {"kind": kind, "key": key, "scope": scope, "status": "OBSERVED",
                    "attributes": attributes, "lineage": []}
            node_id = self.graph._node_id(item)
            if node_id in node_items and node_items[node_id]["attributes"] != attributes:
                raise ValueError("incompatible FLOW_V1 node identity within segment")
            node_items[node_id] = item
            node_ids.append(node_id)
            semantic_rows.append(row)
            summary = node_lineages.setdefault(node_id, {**common,
                "first_record_index": index, "last_record_index": index,
                "first_stream_sequence": row[0], "last_stream_sequence": row[0],
                "first_instruction_sequence": row[1], "last_instruction_sequence": row[1],
                "occurrence_count": 0, "kind_flags_or": 0})
            summary["last_record_index"] = index
            summary["last_stream_sequence"] = row[0]
            summary["last_instruction_sequence"] = row[1]
            summary["occurrence_count"] += 1
            summary["kind_flags_or"] |= row[6]
        edge_items: dict[str, dict[str, Any]] = {}
        edge_lineages: dict[str, dict[str, Any]] = {}
        edge_outcomes: dict[str, set[str]] = {}
        for index, (source, target) in enumerate(zip(node_ids, node_ids[1:])):
            edge = {"source": source, "target": target, "relation": "EXECUTED_NEXT",
                    "scope": scope, "status": "OBSERVED",
                    "rule": "FLOW_V1 ordered execution-record adjacency",
                    "assumptions": [], "lineage": []}
            edge_id = self.graph._edge_id(edge)
            edge_items[edge_id] = edge
            summary = edge_lineages.setdefault(edge_id, {**common,
                "first_record_index": index, "last_record_index": index,
                "first_stream_sequence": semantic_rows[index][0],
                "last_stream_sequence": semantic_rows[index][0],
                "next_stream_sequence_first": semantic_rows[index + 1][0],
                "next_stream_sequence_last": semantic_rows[index + 1][0],
                "occurrence_count": 0, "kind_flags_or": 0})
            summary["last_record_index"] = index
            summary["last_stream_sequence"] = semantic_rows[index][0]
            summary["next_stream_sequence_last"] = semantic_rows[index + 1][0]
            summary["occurrence_count"] += 1
            summary["kind_flags_or"] |= semantic_rows[index][6]
            edge_outcomes.setdefault(edge_id, set()).add(_outcome(semantic_rows[index]))
        for edge_id, summary in edge_lineages.items():
            summary["control_flow_outcomes"] = sorted(edge_outcomes[edge_id])
        instruction_nodes = {int(row[1]): node_id for row, node_id in
            zip(semantic_rows, node_ids) if row[6] & FLAG_INSTRUCTION and row[7] == 0}
        window = {"worker_id": int(values["worker_id"]),
            "capture_id": int(values["capture_id"]),
            "generation": int(values["generation"]), "segment_sha256": segment_hash,
            "source_raw_sha256": source_raw_sha256,
            "source_index_sha256": source_index_sha256,
            "raw_offset": raw_offset,
            "entry_stream_sequence": int(values["entry_stream_sequence"]),
            "exit_stream_sequence": int(values["exit_stream_sequence"]),
            "record_count": int(values["record_count"])}
        def occurrence_window(row: tuple[int, ...]) -> dict[str, Any]:
            row_window = dict(window)
            if raw_offset is not None:
                row_window["source_offset"] = raw_offset + (
                    int(row[0]) - int(values["entry_stream_sequence"])) * RECORD_SIZE
            return row_window

        for row in rows:
            self._record_runtime_occurrence(
                row, values, occurrence_window(row), instruction_nodes)
        for index, (source_id, target_id) in enumerate(zip(node_ids, node_ids[1:])):
            source = semantic_rows[index]
            edge = {"source": source_id, "target": target_id,
                    "relation": "EXECUTED_NEXT", "scope": scope}
            self._record_runtime_occurrence(source, values, occurrence_window(source), instruction_nodes,
                event_kind="EXECUTED_NEXT", edge_id=self.graph._edge_id(edge),
                target_node_id=target_id)
        self.graph.db.commit()
        import_ref = (f"live-forward:{run_id}:{values['worker_id']}:{values['capture_id']}:"
                      f"{values['generation']}:{segment_hash}")
        delta = self.graph.merge({"nodes": list(node_items.values()),
                                  "edges": list(edge_items.values()), "frontiers": [],
                                  "resolves_frontiers": []}, import_ref, segment_hash,
                                 compute_graph_hash=False, compute_components=False)
        if delta.new_conflicts:
            raise ValueError("STOP_CARTOGRAPHER_EXECUTION_IDENTITY_CONFLICT")
        pending = [("node", object_id, lineage) for object_id, lineage in node_lineages.items()]
        pending.extend(("edge", object_id, lineage) for object_id, lineage in edge_lineages.items())
        encoded = []
        for object_type, object_id, lineage in pending:
            lineage_json = canonical(lineage)
            lineage_key = hashlib.sha256(lineage_json.encode("utf-8")).hexdigest()
            encoded.append((object_type, object_id, lineage_key, lineage_json))
        self.graph.db.executemany(
            "INSERT OR IGNORE INTO live_forward_pending_lineage VALUES (?, ?, ?, ?)", encoded)
        self.graph.db.commit()
        self.segments_admitted += 1
        self.duplicate_structural_edges += max(0, len(semantic_rows) - 1 - delta.new_edges)
        return {"import_ref": import_ref, "new_nodes": delta.new_nodes,
                "new_edges": delta.new_edges, "observed_edges": len(rows) - 1,
                "graph_hash": None}

    def _record_runtime_occurrence(self, row: tuple[int, ...], values: dict[str, int],
                                   window: dict[str, Any],
                                   instruction_nodes: dict[int, str],
                                   event_kind: str | None = None,
                                   edge_id: str | None = None,
                                   target_node_id: str | None = None) -> None:
        stream_seq, instruction_seq, master_time, pc, address, value, flags, cpu_id, width, domain, reserved, auxiliary = row
        subtype = (flags & EVENT_MASK) >> EVENT_SHIFT
        if event_kind is None:
            if flags & FLAG_INSTRUCTION:
                event_kind = "INSTRUCTION"
            elif flags & FLAG_EVENT:
                event_kind = {1: "BUS_READ", 2: "BUS_WRITE"}.get(
                    subtype, f"FLOW_EVENT_{subtype}")
            elif flags & FLAG_EXCEPTION_EVENT:
                event_kind = "EXCEPTION_EVENT"
            elif flags & FLAG_CPU_STOP_EVENT:
                event_kind = "CPU_STOP_EVENT"
            else:
                return
        cpu = {0: "M68K", 1: "Z80"}.get(cpu_id)
        if cpu is None:
            raise ValueError("STOP_RUNTIME_OCCURRENCE_CPU_ID_INVALID")
        run_id, epoch = int(values["run_id"]), int(values["epoch"])
        # Worker capture IDs identify windows. The native run/epoch is the
        # canonical capture scope so the same sequence in overlapping windows
        # resolves to one occurrence while each window remains in provenance.
        capture_scope = f"native-run:{run_id}:epoch:{epoch}"
        address_space = f"FLOW_DOMAIN_{domain}"
        occurrence_id = runtime_occurrence_id(capture_id=capture_scope,
            epoch=epoch, cpu=cpu, address_space=address_space,
            native_sequence=stream_seq, event_kind=event_kind,
            run_id=run_id, instruction_sequence=instruction_seq)
        payload = {"occurrence_id": occurrence_id, "capture_id": capture_scope,
            "capture_ids": [int(window["capture_id"])], "run_id": run_id,
            "epoch": epoch, "cpu_id": cpu, "address_space": address_space,
            "native_sequence": int(stream_seq),
            "instruction_sequence": int(instruction_seq), "master_time": int(master_time),
            "event_kind": event_kind,
            "pc": int(pc), "address": int(address), "value": int(value),
            "width": int(width), "flags": int(flags), "reserved": int(reserved),
            "auxiliary": int(auxiliary),
            "record_hex": struct.pack("<QQQIIIHBBHHI", *row).hex(),
            "instruction_node_id": instruction_nodes.get(int(instruction_seq)),
            "edge_id": edge_id, "target_node_id": target_node_id,
            "windows": [window]}
        existing = self.graph.db.execute(
            "SELECT event_json FROM live_forward_runtime_occurrence WHERE occurrence_id=?",
            (occurrence_id,)).fetchone()
        if existing:
            prior = json.loads(existing[0])
            for key in ("capture_ids", "windows"):
                payload[key] = sorted({canonical(item): item
                    for item in prior[key] + payload[key]}.values(), key=canonical)
            prior_core = {key: value for key, value in prior.items()
                          if key not in {"capture_ids", "windows"}}
            payload_core = {key: value for key, value in payload.items()
                            if key not in {"capture_ids", "windows"}}
            if prior_core != payload_core:
                raise ValueError("STOP_RUNTIME_OCCURRENCE_IDENTITY_CONFLICT")
        self.graph.db.execute("INSERT OR REPLACE INTO live_forward_runtime_occurrence "
            "VALUES (?,?)", (occurrence_id, canonical(payload)))

    def metrics(self) -> dict[str, Any]:
        count = lambda table, where="": int(self.graph.db.execute(
            f"SELECT COUNT(*) FROM {table} {where}").fetchone()[0])
        branch_alternatives = self.graph.db.execute(
            "SELECT COUNT(*) FROM (SELECT source_id FROM map_edge "
            "WHERE relation='EXECUTED_NEXT' GROUP BY source_id "
            "HAVING COUNT(DISTINCT target_id) > 1)"
        ).fetchone()[0]
        observed_edges = self.graph.db.execute(
            "SELECT COUNT(*) FROM map_edge WHERE relation='EXECUTED_NEXT' AND status='OBSERVED'"
        ).fetchone()[0]
        return {"segments_admitted": self.segments_admitted,
                "segments_rejected": self.segments_rejected,
                "nodes": count("map_node"), "edges": count("map_edge"),
                "observed_edges": observed_edges,
                "duplicate_structural_edges": self.duplicate_structural_edges,
                "branch_alternatives": branch_alternatives,
                "source_owned_bytes": int(self.graph.db.execute(
                    "SELECT value FROM map_meta WHERE key='source_owned_bytes'").fetchone()[0]),
                "pending_lineage_records": count("live_forward_pending_lineage")
                    if not self._lineage_finalized else 0,
                "graph_hash": self.graph.graph_hash() if self._lineage_finalized else None}

    def _finalize_lineage(self) -> None:
        if self._lineage_finalized:
            return
        cursor = self.graph.db.execute(
            "SELECT object_type, object_id, lineage_json FROM live_forward_pending_lineage "
            "ORDER BY object_type, object_id, lineage_json")
        for (object_type, object_id), rows in groupby(
                cursor, key=lambda row: (row[0], row[1])):
            lineages = [json.loads(row[2]) for row in rows]
            table, id_column = (("map_node", "node_id") if object_type == "node" else
                                ("map_edge", "edge_id"))
            update = self.graph.db.execute(
                f"UPDATE {table} SET lineage=? WHERE {id_column}=?",
                (canonical(lineages), object_id))
            if update.rowcount != 1:
                raise ValueError("STOP_CARTOGRAPHER_EXECUTION_CHAIN_LOSS")
        self.graph.db.execute("DROP TABLE live_forward_pending_lineage")
        self.graph.db.commit()
        self._lineage_finalized = True

    def save_closed(self, path: Path, expected_segments: int) -> dict[str, Any]:
        path = Path(path).resolve()
        if path.exists():
            raise FileExistsError(f"refusing to overwrite session evidence: {path}")
        if expected_segments != self.segments_admitted or self.segments_rejected:
            raise ValueError("session segment admission count does not reconcile")
        self._finalize_lineage()
        graph_hash = self.graph.graph_hash()
        closed_utc = _utc_now()
        self._meta("live_forward_session_state", "CLOSED")
        self._meta("live_forward_closed_utc", closed_utc)
        self._meta("live_forward_graph_sha256", graph_hash)
        self._meta("live_forward_occurrence_sha256", occurrence_hash(self.graph.db))
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = path.with_name(path.name + ".tmp")
        if temporary.exists():
            raise FileExistsError(f"refusing to overwrite session backup temp: {temporary}")
        target = sqlite3.connect(temporary)
        try:
            self.graph.db.backup(target)
            target.commit()
        except Exception:
            target.close()
            temporary.unlink(missing_ok=True)
            raise
        target.close()
        try:
            with temporary.open("r+b") as stream:
                os.fsync(stream.fileno())
            os.replace(temporary, path)
            saved = Cartographer(path, self.rom_sha256)
            try:
                saved_hash = saved.graph_hash()
                if saved_hash != graph_hash:
                    raise ValueError("STOP_SESSION_MAP_SAVE_MISMATCH")
                metadata = _read_meta(saved.db)
                if metadata.get("live_forward_session_state") != "CLOSED" or \
                        metadata.get("live_forward_graph_sha256") != graph_hash:
                    raise ValueError("saved session identity did not round-trip")
            finally:
                saved.close()
        except Exception:
            temporary.unlink(missing_ok=True)
            raise
        return {**self.metrics(), "session_id": self.session_id,
                "session_path": str(path), "graph_hash": graph_hash,
                "created_utc": self.created_utc, "closed_utc": closed_utc}

    def close(self) -> None:
        self.graph.close()


def _read_meta(db: sqlite3.Connection) -> dict[str, str]:
    return {str(row[0]): str(row[1]) for row in db.execute("SELECT key, value FROM map_meta")}
