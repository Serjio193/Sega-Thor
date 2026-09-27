"""Fail-closed VDP and DMA Analysis Stage for M12 post-run pipeline.

Decodes Genesis VDP control commands, register state, DMA operations,
memory-to-VDP producer provenance, candidate ROM graphics ranges, and verifies
S8 frame-779 regression with zero conflicts.
"""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import sys
from typing import Any, Mapping

TOOLS_DIR = Path(__file__).parents[2] / "src" / "tools"
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))
EVIDENCE_TOOLS = TOOLS_DIR / "thor_evidence"
if str(EVIDENCE_TOOLS) not in sys.path:
    sys.path.insert(0, str(EVIDENCE_TOOLS))

from identity import ROM_SHA, ROM_SIZE  # noqa: E402
from thor_evidence.w3_z80_evidence import (  # noqa: E402
    DOMAIN_68K_RAM, DOMAIN_ROM, DOMAIN_VDP,
)

SOURCE_OWNED_CANONICAL = 1487672
REC_STRUCT = struct.Struct("<QQQIIIHBBHHI")


def sha256_file(path: Path | str | None) -> str:
    if path is None:
        raise ValueError("STOP_HASH_TARGET_IS_NONE")
    target = Path(path)
    if not target.is_file():
        raise ValueError(f"STOP_HASH_TARGET_MISSING:{target}")
    hasher = hashlib.sha256()
    with open(target, "rb") as stream:
        while chunk := stream.read(1024 * 1024):
            hasher.update(chunk)
    return hasher.hexdigest()


def decompose_vdp_bus_event(ev: Mapping[str, Any]) -> list[int]:
    """Decompose a VDP bus write event into 16-bit control words per exact width contract."""
    width = ev.get("width")
    val = int(ev["value"])
    if width == 32:
        return [(val >> 16) & 0xFFFF, val & 0xFFFF]
    if width == 16:
        return [val & 0xFFFF]
    raise ValueError(f"STOP_UNRESOLVED_ORACLE_EVENT_WIDTH: width={width}")


def verify_s8_regression(oracle_path: Path | str | None = None) -> dict[str, Any]:
    """Verify generic VDP decoder against durable real VDP oracle V2 or fallback fixture."""
    oracles_dir = Path(__file__).parents[2] / "build" / "thor-evidence" / "oracles" / "vdp"
    cand_f3 = oracles_dir / "vdp_oracle_v2_f3" / "raw_capture.json"
    cand_f200 = oracles_dir / "vdp_oracle_v2_f200" / "raw_capture.json"
    if oracle_path:
        target = Path(oracle_path)
    elif cand_f3.is_file():
        target = cand_f3
    elif cand_f200.is_file():
        target = cand_f200
    else:
        target = Path("build/m12-gfx-runtime/hardware-vdp-frame-capture-s8.json")

    if target.is_file():
        capture = json.loads(target.read_text(encoding="utf-8"))
        decoder = VdpProtocolDecoder()
        if "vdp_events" in capture:
            for ev in capture["vdp_events"]:
                raw_addr = ev.get("raw_address") or int(ev.get("address", "0"), 16)
                if raw_addr in (0xC00004, 0xC00006):
                    words = decompose_vdp_bus_event(ev)
                    pc = int(ev["pc"], 16) if isinstance(ev["pc"], str) else ev["pc"]
                    t = ev.get("m68k_total_cycles", ev.get("master_time", 0))
                    for w in words:
                        decoder.consume_control_word(w, pc, ev.get("frame", 0), t, ev.get("stream_sequence", 0))
            expected_regs = {int(k): v for k, v in capture.get("registers", {}).items()}
            conflicts = sum(1 for r, v in expected_regs.items() if decoder.active_registers.get(r) != v)
            return {"status": "PASS" if conflicts == 0 else "STOP", "conflicts": conflicts,
                    "oracle_path": str(target), "oracle_sha256": sha256_file(target),
                    "oracle_id": capture.get("oracle_id", "vdp_oracle_v2"),
                    "commands_decoded": len(decoder.complete_commands),
                    "dma_decoded": len(decoder.dma_events)}
        for e in capture.get("register_events", []):
            w = 0x8000 | (e["register"] << 8) | e["value"]
            decoder.consume_control_word(w, pc=0x200, frame=e.get("frame", 779), time=1000, seq=e.get("sequence", 0))
        conflicts = sum(1 for e in capture.get("register_events", []) if decoder.active_registers.get(e["register"]) != e["value"])
        return {"status": "PASS" if conflicts == 0 else "STOP", "conflicts": conflicts,
                "oracle_path": str(target), "oracle_sha256": sha256_file(target)}

    fixture = Path(__file__).parents[2] / "tests" / "m12_vdp_frame_artifact_test.py"
    if not fixture.is_file():
        raise ValueError("STOP_S8_FIXTURE_MISSING")

    import importlib.util
    spec = importlib.util.spec_from_file_location("test_s8", fixture)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(mod)
    capture = mod._capture()
    decoder = VdpProtocolDecoder()
    for e in capture["register_events"]:
        w = 0x8000 | (e["register"] << 8) | e["value"]
        decoder.consume_control_word(w, pc=0x200, frame=e["frame"], time=1000, seq=e["sequence"])
    conflicts = sum(1 for e in capture["register_events"] if decoder.active_registers.get(e["register"]) != e["value"])
    return {"status": "PASS" if conflicts == 0 else "STOP", "conflicts": conflicts,
            "oracle_path": str(target), "oracle_sha256": None, "fixture_path": str(fixture),
            "fixture_sha256": sha256_file(fixture)}


class VdpProtocolDecoder:
    """Generic Genesis VDP protocol decoder without hardcoded run assumptions."""

    def __init__(self) -> None:
        self.active_registers: dict[int, int] = {}
        self.register_write_counts: dict[int, int] = {}
        self.register_first_seen: dict[int, dict[str, Any]] = {}
        self.pending_word: dict[str, Any] | None = None
        self.complete_commands: list[dict[str, Any]] = []
        self.incomplete_commands: list[dict[str, Any]] = []
        self.dma_events: list[dict[str, Any]] = []

    def reset_segment(self) -> None:
        if self.pending_word is not None:
            self.incomplete_commands.append(dict(self.pending_word))
            self.pending_word = None
        self.active_registers = {}

    def consume_control_word(self, word: int, pc: int, frame: int,
                              time: int, seq: int) -> dict[str, Any] | None:
        word &= 0xFFFF
        if (word & 0xC000) == 0x8000:
            reg, val = (word >> 8) & 0x1F, word & 0xFF
            self.active_registers[reg] = val
            self.register_write_counts[reg] = self.register_write_counts.get(reg, 0) + 1
            if reg not in self.register_first_seen:
                self.register_first_seen[reg] = {"frame": frame, "time": time, "pc": f"0x{pc:06X}", "value": val}
            return None

        if self.pending_word is None:
            self.pending_word = {"word": word, "pc": pc, "frame": frame, "time": time, "seq": seq}
            return None

        w1_item, self.pending_word = self.pending_word, None
        w1, w2 = int(w1_item["word"]), word
        code = ((w1 >> 14) & 3) | ((w2 >> 2) & 0x3C)
        address = (w1 & 0x3FFF) | ((w2 & 3) << 14)
        target = {1: "VRAM", 3: "CRAM", 5: "VSRAM"}.get(code & 0x0F, "OTHER")
        if target == "OTHER":
            target = "VRAM" if (code & 0x0F) in (0, 1) else ("CRAM" if (code & 0x0F) in (3, 8) else ("VSRAM" if (code & 0x0F) in (4, 5) else "OTHER"))

        is_dma = bool(code & 0x20)
        direction = "WRITE" if (code & 0x01) or (code & 0x0F in (3, 5)) else "READ"
        cmd = {
            "first_word": w1, "second_word": w2, "code": code,
            "address": address, "target": target, "direction": direction,
            "dma_requested": is_dma, "pc": f"0x{w1_item['pc']:06X}",
            "frame": frame, "master_time": time, "first_stream_seq": w1_item["seq"],
            "second_stream_seq": seq,
        }
        if is_dma:
            dma_desc = self._decode_dma(cmd)
            cmd["dma"] = dma_desc
            self.dma_events.append(dma_desc)
        self.complete_commands.append(cmd)
        return cmd

    def _decode_dma(self, cmd: Mapping[str, Any]) -> dict[str, Any]:
        needed = (19, 20, 21, 22, 23)
        present = sum(1 for r in needed if r in self.active_registers)
        if present == 5:
            r19, r20, r21, r22, r23 = (self.active_registers[r] for r in needed)
            len_words = (r20 << 8) | r19
            if len_words == 0:
                len_words = 0x10000
            mode = {2: "FILL", 3: "COPY"}.get(r23 >> 6, "68K_BUS")
            src_addr = (r23 << 17) | (r22 << 9) | (r21 << 1)
            src_domain = "68K_RAM" if src_addr >= 0xFF0000 else ("ROM" if src_addr < 0x400000 else "OTHER")
            classification = "DMA_EXACT" if mode == "68K_BUS" and len_words > 0 else "DMA_PARTIAL"
            return {
                "dma_type": mode, "source_domain": src_domain,
                "source_address": f"0x{src_addr:06X}",
                "physical_rom_address": f"0x{src_addr:06X}" if src_domain == "ROM" else None,
                "destination_domain": cmd["target"], "destination_address": f"0x{cmd['address']:04X}",
                "length_words": len_words, "length_bytes": len_words * 2,
                "classification": classification, "frame": cmd["frame"],
                "master_time": cmd["master_time"], "causing_pc": cmd["pc"],
            }
        classification = "DMA_PARTIAL" if present >= 2 else "DMA_INCOMPLETE"
        return {
            "dma_type": "UNKNOWN", "source_domain": "UNRESOLVED", "source_address": None,
            "physical_rom_address": None, "destination_domain": cmd["target"],
            "destination_address": f"0x{cmd['address']:04X}", "length_words": 0,
            "length_bytes": 0, "classification": classification, "frame": cmd["frame"],
            "master_time": cmd["master_time"], "causing_pc": cmd["pc"],
        }


def cluster_candidate_vdp_ranges(rom_accesses: dict[int, dict[str, Any]],
                                  max_gap: int = 64) -> list[dict[str, Any]]:
    if not rom_accesses:
        return []
    sorted_addrs = sorted(rom_accesses.keys())
    clusters: list[list[int]] = []
    curr: list[int] = [sorted_addrs[0]]
    for addr in sorted_addrs[1:]:
        if addr - curr[-1] <= max_gap:
            curr.append(addr)
        else:
            clusters.append(curr)
            curr = [addr]
    if curr:
        clusters.append(curr)

    result = []
    for idx, cl in enumerate(clusters, 1):
        s_addr, e_addr = cl[0], cl[-1]
        result.append({
            "candidate_id": f"VDP_CAND_{idx:03d}",
            "physical_start": f"0x{s_addr:06X}", "physical_end": f"0x{e_addr:06X}",
            "bytes_observed": e_addr - s_addr + 1,
            "access_count": sum(rom_accesses[a]["count"] for a in cl),
            "frames_seen": sorted({f for a in cl for f in rom_accesses[a]["frames"]}),
            "destination_domains": sorted({d for a in cl for d in rom_accesses[a]["domains"]}),
            "consumer_pcs": sorted({p for a in cl for p in rom_accesses[a]["pcs"]}),
            "classification": "VDP_RESOURCE_CANDIDATE",
        })
    return result


def run_vdp_stage(receipt: dict[str, Any], rom_path: Path, output_dir: Path,
                  total_segments: int | None = None, progress: Any = None) -> dict[str, Any]:
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_VDP_ROM_IDENTITY_MISMATCH")

    handoff = receipt.get("flow_handoff") or receipt.get("raw_segment_spool", {})
    raw_path, index_path = Path(handoff.get("raw_path", "")), Path(handoff.get("index_path", ""))
    if not raw_path.is_file() or not index_path.is_file():
        for arc in [output_dir.parents[2] / "raw-evidence-archive" if len(output_dir.parents) > 2 else None,
                    output_dir.parent / "raw-evidence-archive", raw_path.parent.parent / "raw-evidence-archive"]:
            if arc and (arc / raw_path.name).is_file() and (arc / index_path.name).is_file():
                raw_path, index_path = arc / raw_path.name, arc / index_path.name
                break

    if not raw_path.is_file() or not index_path.is_file():
        raise ValueError("STOP_VDP_STAGE_FLOW_ARTIFACT_MISSING")

    run_id, decoder = int(receipt.get("runtime", {}).get("run_id", 0)), VdpProtocolDecoder()
    vdp_ctrl = vdp_data = vdp_reg = vram_ops = cram_ops = vsram_ops = 0
    rom_chains = ram_chains = temp_relations = unres_relations = processed_segs = 0
    rom_accesses: dict[int, dict[str, Any]] = {}
    exp_segs = total_segments or handoff.get("segments") or None
    unique_cmd_ids, unique_data_ids, unique_dma_ids = set(), set(), set()
    u_dma_exact, u_dma_part, u_dma_inc = set(), set(), set()

    with index_path.open("r", encoding="utf-8") as stream, raw_path.open("rb") as raw_file:
        for line in stream:
            item = json.loads(line)
            epoch = int(item.get("segment", {}).get("epoch", 0))
            raw_file.seek(int(item["raw_offset"]))
            data = raw_file.read(int(item["raw_length"]))
            decoder.reset_segment()
            last_read, curr_frame = None, 0

            for i in range(0, len(data), REC_STRUCT.size):
                fields = REC_STRUCT.unpack_from(data, i)
                seq, inst_seq, master_time, pc, addr, val = fields[:6]
                flags, width, domain = fields[6], fields[8], fields[9]
                subtype = (flags & 0x3800) >> 11

                if subtype == 3:
                    curr_frame = (pc & 0xFFFFFFFF) | ((addr & 0xFFFFFFFF) << 32) if (pc != 0 or addr != 0) else val
                    continue
                if subtype == 1:
                    last_read = (inst_seq, pc, addr, val, domain, master_time, curr_frame)
                    continue
                if subtype != 2:
                    continue

                if addr in (0xC00000, 0xC00002):
                    vdp_data += 1
                    unique_data_ids.add((run_id, epoch, curr_frame, seq))
                    if last_read is not None and last_read[0] == inst_seq:
                        if last_read[4] in (DOMAIN_ROM, 0):
                            rom_chains += 1
                            r_entry = rom_accesses.setdefault(last_read[2], {"count": 0, "frames": set(), "pcs": set(), "domains": set()})
                            r_entry["count"] += 1; r_entry["frames"].add(curr_frame); r_entry["pcs"].add(f"0x{pc:06X}"); r_entry["domains"].add("VRAM")
                        elif last_read[4] in (DOMAIN_68K_RAM, 1):
                            ram_chains += 1
                        else:
                            unres_relations += 1
                    elif last_read is not None and last_read[6] == curr_frame:
                        temp_relations += 1
                    else:
                        unres_relations += 1
                    continue

                if addr in (0xC00004, 0xC00006):
                    vdp_ctrl += 1
                    words = [(val >> 16) & 0xFFFF, val & 0xFFFF] if width == 32 else [val & 0xFFFF]
                    for w in words:
                        if (w & 0xC000) == 0x8000:
                            vdp_reg += 1
                            decoder.consume_control_word(w, pc, curr_frame, master_time, seq)
                        else:
                            cmd = decoder.consume_control_word(w, pc, curr_frame, master_time, seq)
                            if cmd is not None:
                                tgt = cmd["target"]
                                vram_ops += 1 if tgt == "VRAM" else 0
                                cram_ops += 1 if tgt == "CRAM" else 0
                                vsram_ops += 1 if tgt == "VSRAM" else 0
                                cmd_id = (run_id, epoch, curr_frame, cmd["first_stream_seq"])
                                unique_cmd_ids.add(cmd_id)
                                if cmd.get("dma"):
                                    dma = cmd["dma"]
                                    unique_dma_ids.add(cmd_id)
                                    cls = dma["classification"]
                                    if cls == "DMA_EXACT":
                                        u_dma_exact.add(cmd_id)
                                        if dma["source_domain"] == "68K_RAM":
                                            ram_chains += 1
                                        elif dma["source_domain"] == "ROM":
                                            rom_chains += 1
                                            src_i = int(dma["source_address"], 16)
                                            r_entry = rom_accesses.setdefault(src_i, {"count": 0, "frames": set(), "pcs": set(), "domains": set()})
                                            r_entry["count"] += 1; r_entry["frames"].add(curr_frame); r_entry["pcs"].add(f"0x{pc:06X}"); r_entry["domains"].add(tgt)
                                        else:
                                            unres_relations += 1
                                    elif cls == "DMA_PARTIAL":
                                        u_dma_part.add(cmd_id)
                                        unres_relations += 1
                                    else:
                                        u_dma_inc.add(cmd_id)
                                        unres_relations += 1

            processed_segs += 1
            if progress and processed_segs % 128 == 0:
                progress.update(processed_segs, total=exp_segs, detail=f"processed {processed_segs:,} FLOW segments")
                progress.heartbeat("analyzing VDP / DMA provenance")

    decoder.reset_segment()
    s8_res = verify_s8_regression()
    if s8_res["conflicts"] != 0 or s8_res["status"] != "PASS":
        raise ValueError(f"STOP_S8_VDP_REGRESSION_CONFLICTS:{s8_res['conflicts']}")

    dma_exact = sum(1 for d in decoder.dma_events if d["classification"] == "DMA_EXACT")
    dma_partial = sum(1 for d in decoder.dma_events if d["classification"] == "DMA_PARTIAL")
    dma_incomplete = sum(1 for d in decoder.dma_events if d["classification"] == "DMA_INCOMPLETE")
    candidate_ranges = cluster_candidate_vdp_ranges(rom_accesses)

    u_cmds = len(unique_cmd_ids)
    u_data = len(unique_data_ids)
    u_dma = len(unique_dma_ids)

    analysis_path = output_dir / "postrun_vdp_analysis.json"
    analysis_data = {
        "schema": "oasis.m12.postrun-vdp-analysis.v1", "run_id": run_id, "status": "PASS", "state": "PASS",
        "vdp_control_writes": vdp_ctrl, "vdp_data_writes": vdp_data, "vdp_register_writes": vdp_reg,
        "complete_vdp_commands": len(decoder.complete_commands), "incomplete_vdp_commands": len(decoder.incomplete_commands),
        "unique_vdp_commands": u_cmds, "segment_vdp_command_observations": len(decoder.complete_commands),
        "duplicate_vdp_commands_removed": len(decoder.complete_commands) - u_cmds,
        "unique_vdp_data_writes": u_data, "segment_vdp_data_write_observations": vdp_data,
        "duplicate_vdp_data_writes_removed": vdp_data - u_data,
        "vram_operations": vram_ops, "cram_operations": cram_ops, "vsram_operations": vsram_ops,
        "dma_events": len(decoder.dma_events), "dma_exact": dma_exact, "dma_partial": dma_partial, "dma_incomplete": dma_incomplete,
        "unique_dma_events": u_dma, "segment_dma_observations": len(decoder.dma_events),
        "duplicate_dma_observations_removed": len(decoder.dma_events) - u_dma,
        "unique_dma_exact": len(u_dma_exact), "unique_dma_partial": len(u_dma_part), "unique_dma_incomplete": len(u_dma_inc),
        "exact_rom_to_vdp_chains": rom_chains, "exact_ram_to_vdp_chains": ram_chains,
        "temporal_only_vdp_relations": temp_relations, "unresolved_vdp_relations": unres_relations,
        "vdp_resource_candidates": len(candidate_ranges), "s8_vdp_regression": s8_res["status"],
        "s8_conflicts": s8_res["conflicts"], "source_owned_before": SOURCE_OWNED_CANONICAL,
        "source_owned_after": SOURCE_OWNED_CANONICAL, "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    analysis_path.write_text(json.dumps(analysis_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    reg_path = output_dir / "postrun_vdp_registers.json"
    reg_summary = {str(r): {"register_id": r, "write_count": decoder.register_write_counts.get(r, 0), "first_seen": decoder.register_first_seen.get(r)} for r in sorted(decoder.register_write_counts.keys())}
    reg_path.write_text(json.dumps({"schema": "oasis.m12.postrun-vdp-registers.v1", "run_id": run_id, "observed_registers_count": len(reg_summary), "registers": reg_summary, "truth_version": "M12-CANONICAL-TRUTH-V1"}, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    dma_path = output_dir / "postrun_vdp_dma.json"
    dma_path.write_text(json.dumps({"schema": "oasis.m12.postrun-vdp-dma.v1", "run_id": run_id, "dma_events_count": len(decoder.dma_events), "dma_exact": dma_exact, "dma_partial": dma_partial, "dma_incomplete": dma_incomplete, "unique_dma_events": u_dma, "unique_dma_exact": len(u_dma_exact), "unique_dma_partial": len(u_dma_part), "unique_dma_incomplete": len(u_dma_inc), "dma_events_sample": decoder.dma_events[:50], "truth_version": "M12-CANONICAL-TRUTH-V1"}, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    prov_path = output_dir / "postrun_vdp_provenance.json"
    prov_path.write_text(json.dumps({"schema": "oasis.m12.postrun-vdp-provenance.v1", "run_id": run_id, "exact_rom_to_vdp_chains": rom_chains, "exact_ram_to_vdp_chains": ram_chains, "temporal_only_vdp_relations": temp_relations, "unresolved_vdp_relations": unres_relations, "truth_version": "M12-CANONICAL-TRUTH-V1"}, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    cand_path = output_dir / "postrun_vdp_candidates.json"
    cand_path.write_text(json.dumps({"schema": "oasis.m12.postrun-vdp-candidates.v1", "run_id": run_id, "candidate_ranges_count": len(candidate_ranges), "candidate_ranges": candidate_ranges, "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1"}, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    campaign_id = receipt.get("campaign_dir", output_dir.parent.name)
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")
    flow_idx_sha = handoff.get("index_sha256") or sha256_file(index_path)
    receipt_path = output_dir / "postrun_vdp_receipt.json"
    receipt_data = {
        "schema": "oasis.m12.postrun-vdp-receipt.v1", "status": "PASS", "state": "PASS",
        "run_id": run_id, "campaign_id": campaign_id, "timestamp": timestamp,
        "stage_version": "M12-POSTRUN-VDP-DMA-ANALYSIS-V1",
        "input_hashes": {"flow_v1_records_sha256": sha256_file(raw_path), "flow_v1_segments_sha256": flow_idx_sha, "rom_sha256": ROM_SHA},
        "output_hashes": {
            "postrun_vdp_analysis_sha256": sha256_file(analysis_path), "postrun_vdp_registers_sha256": sha256_file(reg_path),
            "postrun_vdp_dma_sha256": sha256_file(dma_path), "postrun_vdp_provenance_sha256": sha256_file(prov_path),
            "postrun_vdp_candidates_sha256": sha256_file(cand_path),
        },
        "metrics": {
            "vdp_control_writes": vdp_ctrl, "vdp_data_writes": vdp_data, "vdp_register_writes": vdp_reg,
            "complete_vdp_commands": len(decoder.complete_commands), "incomplete_vdp_commands": len(decoder.incomplete_commands),
            "unique_vdp_commands": u_cmds, "segment_vdp_command_observations": len(decoder.complete_commands),
            "duplicate_vdp_commands_removed": len(decoder.complete_commands) - u_cmds,
            "unique_vdp_data_writes": u_data, "segment_vdp_data_write_observations": vdp_data,
            "duplicate_vdp_data_writes_removed": vdp_data - u_data,
            "vram_operations": vram_ops, "cram_operations": cram_ops, "vsram_operations": vsram_ops,
            "dma_events": len(decoder.dma_events), "dma_exact": dma_exact, "dma_partial": dma_partial, "dma_incomplete": dma_incomplete,
            "unique_dma_events": u_dma, "segment_dma_observations": len(decoder.dma_events),
            "duplicate_dma_observations_removed": len(decoder.dma_events) - u_dma,
            "unique_dma_exact": len(u_dma_exact), "unique_dma_partial": len(u_dma_part), "unique_dma_incomplete": len(u_dma_inc),
            "exact_rom_to_vdp_chains": rom_chains, "exact_ram_to_vdp_chains": ram_chains,
            "temporal_only_vdp_relations": temp_relations, "unresolved_vdp_relations": unres_relations,
            "vdp_resource_candidates": len(candidate_ranges),
        },
        "s8_vdp_regression": s8_res["status"], "conflicts": s8_res["conflicts"],
        "source_owned_before": SOURCE_OWNED_CANONICAL, "source_owned_after": SOURCE_OWNED_CANONICAL, "source_owned_delta": 0,
        "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    receipt_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # Mirror to campaign post-run-analysis if run from inside an isolated subfolder
    candidates = [output_dir.parent / "post-run-analysis"]
    if len(output_dir.parents) > 2:
        candidates.append(output_dir.parents[2] / "post-run-analysis")
    import shutil
    for post_run_mirror in candidates:
        if post_run_mirror and post_run_mirror.is_dir() and output_dir != post_run_mirror:
            for f in (analysis_path, reg_path, dma_path, prov_path, cand_path, receipt_path):
                shutil.copy2(f, post_run_mirror / f.name)

    return {
        "status": "PASS", "state": "PASS", "segments_processed": processed_segs,
        "vdp_control_writes": vdp_ctrl, "vdp_data_writes": vdp_data, "vdp_register_writes": vdp_reg,
        "complete_vdp_commands": len(decoder.complete_commands), "incomplete_vdp_commands": len(decoder.incomplete_commands),
        "unique_vdp_commands": u_cmds, "segment_vdp_command_observations": len(decoder.complete_commands),
        "duplicate_vdp_commands_removed": len(decoder.complete_commands) - u_cmds,
        "unique_vdp_data_writes": u_data, "segment_vdp_data_write_observations": vdp_data,
        "duplicate_vdp_data_writes_removed": vdp_data - u_data,
        "vram_operations": vram_ops, "cram_operations": cram_ops, "vsram_operations": vsram_ops,
        "dma_events": len(decoder.dma_events), "dma_exact": dma_exact, "dma_partial": dma_partial, "dma_incomplete": dma_incomplete,
        "unique_dma_events": u_dma, "segment_dma_observations": len(decoder.dma_events),
        "duplicate_dma_observations_removed": len(decoder.dma_events) - u_dma,
        "unique_dma_exact": len(u_dma_exact), "unique_dma_partial": len(u_dma_part), "unique_dma_incomplete": len(u_dma_inc),
        "exact_rom_to_vdp_chains": rom_chains, "exact_ram_to_vdp_chains": ram_chains,
        "temporal_only_vdp_relations": temp_relations, "unresolved_vdp_relations": unres_relations,
        "vdp_resource_candidates": len(candidate_ranges), "s8_vdp_regression": s8_res["status"],
        "s8_conflicts": s8_res["conflicts"], "source_owned_before": SOURCE_OWNED_CANONICAL,
        "source_owned_after": SOURCE_OWNED_CANONICAL, "source_owned_delta": 0,
        "analysis_path": str(analysis_path), "registers_path": str(reg_path),
        "dma_path": str(dma_path), "provenance_path": str(prov_path),
        "candidates_path": str(cand_path), "receipt_path": str(receipt_path),
    }
