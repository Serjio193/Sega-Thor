#!/usr/bin/env python3
"""Independently verify saved FLOW_V1-to-ROM claims against raw records and ROM."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import subprocess

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
RECORD = struct.Struct("<QQIIHHI")
INSTRUCTION = 1


def _canonical(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _digest(value: object) -> str:
    return hashlib.sha256(_canonical(value).encode("utf-8")).hexdigest()


def _readonly(path: Path) -> sqlite3.Connection:
    return sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)


def _decode(decoder: Path, rom: Path, pcs: set[int], folder: Path) -> dict[int, dict[str, object]]:
    source, result = folder / "audit-pcs.txt", folder / "audit-decode.tsv"
    source.write_text("".join(f"0x{pc:08x}\n" for pc in sorted(pcs)), encoding="ascii")
    process = subprocess.run([str(decoder.resolve()), str(rom.resolve()), str(source), str(result)],
        capture_output=True, text=True, check=False)
    if process.returncode:
        raise ValueError("STOP_ROM_LINK_INDEPENDENT_DECODER_FAILED: " + process.stderr[-2000:])
    decoded: dict[int, dict[str, object]] = {}
    for line in result.read_text(encoding="ascii").splitlines():
        fields = line.split("\t")
        if len(fields) != 9:
            raise ValueError("STOP_ROM_LINK_INDEPENDENT_DECODER_OUTPUT_INVALID")
        decoded[int(fields[0], 0)] = {"cpu_address": int(fields[1], 0),
            "memory_region": fields[2], "rom_offset": int(fields[3], 0) if fields[3] else None,
            "rom_sha256": fields[4], "status": fields[5],
            "opcode": int(fields[6], 0) if fields[6] else None,
            "length": int(fields[7]), "bytes": bytes.fromhex(fields[8])}
    if decoded.keys() != pcs:
        raise ValueError("STOP_ROM_LINK_INDEPENDENT_DECODER_PC_SET_MISMATCH")
    return decoded


def audit(session: Path, rom_path: Path, raw_path: Path, index_path: Path,
          export_path: Path, decoder: Path, output: Path) -> dict[str, object]:
    output.parent.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_ROM_LINK_AUDIT_ROM_IDENTITY_MISMATCH")
    raw = raw_path.read_bytes()
    segments: dict[tuple, tuple[dict, bytes]] = {}
    expected_instructions: set[tuple] = set()
    expected_terminals: dict[tuple, tuple[int, tuple[int, ...]]] = {}
    segment_slots = set()
    capture_ids = set()
    worker_generations = set()
    worker_segment_counts: dict[int, int] = {}
    raw_spans = []
    for line in index_path.read_text(encoding="utf-8").splitlines():
        item = json.loads(line)
        segment = item["segment"]
        key = (int(segment["run_id"]), int(segment["epoch"]), int(segment["worker_id"]),
               int(segment["capture_id"]), int(segment["generation"]),
               str(segment["segment_sha256"]))
        slot = key[:-1]
        if key in segments or slot in segment_slots:
            raise ValueError("STOP_ROM_LINK_AUDIT_DUPLICATE_SEGMENT_IDENTITY")
        capture_id = int(segment["capture_id"])
        generation_key = key[:3] + (key[4],)
        if capture_id in capture_ids or generation_key in worker_generations:
            raise ValueError("STOP_ROM_LINK_AUDIT_REUSED_CAPTURE_OR_GENERATION")
        start, length = int(item["raw_offset"]), int(item["raw_length"])
        blob = raw[start:start + length]
        if len(blob) != length or hashlib.sha256(blob).hexdigest() != item["raw_sha256"] or \
                item["raw_sha256"] != segment["records_sha256"] or \
                len(blob) != int(segment["record_count"]) * RECORD.size:
            raise ValueError("STOP_ROM_LINK_AUDIT_RAW_SEGMENT_MISMATCH")
        segments[key] = (item, blob)
        segment_slots.add(slot)
        capture_ids.add(capture_id)
        worker_generations.add(generation_key)
        worker_segment_counts[key[2]] = worker_segment_counts.get(key[2], 0) + 1
        raw_spans.append((start, start + length))
        last_instruction = None
        for record_index, row in enumerate(RECORD.iter_unpack(blob)):
            if row[5] & INSTRUCTION:
                occurrence = key + (record_index,)
                if occurrence in expected_instructions:
                    raise ValueError("STOP_ROM_LINK_AUDIT_DUPLICATE_RAW_INSTRUCTION")
                expected_instructions.add(occurrence)
                last_instruction = (record_index, row)
        if last_instruction is not None:
            expected_terminals[key] = last_instruction
    cursor = 0
    for start, end in sorted(raw_spans):
        if start != cursor or end < start:
            raise ValueError("STOP_ROM_LINK_AUDIT_RAW_SEGMENT_OFFSETS_INVALID")
        cursor = end
    if cursor != len(raw) or not expected_instructions:
        raise ValueError("STOP_ROM_LINK_AUDIT_RAW_SEGMENT_COVERAGE_INVALID")

    exported = json.loads(export_path.read_text(encoding="utf-8"))
    projection_decoder_sha = str(exported.get("decoder_sha256", ""))
    if exported.get("schema") != "oasis.m12.live-forward-rom-ranges.v1" or \
            exported.get("interval_convention") != "[start_offset,end_offset_exclusive)" or \
            exported.get("rom_sha256") != ROM_SHA or len(projection_decoder_sha) != 64 or \
            any(char not in "0123456789abcdef" for char in projection_decoder_sha) or \
            not isinstance(exported.get("ranges"), list) or \
            not isinstance(exported.get("metrics"), dict):
        raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_ROM_MISMATCH")
    audit_decoder_sha = hashlib.sha256(decoder.read_bytes()).hexdigest()

    db = _readonly(session)
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_ROM_LINK_AUDIT_SQLITE_INTEGRITY")
        metadata = dict(db.execute("SELECT key,value FROM map_meta"))
        if metadata.get("rom_sha256") != ROM_SHA or metadata.get("source_owned_bytes") != "0":
            raise ValueError("STOP_ROM_LINK_AUDIT_MAP_IDENTITY_OR_OWNERSHIP")
        range_edges = []
        unresolved_edges = []
        next_edges = []
        for edge in db.execute("SELECT edge_id,source_id,target_id,relation,scope,status,body,lineage "
                                "FROM map_edge ORDER BY edge_id"):
            edge_id, source_id, target_id, relation, scope, status, body_text, lineage_text = edge
            body, lineages = json.loads(body_text), json.loads(lineage_text)
            if relation == "EXECUTED_FROM_ROM":
                if status != "OBSERVED" or scope != ROM_SHA or not lineages or \
                        body.get("relation") != relation or body.get("scope") != scope or \
                        body.get("source") != source_id or body.get("target") != target_id:
                    raise ValueError("STOP_ROM_LINK_AUDIT_RANGE_EDGE_STATUS")
                range_edges.extend((edge_id, source_id, target_id, lineage) for lineage in lineages)
            elif relation == "OBSERVED_NEXT_PC":
                if status != "OBSERVED" or scope != ROM_SHA or not lineages or \
                        body.get("relation") != relation or body.get("scope") != scope or \
                        body.get("source") != source_id or body.get("target") != target_id:
                    raise ValueError("STOP_ROM_LINK_AUDIT_NEXT_PC_STATUS")
                next_edges.extend((source_id, target_id, lineage) for lineage in lineages)
            elif relation == "ROM_LINK_UNRESOLVED":
                if status != "UNRESOLVED" or scope != ROM_SHA or not lineages or \
                        body.get("relation") != relation or body.get("scope") != scope or \
                        body.get("source") != source_id or body.get("target") != target_id:
                    raise ValueError("STOP_ROM_LINK_AUDIT_UNRESOLVED_EDGE_STATUS")
                target = db.execute("SELECT kind,body FROM map_node WHERE node_id=?", (target_id,)).fetchone()
                if not target or target[0] != "ROM_LINK_UNRESOLVED":
                    raise ValueError("STOP_ROM_LINK_AUDIT_FALSE_UNRESOLVED_TARGET")
                unresolved_edges.extend((edge_id, source_id, target_id, lineage) for lineage in lineages)

        def segment_key(lineage: dict) -> tuple:
            return (int(lineage["run_id"]), int(lineage["epoch"]), int(lineage["worker_id"]),
                    int(lineage["capture_id"]), int(lineage["generation"]),
                    str(lineage["segment_sha256"]))

        claimed_instructions = set()
        pcs: set[int] = set()
        def raw_occurrence(lineage: dict) -> tuple[tuple, dict, tuple[int, ...]]:
            identity = segment_key(lineage)
            record_index = int(lineage["record_index"])
            if identity not in segments or identity + (record_index,) not in expected_instructions:
                raise ValueError("STOP_ROM_LINK_AUDIT_LINEAGE_SEGMENT_OR_RECORD_MISSING")
            item, blob = segments[identity]
            if record_index < 0 or record_index >= int(item["segment"]["record_count"]):
                raise ValueError("STOP_ROM_LINK_AUDIT_RECORD_INDEX_INVALID")
            row = RECORD.unpack_from(blob, record_index * RECORD.size)
            if not row[5] & INSTRUCTION:
                raise ValueError("STOP_ROM_LINK_AUDIT_NON_INSTRUCTION_LINEAGE")
            expected = {"stream_sequence": row[0], "instruction_sequence": row[1],
                "raw_cpu_pc": row[2], "flow_opcode": row[4], "run_id": identity[0],
                "epoch": identity[1], "worker_id": identity[2], "capture_id": identity[3],
                "generation": identity[4], "segment_sha256": identity[5],
                "record_index": record_index,
                "raw_records_offset": int(item["raw_offset"]) + record_index * RECORD.size}
            for field, value in expected.items():
                if lineage.get(field) != value:
                    raise ValueError("STOP_ROM_LINK_AUDIT_FLOW_RECORD_MISMATCH")
            for field, value in (("records_sha256", item["raw_sha256"]),
                                 ("flags", row[5]), ("auxiliary", row[6])):
                if field in lineage and lineage[field] != value:
                    raise ValueError("STOP_ROM_LINK_AUDIT_FLOW_RECORD_METADATA_MISMATCH")
            return identity, item, row

        for relation_rows in (range_edges, unresolved_edges):
            for _, _, _, lineage in relation_rows:
                identity, _, row = raw_occurrence(lineage)
                occurrence = identity + (int(lineage["record_index"]),)
                if occurrence in claimed_instructions:
                    raise ValueError("STOP_ROM_LINK_AUDIT_DUPLICATE_INSTRUCTION_LINEAGE")
                claimed_instructions.add(occurrence)
                pcs.add(row[2])
        if claimed_instructions != expected_instructions:
            raise ValueError("STOP_ROM_LINK_AUDIT_INSTRUCTION_COVERAGE_MISMATCH")
        independently_decoded = _decode(decoder, rom_path, pcs, output.parent)

        node_cache = {}
        def node(node_id: str) -> tuple | None:
            if node_id not in node_cache:
                node_cache[node_id] = db.execute(
                    "SELECT kind,node_key,scope,status,body FROM map_node WHERE node_id=?",
                    (node_id,)).fetchone()
            return node_cache[node_id]

        by_worker: dict[int, list[tuple]] = {}
        chain_by_segment: dict[tuple, list[tuple]] = {}
        intervals = set()
        edge_evidence: dict[str, int] = {}
        range_identity: dict[str, tuple] = {}
        length_counts: dict[int, int] = {}
        opcode_mismatches = identity_conflicts = unsupported = 0
        for edge_id, source_id, target_id, lineage in range_edges:
            identity, _, row = raw_occurrence(lineage)
            stream, instruction, pc, next_pc, opcode, _, _ = row
            worker = int(lineage["worker_id"])
            by_worker.setdefault(worker, []).append((int(lineage["capture_id"]), stream,
                edge_id, source_id, target_id, lineage))
            decoded = independently_decoded[pc]
            if decoded["status"] != "DECODED" or decoded["memory_region"] != "ROM" or \
                    decoded["rom_sha256"] != ROM_SHA:
                unsupported += decoded["status"] != "DECODED"
                raise ValueError("STOP_ROM_LINK_AUDIT_RESOLUTION_OR_DECODE_MISMATCH")
            offset, length, bytecode = int(decoded["rom_offset"]), int(decoded["length"]), decoded["bytes"]
            end = offset + length
            if decoded["cpu_address"] != (pc & 0xFFFFFF) or offset != lineage.get("rom_offset") or \
                    decoded["opcode"] != opcode or opcode != int.from_bytes(bytecode[:2], "big"):
                opcode_mismatches += decoded["opcode"] != opcode or opcode != int.from_bytes(bytecode[:2], "big")
                raise ValueError("STOP_ROM_LINK_AUDIT_RESOLUTION_OR_OPCODE_MISMATCH")
            if offset < 0 or length < 2 or length % 2 or end > len(rom) or \
                    lineage.get("memory_region") != "ROM" or lineage.get("rom_sha256") != ROM_SHA or \
                    lineage.get("cpu_address") != decoded["cpu_address"] or \
                    lineage.get("instruction_length") != length or \
                    lineage.get("instruction_bytes_sha256") != hashlib.sha256(bytecode).hexdigest() or \
                    lineage.get("decoder_sha256") != projection_decoder_sha or \
                    lineage.get("range_node_id") != target_id:
                raise ValueError("STOP_ROM_LINK_AUDIT_LINEAGE_RANGE_METADATA_MISMATCH")
            target = node(target_id)
            key_text = f"{offset:08X}:{end:08X}:{bytecode.hex().upper()}"
            canonical_id = _digest({"kind": "ROM_INSTRUCTION_RANGE", "key": key_text, "scope": ROM_SHA})
            if not target or target[0] != "ROM_INSTRUCTION_RANGE" or target[1] != key_text or \
                    target[2] != ROM_SHA or target[3] != "OBSERVED" or target_id != canonical_id:
                raise ValueError("STOP_ROM_LINK_AUDIT_RANGE_NODE_MISSING_OR_IDENTITY_INVALID")
            attrs = json.loads(target[4]).get("attributes", {})
            identity_value = (offset, end, bytecode, hashlib.sha256(bytecode).hexdigest(), opcode)
            old_identity = range_identity.get(target_id)
            if old_identity is not None and old_identity != identity_value:
                identity_conflicts += 1
                raise ValueError("STOP_ROM_LINK_AUDIT_RANGE_IDENTITY_CONFLICT")
            range_identity[target_id] = identity_value
            if attrs.get("rom_sha256") != ROM_SHA or attrs.get("start_offset") != offset or \
                    attrs.get("end_offset_exclusive") != end or attrs.get("length") != length or \
                    attrs.get("opcode") != opcode or attrs.get("bytes_hex") != bytecode.hex().upper() or \
                    attrs.get("bytes_sha256") != identity_value[3] or rom[offset:end] != bytecode:
                raise ValueError("STOP_ROM_LINK_AUDIT_SAVED_RANGE_MISMATCH")
            source = node(source_id)
            if not source or source[0] != "M68K_INSTRUCTION" or \
                    source[1] != f"{pc:08X}:{opcode:04X}" or source[2] != "rom:" + ROM_SHA:
                raise ValueError("STOP_ROM_LINK_AUDIT_SOURCE_NODE_MISMATCH")
            if lineage.get("flow_opcode") != opcode:
                opcode_mismatches += 1
                raise ValueError("STOP_ROM_LINK_AUDIT_FLOW_OPCODE_MISMATCH")
            edge_evidence[target_id] = edge_evidence.get(target_id, 0) + 1
            intervals.add((offset, end))
            length_counts[length] = length_counts.get(length, 0) + 1
            claim = (int(lineage["record_index"]), edge_id, target_id, lineage, row, decoded)
            chain = chain_by_segment.setdefault(identity, [])
            chain.append(claim)
            chain.sort(key=lambda value: value[0])
            if len(chain) > 3:
                chain.pop()

        nonrom_occurrences = unresolved_rom_occurrences = 0
        for _, source_id, target_id, lineage in unresolved_edges:
            identity, _, row = raw_occurrence(lineage)
            pc, opcode = row[2], row[4]
            decoded = independently_decoded[pc]
            target = node(target_id)
            if not target or target[0] != "ROM_LINK_UNRESOLVED" or target[2] != ROM_SHA or \
                    target[3] != "UNRESOLVED":
                raise ValueError("STOP_ROM_LINK_AUDIT_FALSE_UNRESOLVED_TARGET")
            attrs = json.loads(target[4]).get("attributes", {})
            if decoded["cpu_address"] != (pc & 0xFFFFFF) or \
                    attrs.get("raw_cpu_pc") != pc or attrs.get("memory_region") != decoded["memory_region"] or \
                    lineage.get("cpu_address") != decoded["cpu_address"] or \
                    lineage.get("memory_region") != decoded["memory_region"] or \
                    attrs.get("reason") != lineage.get("reason"):
                raise ValueError("STOP_ROM_LINK_AUDIT_UNRESOLVED_CLASSIFICATION_MISMATCH")
            source = node(source_id)
            if not source or source[0] != "M68K_INSTRUCTION" or \
                    source[1] != f"{pc:08X}:{opcode:04X}" or source[2] != "rom:" + ROM_SHA:
                raise ValueError("STOP_ROM_LINK_AUDIT_UNRESOLVED_SOURCE_MISMATCH")
            if decoded["memory_region"] == "ROM":
                if decoded["status"] == "DECODED":
                    raise ValueError("STOP_ROM_LINK_AUDIT_DECODABLE_ROM_LEFT_UNRESOLVED")
                unresolved_rom_occurrences += 1
                unsupported += decoded["status"] == "DECODE_UNSUPPORTED"
            else:
                nonrom_occurrences += 1

        next_seen = set()
        terminal_by_worker = {}
        for source_id, target_id, lineage in next_edges:
            identity, item, row = raw_occurrence(lineage)
            terminal = expected_terminals.get(identity)
            record_index = int(lineage["record_index"])
            occurrence = identity + (record_index,)
            if not terminal or terminal[0] != record_index or \
                    record_index != terminal[0] or row[3] != lineage.get("raw_next_pc"):
                raise ValueError("STOP_ROM_LINK_AUDIT_TERMINAL_NEXT_PC_MISMATCH")
            if occurrence in next_seen:
                raise ValueError("STOP_ROM_LINK_AUDIT_DUPLICATE_TERMINAL_NEXT_PC")
            next_seen.add(occurrence)
            target = node(target_id)
            source = node(source_id)
            if not target or target[0] != "M68K_TARGET_ADDRESS" or \
                    target[2] != "rom:" + ROM_SHA or target[3] != "OBSERVED" or \
                    json.loads(target[4])["attributes"].get("raw_next_pc") != row[3] or \
                    not source or source[0] != "M68K_INSTRUCTION" or \
                    source[1] != f"{row[2]:08X}:{row[4]:04X}" or source[2] != "rom:" + ROM_SHA:
                raise ValueError("STOP_ROM_LINK_AUDIT_TERMINAL_NEXT_PC_TARGET_INVALID")
            terminal_by_worker.setdefault(int(lineage["worker_id"]), {
                "worker_id": int(lineage["worker_id"]), "capture_id": int(lineage["capture_id"]),
                "raw_next_pc": row[3], "target_kind": target[0]})
        expected_terminal_occurrences = {key + (index,) for key, (index, _) in expected_terminals.items()}
        if next_seen != expected_terminal_occurrences:
            raise ValueError("STOP_ROM_LINK_AUDIT_TERMINAL_NEXT_PC_COVERAGE")

        range_ids = {row[0] for row in db.execute(
            "SELECT node_id FROM map_node WHERE kind='ROM_INSTRUCTION_RANGE'")}
        exported_ids = {row["range_node_id"] for row in exported["ranges"]}
        if range_ids != exported_ids or len(exported_ids) != len(exported["ranges"]) or \
                range_ids != set(range_identity):
            raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_RANGE_SET_MISMATCH")
        export_evidence = {row["range_node_id"]: row["evidence_count"] for row in exported["ranges"]}
        if edge_evidence != export_evidence:
            raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_EVIDENCE_COUNT_MISMATCH")
        for row in exported["ranges"]:
            target = node(row["range_node_id"])
            attrs = json.loads(target[4]).get("attributes", {})
            expected = {"rom_sha256": ROM_SHA, "start_offset": attrs.get("start_offset"),
                "end_offset_exclusive": attrs.get("end_offset_exclusive"),
                "length": attrs.get("length"), "opcode": attrs.get("opcode"),
                "bytes_hex": attrs.get("bytes_hex"), "bytes_sha256": attrs.get("bytes_sha256")}
            if any(row.get(field) != value for field, value in expected.items()):
                raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_RANGE_ROW_MISMATCH")
        merged_intervals = []
        for start, end in sorted(intervals):
            if merged_intervals and start <= merged_intervals[-1][1]:
                merged_intervals[-1] = (merged_intervals[-1][0], max(merged_intervals[-1][1], end))
            else:
                merged_intervals.append((start, end))
        unique_bytes = sum(end - start for start, end in merged_intervals)
        export_metrics = exported["metrics"]
        expected_metrics = {"instruction_occurrences": len(range_edges),
            "observed_executed_rom_bytes": sum(length * count
                for length, count in length_counts.items()),
            "unique_instruction_ranges": len(range_ids),
            "unique_instruction_bytes": unique_bytes,
            "unresolved_instruction_occurrences": len(unresolved_edges),
            "unsupported_decode_occurrences": unsupported,
            "terminal_next_pc_facts": len(next_seen)}
        if any(export_metrics.get(field) != value for field, value in expected_metrics.items()):
            raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_METRICS_MISMATCH")
        expected_status = "STOP_ROM_LINK_DECODE_UNSUPPORTED" if unsupported else \
            "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE"
        if exported.get("status") != expected_status:
            raise ValueError("STOP_ROM_LINK_AUDIT_EXPORT_STATUS_MISMATCH")

        def sample(edge_id: str, target_id: str, lineage: dict, row: tuple,
                   decoded: dict) -> dict[str, object]:
            offset, length = int(decoded["rom_offset"]), int(decoded["length"])
            bytecode = decoded["bytes"]
            return {"run_id": int(lineage["run_id"]), "epoch": int(lineage["epoch"]),
                "worker_id": int(lineage["worker_id"]), "capture_id": int(lineage["capture_id"]),
                "generation": int(lineage["generation"]), "segment_sha256": lineage["segment_sha256"],
                "record_index": int(lineage["record_index"]), "instruction_sequence": row[1],
                "pc": row[2], "next_pc": row[3], "opcode": row[4], "rom_offset": offset,
                "rom_start": offset, "rom_end_exclusive": offset + length, "length": length,
                "bytes_hex": bytecode.hex().upper(), "bytes_sha256": hashlib.sha256(bytecode).hexdigest(),
                "range_node_id": target_id, "edge_id": edge_id}

        checked = []
        for worker in sorted(by_worker):
            candidates = sorted(by_worker[worker], key=lambda value: (value[0], value[1], value[2]))
            chosen = candidates[:1]
            if len(candidates) > 2:
                chosen += [candidates[len(candidates) // 2], candidates[-1]]
            for _, _, edge_id, _, target_id, lineage in chosen:
                _, _, row = raw_occurrence(lineage)
                checked.append(sample(edge_id, target_id, lineage, row,
                                      independently_decoded[row[2]]))
        chain_identity = next((key for key in sorted(chain_by_segment)
                               if len(chain_by_segment[key]) == 3), None)
        chain_sample = None
        if chain_identity is not None:
            records = []
            for _, edge_id, target_id, lineage, row, decoded in chain_by_segment[chain_identity]:
                records.append(sample(edge_id, target_id, lineage, row, decoded))
            chain_sample = {"run_id": chain_identity[0], "epoch": chain_identity[1],
                "worker_id": chain_identity[2], "capture_id": chain_identity[3],
                "generation": chain_identity[4], "segment_sha256": chain_identity[5],
                "records": records}
        total_claims = len(range_edges) + len(unresolved_edges)
        if total_claims != len(expected_instructions):
            raise ValueError("STOP_ROM_LINK_AUDIT_INSTRUCTION_COVERAGE_MISMATCH")
        result = {"status": "PASS_INDEPENDENT_ROM_RANGE_AUDIT", "rom_sha256": ROM_SHA,
            "segments_audited": len(segments), "instruction_occurrences_inspected": len(expected_instructions),
            "rom_linked_occurrences": len(range_edges), "nonrom_occurrences": nonrom_occurrences,
            "unresolved_instruction_occurrences": len(unresolved_edges),
            "unique_instruction_ranges": len(range_ids), "unique_executed_rom_bytes": unique_bytes,
            "two_byte_instructions": length_counts.get(2, 0),
            "extended_instructions": sum(count for length, count in length_counts.items() if length > 2),
            "multiple_extension_byte_instructions": sum(
                count for length, count in length_counts.items() if length >= 6),
            "instruction_length_counts": {str(length): count for length, count in sorted(length_counts.items())},
            "terminal_next_pc_facts": len(next_seen), "identity_conflicts": identity_conflicts,
            "opcode_mismatches": opcode_mismatches, "decode_unsupported_occurrences": unsupported,
            "unique_capture_ids": len(capture_ids),
            "worker_segment_counts": {str(worker): count
                                       for worker, count in sorted(worker_segment_counts.items())},
            "worker_generation_counts": {str(worker): sum(1 for key in worker_generations
                                             if key[2] == worker)
                                         for worker in sorted(worker_segment_counts)},
            "audited_range_occurrences": len(range_edges),
            "audited_terminal_next_pc_facts": len(next_seen), "workers_represented": sorted(by_worker),
            "range_count": len(range_ids), "nonrom_unresolved_occurrences": nonrom_occurrences,
            "rom_unresolved_occurrences": unresolved_rom_occurrences,
            "source_owned_bytes": int(metadata["source_owned_bytes"]),
            "projection_decoder_sha256": projection_decoder_sha,
            "audit_decoder_sha256": audit_decoder_sha,
            "range_samples": checked, "terminal_samples": list(terminal_by_worker.values()),
            "chain_sample": chain_sample}
    finally:
        db.close()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("session", "rom", "raw-records", "segments", "ranges", "decoder", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.session, args.rom, args.raw_records, args.segments,
        args.ranges, args.decoder, args.output), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
