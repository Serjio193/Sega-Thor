#!/usr/bin/env python3
"""Deterministic fixtures for exact FLOW_V1-to-ROM range projection."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(EVIDENCE))
sys.path.insert(0, str(EVIDENCE.parent))
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
import live_forward_rom_link as linkage
import live_forward_rom_link_audit as independent_audit
from live_forward_cartographer import LiveForwardCartographer


PACK = independent_audit.RECORD


def _require(value: bool, message: str) -> None:
    if not value:
        raise AssertionError(message)


def _resolution(cpu: int, region: str, offset: int | None, status: str,
                opcode: int | None, data: bytes) -> dict:
    return {"cpu_address": cpu & 0xFFFFFF, "memory_region": region, "rom_offset": offset,
            "rom_sha256": linkage.ROM_SHA if region == "ROM" else "",
            "decode_status": status, "opcode": opcode, "length": len(data), "bytes": data}


def _segment(rows: list[tuple[int, ...]], worker: int = 0, capture: int = 1) -> tuple[dict, bytes]:
    data = b"".join(PACK.pack(*row) for row in rows)
    records_sha = hashlib.sha256(data).hexdigest()
    segment_sha = hashlib.sha256(f"{worker}:{capture}:".encode() + data).hexdigest()
    segment = {"valid": True, "ready_for_cartographer": True, "run_id": 99, "epoch": 1,
        "worker_id": worker, "capture_id": capture, "generation": capture,
        "entry_stream_sequence": rows[0][0], "exit_stream_sequence": rows[-1][0] + 1,
        "entry_instruction_sequence": rows[0][1], "exit_instruction_sequence": rows[-1][1] + 1,
        "record_count": len(rows), "configured_depth": 20,
        "segment_sha256": segment_sha, "records_sha256": records_sha}
    return segment, data


def _graph(rows_by_worker: list[tuple[int, list[tuple[int, ...]]]]) -> tuple[LiveForwardCartographer,
                                                                              LiveForwardRomLinker]:
    session = LiveForwardCartographer(linkage.ROM_SHA, "a" * 64)
    linker = linkage.LiveForwardRomLinker(linkage.ROM_SHA)
    for worker, rows in rows_by_worker:
        segment, data = _segment(rows, worker, worker + 1)
        session.admit(segment, rows, data)
        linker.stage(segment, rows, data)
    return session, linker


def main() -> None:
    old_sha, old_size = linkage.ROM_SHA, linkage.ROM_SIZE
    old_decode = linkage.LiveForwardRomLinker._decode_batch
    old_audit_decode = independent_audit._decode
    old_audit_sha, old_audit_size = independent_audit.ROM_SHA, independent_audit.ROM_SIZE
    rom = bytes.fromhex("4e7148e73030f000")
    linkage.ROM_SHA, linkage.ROM_SIZE = hashlib.sha256(rom).hexdigest(), len(rom)
    decode_table = {
        0: _resolution(0, "ROM", 0, "DECODED", 0x4E71, rom[:2]),
        2: _resolution(2, "ROM", 2, "DECODED", 0x48E7, rom[2:6]),
        6: _resolution(6, "ROM", 6, "DECODE_UNSUPPORTED", 0xF000, rom[6:8]),
        0xFF0000: _resolution(0xFF0000, "MAIN_RAM", None, "NOT_ROM_BACKED", None, b""),
    }
    linkage.LiveForwardRomLinker._decode_batch = staticmethod(
        lambda decoder, rom_path, pcs, output: {pc: decode_table[pc] for pc in pcs})
    try:
        with tempfile.TemporaryDirectory(prefix="rom-link-tests-") as temporary:
            root = Path(temporary)
            rom_path, decoder = root / "rom.bin", root / "decoder.exe"
            rom_path.write_bytes(rom)
            decoder.write_bytes(b"synthetic decoder identity")
            rows_a = [(1, 1, 0, 0, 2, 0x4E71, 1, 0, 0, 0, 0, 0),
                      (2, 2, 0, 2, 0x20, 0x48E7, 1, 0, 0, 0, 0, 0)]
            rows_b = [(3, 3, 0, 0, 2, 0x4E71, 1, 0, 0, 0, 0, 0)]
            session, linker = _graph([(0, rows_a), (1, rows_b)])
            result = linker.project(session.graph, rom_path, decoder, root / "first")
            _require(result["status"] == "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE",
                     "supported 2-byte/extension-word projection did not pass")
            _require(result["observed_executed_rom_bytes"] == 8 and
                     result["unique_instruction_ranges"] == 2 and
                     result["unique_instruction_bytes"] == 6,
                     "occurrence and unique-byte metrics were conflated")
            export_path = Path(result["ranges_path"])
            exported = json.loads(export_path.read_text(encoding="utf-8"))
            by_offset = {row["start_offset"]: row for row in exported["ranges"]}
            _require(by_offset[0]["length"] == 2 and by_offset[2]["length"] == 4,
                     "full decoded instruction lengths were not retained")
            _require(by_offset[0]["evidence_count"] == 2,
                     "repeated and overlapping occurrences did not share one range")
            db = session.graph.db
            _require(db.execute("SELECT COUNT(*) FROM map_node WHERE kind='ROM_INSTRUCTION_RANGE'")
                     .fetchone()[0] == 2, "canonical range nodes were duplicated")
            next_targets = db.execute("SELECT n.kind,n.body FROM map_edge e JOIN map_node n "
                "ON n.node_id=e.target_id WHERE e.relation='OBSERVED_NEXT_PC'").fetchall()
            _require(any(kind == "M68K_TARGET_ADDRESS" and
                         json.loads(body)["attributes"]["raw_next_pc"] == 0x20
                         for kind, body in next_targets), "terminal next_pc address fact was lost")
            _require(db.execute("SELECT 1 FROM map_node WHERE kind='M68K_INSTRUCTION' "
                                "AND node_key LIKE '00000020:%'").fetchone() is None,
                     "terminal target was falsely asserted as a captured instruction")
            _require(result["source_owned_before"] == result["source_owned_after"] == 0,
                     "runtime byte coverage changed source ownership")

            replay = linkage.LiveForwardRomLinker(linkage.ROM_SHA)
            for segment, data in linker._segments:
                rows = list(PACK.iter_unpack(data))
                replay.stage(segment, rows, data)
            before = session.graph.graph_hash()
            replay_result = replay.project(session.graph, rom_path, decoder, root / "replay")
            _require(session.graph.graph_hash() == before and
                     Path(replay_result["ranges_path"]).read_bytes() == export_path.read_bytes(),
                     "repeated import changed map identity or deterministic export")
            session_path = root / "audit-session.sqlite"
            session.save_closed(session_path, 2)
            independent_audit.ROM_SHA, independent_audit.ROM_SIZE = linkage.ROM_SHA, linkage.ROM_SIZE
            independent_audit._decode = lambda decoder, rom_path, pcs, folder: {
                pc: decode_table[pc] | {"status": decode_table[pc]["decode_status"]}
                for pc in pcs}
            audited = independent_audit.audit(session_path, rom_path,
                Path(replay_result["flow_records_path"]), Path(replay_result["flow_segments_path"]),
                Path(replay_result["ranges_path"]), decoder, root / "audit.json")
            _require(audited["status"] == "PASS_INDEPENDENT_ROM_RANGE_AUDIT" and
                     audited["audited_range_occurrences"] == 3 and
                     audited["instruction_occurrences_inspected"] == 3 and
                     audited["unique_instruction_ranges"] == 2 and
                     audited["unique_executed_rom_bytes"] == 6 and
                     audited["two_byte_instructions"] == 2 and
                     audited["extended_instructions"] == 1 and
                     audited["unique_capture_ids"] == 2 and
                     audited["worker_segment_counts"] == {"0": 1, "1": 1} and
                     audited["worker_generation_counts"] == {"0": 1, "1": 1} and
                     audited["audited_terminal_next_pc_facts"] == 2,
                     "independent saved-claim audit failed to reconcile all records and Workers")
            tampered_export = root / "tampered-ranges.json"
            tampered_payload = json.loads(export_path.read_text(encoding="utf-8"))
            tampered_payload["ranges"][0]["bytes_hex"] = "0000"
            tampered_export.write_text(json.dumps(tampered_payload), encoding="utf-8")
            try:
                independent_audit.audit(session_path, rom_path,
                    Path(replay_result["flow_records_path"]), Path(replay_result["flow_segments_path"]),
                    tampered_export, decoder, root / "audit-tampered-export.json")
            except ValueError as error:
                _require("STOP_ROM_LINK_AUDIT_EXPORT_RANGE_ROW_MISMATCH" in str(error),
                         "auditor did not identify a corrupted exported range row")
            else:
                raise AssertionError("independent auditor accepted a corrupted exported range row")
            db = sqlite3.connect(session_path)
            try:
                rows = db.execute("SELECT edge_id,lineage FROM map_edge "
                    "WHERE relation='EXECUTED_FROM_ROM'").fetchall()
                for edge_id, lineage_text in rows:
                    lineages = json.loads(lineage_text)
                    changed = False
                    for lineage in lineages:
                        if lineage["worker_id"] == 0 and lineage["record_index"] == 1:
                            lineage["raw_cpu_pc"] = 4
                            changed = True
                            break
                    if changed:
                        db.execute("UPDATE map_edge SET lineage=? WHERE edge_id=?",
                            (json.dumps(lineages, sort_keys=True, separators=(",", ":")), edge_id))
                        break
                _require(changed, "could not select the formerly unsampled linked occurrence")
                db.commit()
            finally:
                db.close()
            try:
                independent_audit.audit(session_path, rom_path,
                    Path(replay_result["flow_records_path"]), Path(replay_result["flow_segments_path"]),
                    Path(replay_result["ranges_path"]), decoder, root / "audit-tampered.json")
            except ValueError as error:
                _require("STOP_ROM_LINK_AUDIT_FLOW_RECORD_MISMATCH" in str(error),
                         "auditor did not identify the corrupted unsampled FLOW lineage")
            else:
                raise AssertionError("independent auditor accepted a corrupted unsampled FLOW lineage")

            ram_row = [(1, 1, 0, 0xFF0000, 0x100, 0x4E71, 1, 0, 0, 0, 0, 0)]
            ram_session, ram_linker = _graph([(0, ram_row)])
            ram_result = ram_linker.project(ram_session.graph, rom_path, decoder, root / "ram")
            _require(ram_result["unique_instruction_ranges"] == 0 and
                     ram_result["unresolved_instruction_occurrences"] == 1,
                     "RAM execution was falsely mapped into canonical ROM")
            ram_session.close()

            false_mapping_table = dict(decode_table)
            false_mapping_table[0xFF0000] = _resolution(0xFF0000, "ROM", 0,
                "DECODED", 0x4E71, rom[:2])
            linkage.LiveForwardRomLinker._decode_batch = staticmethod(
                lambda decoder, rom_path, pcs, output: {pc: false_mapping_table[pc] for pc in pcs})
            false_session, false_linker = _graph([(0, ram_row)])
            try:
                false_linker.project(false_session.graph, rom_path, decoder, root / "false-mapping")
            except ValueError as error:
                _require("STOP_ROM_LINK_FALSE_MAPPING" in str(error),
                         "false non-ROM resolution did not fail with the required stop code")
            else:
                raise AssertionError("RAM address falsely resolved to ROM")
            false_session.close()

            unsupported_row = [(1, 1, 0, 6, 8, 0xF000, 1, 0, 0, 0, 0, 0)]
            linkage.LiveForwardRomLinker._decode_batch = staticmethod(
                lambda decoder, rom_path, pcs, output: {pc: decode_table[pc] for pc in pcs})
            unsupported_session, unsupported_linker = _graph([(0, unsupported_row)])
            unsupported_result = unsupported_linker.project(unsupported_session.graph,
                rom_path, decoder, root / "unsupported")
            _require(unsupported_result["status"] == "STOP_ROM_LINK_DECODE_UNSUPPORTED" and
                     unsupported_result["unique_instruction_ranges"] == 0,
                     "unsupported instruction length was promoted to a ROM range")
            unsupported_session.close()

            mismatch_table = dict(decode_table)
            mismatch_table[0] = _resolution(0, "ROM", 0, "DECODED", 0x48E7, rom[:2])
            linkage.LiveForwardRomLinker._decode_batch = staticmethod(
                lambda decoder, rom_path, pcs, output: {pc: mismatch_table[pc] for pc in pcs})
            mismatch_session, mismatch_linker = _graph([(0, [(1, 1, 0, 0, 2, 0x4E71, 1, 0, 0, 0, 0, 0)])])
            try:
                mismatch_linker.project(mismatch_session.graph, rom_path, decoder, root / "mismatch")
            except ValueError as error:
                _require("STOP_ROM_LINK_FLOW_OPCODE_MISMATCH" in str(error),
                         "opcode mismatch did not fail with the required stop code")
            else:
                raise AssertionError("FLOW opcode mismatch was accepted")
            mismatch_session.close()

            oob_table = dict(decode_table)
            oob_table[6] = _resolution(6, "ROM", 7, "DECODED", 0xF000, bytes.fromhex("f0004e71"))
            linkage.LiveForwardRomLinker._decode_batch = staticmethod(
                lambda decoder, rom_path, pcs, output: {pc: oob_table[pc] for pc in pcs})
            oob_session, oob_linker = _graph([(0, [(1, 1, 0, 6, 8, 0xF000, 1, 0, 0, 0, 0, 0)])])
            try:
                oob_linker.project(oob_session.graph, rom_path, decoder, root / "oob")
            except ValueError as error:
                _require("STOP_ROM_LINK_RANGE_OUT_OF_BOUNDS" in str(error),
                         "out-of-bounds ROM extent did not fail closed")
            else:
                raise AssertionError("out-of-bounds ROM extent was accepted")
            oob_session.close()

            try:
                linkage.LiveForwardRomLinker("0" * 64)
            except ValueError as error:
                _require("STOP_ROM_IDENTITY_MISMATCH" in str(error), "ROM SHA mismatch stop code changed")
            else:
                raise AssertionError("unexpected ROM SHA was accepted")
            _require(linker._instruction_rows(PACK.pack(*rows_a[0]))[0][1][5] == 0x4E71,
                     "FLOW_V1 first opcode word was not read exactly")
            z80_instruction = (*rows_a[0][:7], 1, *rows_a[0][8:])
            _require(linker._instruction_rows(PACK.pack(*z80_instruction)) == [],
                     "Z80 instruction was promoted through the M68K ROM decoder")
            session.close()
    finally:
        linkage.ROM_SHA, linkage.ROM_SIZE = old_sha, old_size
        linkage.LiveForwardRomLinker._decode_batch = old_decode
        independent_audit.ROM_SHA, independent_audit.ROM_SIZE = old_audit_sha, old_audit_size
        independent_audit._decode = old_audit_decode
    print("PASS: exact ROM range, occurrence dedup, terminal next_pc, idempotency and fail-closed fixtures")


if __name__ == "__main__":
    main()
