"""Stage 6 control-provenance scan over one sealed FLOW artifact."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from auto67_control_provenance import analyze_flow_segment
from identity import ROM_SHA
from live_forward_scaling_audit import RECORD


def _filter_m68k(rows: Any) -> list[tuple[int, ...]]:
    res = []
    for r in rows:
        if len(r) >= 8:
            if r[7] == 0:
                res.append((r[0], r[1], r[3], r[4], r[5] & 0xFFFF, r[6], r[7]))
        else:
            res.append(r)
    return res


class ControlProvenanceStream:
    """Incremental equivalent of the legacy sealed-FLOW Stage 6 scan."""

    def __init__(self, rom: bytes, rom_sha: str = ROM_SHA):
        self.rom = rom
        self.rom_sha = rom_sha
        self.consumers: list[dict[str, Any]] = []
        self.statuses: list[str] = []
        self.segments_processed = 0
        self.records_processed = 0
        self.flow_mismatches = 0
        self.identity_conflicts = 0
        self.rom_mismatches = 0
        self.predecessor_failures = 0
        self.first_offending_consumer: dict[str, Any] | None = None

    def accept(self, segment: dict[str, Any], rows: tuple[tuple[int, ...], ...]) -> None:
        m68k_rows = _filter_m68k(rows)
        analysis = analyze_flow_segment(segment, m68k_rows, self.rom, self.rom_sha)
        self.consumers.extend(analysis.get("consumers", []))
        self.segments_processed += 1
        self.records_processed += len(rows)
        status = str(analysis.get("status", "UNKNOWN"))
        if status != "PASS_CONTROL_PROVENANCE_V1":
            self.statuses.append(status)
            if "FLOW_MISMATCH" in status:
                self.flow_mismatches += 1
            elif "IDENTITY" in status:
                self.identity_conflicts += 1
            elif "ROM" in status:
                self.rom_mismatches += 1
            else:
                self.predecessor_failures += 1
        if self.first_offending_consumer is None and analysis.get("consumers"):
            event = analysis["consumers"][0]
            occurrence = event.get("consumer_occurrence", {})
            self.first_offending_consumer = {
                "segment_id": ":".join(str(segment.get(key, "")) for key in
                                          ("run_id", "epoch", "worker_id", "capture_id", "generation")),
                "record_index": occurrence.get("record_index"), "PC": occurrence.get("pc"),
                "opcode": occurrence.get("opcode"),
                "target": event.get("target_rom_pc") or event.get("actual_next_pc"),
                "register": (event.get("required_registers") or [None])[0],
                "exact_reason": event.get("status") or event.get("classification"),
            }

    def finish(self, input_generation: str | None, identity: dict[str, Any],
               total_segments: int | None, diagnostic_path: Path | None = None) -> dict[str, Any]:
        counts = _relation_counts(self.consumers)
        base = {"status": "ERROR", "input_generation": input_generation,
                "run_id": int(identity.get("run_id", 0)),
                "raw_flow_identity": identity,
                "indirect_consumers": len(self.consumers),
                "resolved_pointer_relations": counts["pointer"],
                "resolved_offset_relations": counts["offset"],
                "resolved_jump_table_relations": counts["jump_table"],
                "unresolved": counts["unresolved"], "unsupported": counts["unsupported"],
                "segments_total": total_segments or self.segments_processed,
                "segments_processed": self.segments_processed,
                "records_processed": self.records_processed,
                "consumers_total": len(self.consumers),
                "indirect_JMP_occurrences": sum(_consumer_kind(item) == "JMP" for item in self.consumers),
                "indirect_JSR_occurrences": sum(_consumer_kind(item) == "JSR" for item in self.consumers),
                "FLOW_mismatches": self.flow_mismatches, "identity_conflicts": self.identity_conflicts,
                "ROM_mismatches": self.rom_mismatches, "predecessor_failures": self.predecessor_failures,
                "analysis_statuses": self.statuses, "first_offending_consumer": self.first_offending_consumer,
                "diagnostic_path": str(diagnostic_path) if diagnostic_path else None,
                "output_generation_or_same": input_generation, "stop_reason": None, "error": None}
        if base["FLOW_mismatches"] or base["identity_conflicts"] or base["ROM_mismatches"]:
            base.update(status="STOP", stop_reason=(self.statuses[0] if self.statuses else
                                                      "STOP_CONTROL_PROVENANCE_INTEGRITY_FAILURE"))
        elif counts["pointer"] or counts["offset"] or counts["jump_table"]:
            base["status"] = "PASS"
        else:
            base["status"] = "NO_DELTA"
        _diagnostic(base, self.consumers, self.statuses, diagnostic_path)
        return base


def _relation_counts(consumers: list[dict[str, Any]]) -> dict[str, int]:
    pointer = sum(item.get("relation") == "OBSERVED_CODE_POINTER_TO" for item in consumers)
    offset = sum(item.get("relation") == "OBSERVED_CODE_OFFSET_TO" for item in consumers)
    jump_table = sum(bool(item.get("jump_table_relation")) for item in consumers)
    unresolved = sum(not item.get("relation") and item.get("status") not in {
        "OBSERVED_RUNTIME", None} for item in consumers)
    unsupported = sum("UNSUPPORTED" in str(item.get("status", "")) or
                      "UNRESOLVED" in str(item.get("status", "")) for item in consumers)
    return {"pointer": pointer, "offset": offset, "jump_table": jump_table,
            "unresolved": unresolved, "unsupported": unsupported}


def _consumer_kind(consumer: dict[str, Any]) -> str | None:
    occurrence = consumer.get("consumer_occurrence", {})
    opcode = int(occurrence.get("opcode", -1))
    form = opcode & 0xFFC0
    return "JSR" if form == 0x4E80 else "JMP" if form == 0x4EC0 else None


def _diagnostic(base: dict[str, Any], consumers: list[dict[str, Any]],
                statuses: list[str], diagnostic_path: Path | None) -> None:
    base["diagnostic"] = {
        "schema": "oasis.m12.control-provenance-diagnostic.v1",
        "stop_code": base.get("stop_reason"),
        "stop_message": base.get("error") or base.get("stop_reason"),
        "run_id": base["run_id"],
        "canonical_generation": base["input_generation"],
        "raw_flow_path": base["raw_flow_identity"]["raw_path"],
        "raw_flow_sha256": base["raw_flow_identity"]["raw_sha256"],
        "segments_total": base["segments_total"],
        "segments_processed": base["segments_processed"],
        "records_processed": base["records_processed"],
        "indirect_JMP_occurrences": base["indirect_JMP_occurrences"],
        "indirect_JSR_occurrences": base["indirect_JSR_occurrences"],
        "consumers_total": base["consumers_total"],
        "resolved_pointer_chains": base["resolved_pointer_relations"],
        "resolved_offset_chains": base["resolved_offset_relations"],
        "resolved_jump_table_entries": base["resolved_jump_table_relations"],
        "unresolved_consumers": base["unresolved"],
        "unsupported_transforms": base["unsupported"],
        "FLOW_mismatches": base["FLOW_mismatches"],
        "identity_conflicts": base["identity_conflicts"],
        "ROM_mismatches": base["ROM_mismatches"],
        "predecessor_failures": base["predecessor_failures"],
        "analysis_statuses": statuses,
        "first_offending_consumer": base.get("first_offending_consumer"),
    }
    if diagnostic_path is not None:
        diagnostic_path.parent.mkdir(parents=True, exist_ok=True)
        diagnostic_path.write_text(json.dumps(base["diagnostic"], indent=2,
                                               sort_keys=True) + "\n",
                                   encoding="utf-8")


def run_control_provenance(receipt: dict[str, Any], rom_path: Path,
                           input_generation: str | None, total_segments: int | None,
                           progress: Any, diagnostic_path: Path | None = None) -> dict[str, Any]:
    """Scan the exact receipt-owned FLOW path and return the Stage 6 contract."""
    spool = receipt.get("raw_segment_spool") or receipt.get("flow_handoff", {})
    runtime = receipt.get("runtime", {})
    run_id = int(runtime.get("run_id", 0))
    raw_path = Path(str(spool.get("raw_path", "")))
    index_path = Path(str(spool.get("index_path", "")))
    identity = {"run_id": run_id, "raw_path": str(raw_path),
                "raw_sha256": spool.get("raw_sha256"),
                "index_path": str(index_path), "index_sha256": spool.get("index_sha256"),
                "segments": int(spool.get("segments", 0))}
    base = {"status": "ERROR", "input_generation": input_generation,
            "run_id": run_id, "raw_flow_identity": identity,
            "indirect_consumers": 0, "resolved_pointer_relations": 0,
            "resolved_offset_relations": 0, "resolved_jump_table_relations": 0,
            "unresolved": 0, "unsupported": 0,
            "segments_total": total_segments or identity["segments"],
            "segments_processed": 0, "records_processed": 0,
            "consumers_total": 0, "indirect_JMP_occurrences": 0,
            "indirect_JSR_occurrences": 0, "FLOW_mismatches": 0,
            "identity_conflicts": 0, "ROM_mismatches": 0,
            "predecessor_failures": 0, "analysis_statuses": [],
            "first_offending_consumer": None,
            "diagnostic_path": str(diagnostic_path) if diagnostic_path else None,
            "output_generation_or_same": input_generation,
            "stop_reason": None, "error": None}
    consumers: list[dict[str, Any]] = []
    try:
        if not raw_path.is_file() or not index_path.is_file():
            archive = raw_path.parent.parent / "raw-evidence-archive"
            if (archive / raw_path.name).is_file() and (archive / index_path.name).is_file():
                raw_path, index_path = archive / raw_path.name, archive / index_path.name
        if run_id <= 0 or not raw_path.is_file() or not index_path.is_file():
            raise ValueError("STOP_CONTROL_PROVENANCE_FLOW_ARTIFACT_MISSING")
        rom = rom_path.read_bytes()
        if len(rom) == 0 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
            base["ROM_mismatches"] = 1
            raise ValueError("STOP_CONTROL_PROVENANCE_ROM_IDENTITY_MISMATCH")
        expected = total_segments or identity["segments"] or None
        processed = 0
        with index_path.open("r", encoding="utf-8") as stream, raw_path.open("rb") as raw:
            for line in stream:
                item = json.loads(line)
                raw.seek(int(item["raw_offset"]))
                data = raw.read(int(item["raw_length"]))
                rows = list(RECORD.iter_unpack(data))
                m68k_rows = _filter_m68k(rows)
                analysis = analyze_flow_segment(item["segment"], m68k_rows, rom, ROM_SHA)
                consumers.extend(analysis.get("consumers", []))
                processed += 1
                base["segments_processed"] = processed
                base["records_processed"] += len(rows)
                status = str(analysis.get("status", "UNKNOWN"))
                if status != "PASS_CONTROL_PROVENANCE_V1":
                    base["analysis_statuses"].append(status)
                    if "FLOW_MISMATCH" in status or status in {
                            "STOP_PREDECESSOR_GAP", "STOP_CONSUMER_OCCURRENCE_IDENTITY_MISMATCH"}:
                        base["FLOW_mismatches"] += 1
                    elif "IDENTITY" in status:
                        base["identity_conflicts"] += 1
                    elif "ROM" in status:
                        base["ROM_mismatches"] += 1
                    else:
                        base["predecessor_failures"] += 1
                if base["first_offending_consumer"] is None and analysis.get("consumers"):
                    event = analysis["consumers"][0]
                    occurrence = event.get("consumer_occurrence", {})
                    base["first_offending_consumer"] = {
                        "segment_id": ":".join(str(item["segment"].get(key, "")) for key in
                                                  ("run_id", "epoch", "worker_id", "capture_id", "generation")),
                        "record_index": occurrence.get("record_index"),
                        "PC": occurrence.get("pc"), "opcode": occurrence.get("opcode"),
                        "target": event.get("target_rom_pc") or event.get("actual_next_pc"),
                        "register": (event.get("required_registers") or [None])[0],
                        "exact_reason": event.get("status") or event.get("classification"),
                    }
                if processed % 128 == 0:
                    progress.update(processed, total=expected,
                                    detail=f"processed {processed:,} / {expected:,} FLOW segments"
                                    if expected else f"processed {processed:,} FLOW segments")
                    progress.heartbeat("scanning sealed ordered FLOW")
        progress.update(processed, total=expected,
                        detail=f"processed {processed:,} / {expected:,} FLOW segments"
                        if expected else f"processed {processed:,} FLOW segments")
        progress.heartbeat("sealed FLOW scan complete; publishing Stage 6 result")
        counts = _relation_counts(consumers)
        base["consumers_total"] = len(consumers)
        base["indirect_JMP_occurrences"] = sum(_consumer_kind(item) == "JMP" for item in consumers)
        base["indirect_JSR_occurrences"] = sum(_consumer_kind(item) == "JSR" for item in consumers)
        base.update(indirect_consumers=len(consumers),
                    resolved_pointer_relations=counts["pointer"],
                    resolved_offset_relations=counts["offset"],
                    resolved_jump_table_relations=counts["jump_table"],
                    unresolved=counts["unresolved"], unsupported=counts["unsupported"])
        if base["FLOW_mismatches"] or base["identity_conflicts"] or base["ROM_mismatches"]:
            reason = next((item for item in base["analysis_statuses"] if
                           "FLOW_MISMATCH" in item or "IDENTITY" in item or "ROM" in item),
                          "STOP_CONTROL_PROVENANCE_INTEGRITY_FAILURE")
            base.update(status="STOP", stop_reason=reason)
        elif counts["pointer"] or counts["offset"] or counts["jump_table"]:
            base.update(status="PASS", stop_reason=None)
        else:
            base.update(status="NO_DELTA", stop_reason=None)
        _diagnostic(base, consumers, base["analysis_statuses"], diagnostic_path)
        return base
    except Exception as error:  # The coordinator publishes ERROR and terminalizes.
        base["error"] = f"{type(error).__name__}: {error}"
        base["stop_reason"] = str(error)
        _diagnostic(base, consumers, base["analysis_statuses"], diagnostic_path)
        return base
