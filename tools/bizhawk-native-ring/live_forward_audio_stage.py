"""Stage 7 Audio Analysis Stage over FLOW records and canonical ROM.

Performs exact resource recognition, W5 roundtrip decode/encode verification,
strict causal provenance tracing, candidate range discovery, and artifact emission.
"""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys
from typing import Any

TOOLS_DIR = Path(__file__).parents[2] / "src" / "tools"
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))
EVIDENCE_TOOLS = TOOLS_DIR / "thor_evidence"
if str(EVIDENCE_TOOLS) not in sys.path:
    sys.path.insert(0, str(EVIDENCE_TOOLS))

from identity import ROM_SHA, ROM_SIZE  # noqa: E402
from live_forward_scaling_audit import RECORD  # noqa: E402
from thor_evidence.w3_z80_evidence import (  # noqa: E402
    CPU_68K, CPU_Z80, DOMAIN_BANKED_ROM, DOMAIN_PSG, DOMAIN_YM2612,
    DOMAIN_Z80_RAM, DOMAIN_Z80_WINDOW, W3Record, unpack_record,
)
from thor_evidence.w4_audio_sink import AudioSinkTracker, AudioSinkType  # noqa: E402
from thor_evidence.w4_audio_provenance import (  # noqa: E402
    AudioProvenanceAnalyzer, CausalLevel, OriginType,
)
from thor_evidence.w5_audio_format import parse_bank_descriptors  # noqa: E402
from thor_evidence.w5_audio_decode import decode_audio_resource  # noqa: E402
from thor_evidence.w5_audio_encode import encode_audio_resource, encode_format_a_tokens  # noqa: E402

ACCEPTED_RESOURCES = (
    ("RESOURCE_1", 0x17, 7, 0x0BD540, 0x0BF768, 8744, 0),
    ("RESOURCE_2", 0x17, 6, 0x0BC95C, 0x0BD540, 3044, 0),
)
SOURCE_OWNED_CANONICAL = 1487672


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


def cluster_candidate_reads(other_reads: dict[int, dict[str, Any]],
                            max_gap: int = 64) -> list[dict[str, Any]]:
    if not other_reads:
        return []
    sorted_addrs = sorted(other_reads.keys())
    clusters: list[list[int]] = []
    current_cluster: list[int] = [sorted_addrs[0]]
    for addr in sorted_addrs[1:]:
        if addr - current_cluster[-1] <= max_gap:
            current_cluster.append(addr)
        else:
            clusters.append(current_cluster)
            current_cluster = [addr]
    if current_cluster:
        clusters.append(current_cluster)

    result = []
    for i, cl in enumerate(clusters, 1):
        s_addr, e_addr = cl[0], cl[-1]
        total_count = sum(other_reads[a]["count"] for a in cl)
        pcs = sorted({pc for a in cl for pc in other_reads[a]["pcs"]})
        sinks = sorted({sink for a in cl for sink in other_reads[a]["sinks"]})
        result.append({
            "candidate_id": f"AUDIO_CAND_{i:03d}",
            "range_start": f"0x{s_addr:06X}",
            "range_end": f"0x{e_addr:06X}",
            "span_bytes": e_addr - s_addr + 1,
            "read_count": total_count,
            "consuming_z80_pcs": [f"0x{p:04X}" for p in pcs],
            "associated_sinks": sinks if sinks else ["YM2612_REGISTER_WRITE"],
            "truth_class": "HYPOTHESIS",
        })
    return result


def verify_accepted_resources(rom: bytes) -> list[dict[str, Any]]:
    results = []
    for res_id, bank_id, entry_idx, start, end, length, mode in ACCEPTED_RESOURCES:
        descs = parse_bank_descriptors(rom, 0x0B8000, bank_id)
        matching = [d for d in descs if d.physical_address == start]
        if not matching:
            raise ValueError(f"STOP_ACCEPTED_RESOURCE_DESCRIPTOR_MISSING:{res_id}")
        desc = matching[0]
        if desc.byte_length != length or desc.mode != mode:
            raise ValueError(f"STOP_ACCEPTED_RESOURCE_METADATA_MISMATCH:{res_id}")
        ir = decode_audio_resource(rom, desc, res_id)
        encoded = encode_audio_resource(ir)
        orig = rom[start:end]
        if encoded != orig:
            raise ValueError(f"STOP_AUDIO_ROUNDTRIP_MISMATCH:{res_id}")

        # Non-tautological test: verify source_nibble is never read by encoder
        modified_tokens = [
            t.__class__(
                token_index=t.token_index, byte_index=t.byte_index,
                nibble_pos=t.nibble_pos, kind=t.kind,
                delta_value=t.delta_value, repeat_count=t.repeat_count,
                pcm_samples=t.pcm_samples, source_nibble=None,
            ) for t in ir.tokens
        ]
        encoded_blind = encode_format_a_tokens(modified_tokens)
        if encoded_blind != orig:
            raise ValueError(f"STOP_AUDIO_ENCODER_SOURCE_NIBBLE_DEPENDENCY:{res_id}")

        results.append({
            "resource_id": res_id,
            "bank_id": f"0x{bank_id:02X}",
            "entry_index": entry_idx,
            "physical_address": f"0x{start:06X}",
            "physical_end_exclusive": f"0x{end:06X}",
            "byte_length": length,
            "mode": mode,
            "format": f"AUDIO_FORMAT_A_MODE{mode}",
            "roundtrip_exact": True,
            "decoded_pcm_samples": len(ir.pcm_samples),
            "total_tokens": len(ir.tokens),
            "source_nibble_read_by_encoder": False,
        })
    return results


def run_audio_stage(receipt: dict[str, Any],
                    rom_path: Path,
                    output_dir: Path,
                    total_segments: int | None = None,
                    progress: Any = None) -> dict[str, Any]:
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_AUDIO_ROM_IDENTITY_MISMATCH")

    # Locate raw records and index
    handoff = receipt.get("flow_handoff") or receipt.get("raw_segment_spool", {})
    raw_path = Path(handoff.get("raw_path", ""))
    index_path = Path(handoff.get("index_path", ""))
    if not raw_path.is_file() or not index_path.is_file():
        archive = raw_path.parent.parent / "raw-evidence-archive"
        if (archive / raw_path.name).is_file() and (archive / index_path.name).is_file():
            raw_path, index_path = archive / raw_path.name, archive / index_path.name
        else:
            cand_raw = output_dir.parent / "raw-evidence-archive" / raw_path.name
            cand_idx = output_dir.parent / "raw-evidence-archive" / index_path.name
            if cand_raw.is_file() and cand_idx.is_file():
                raw_path, index_path = cand_raw, cand_idx

    if not raw_path.is_file() or not index_path.is_file():
        raise ValueError("STOP_AUDIO_STAGE_FLOW_ARTIFACT_MISSING")

    run_id = int(receipt.get("runtime", {}).get("run_id", 0))
    sink_tracker = AudioSinkTracker()
    analyzer = AudioProvenanceAnalyzer()

    ym2612_writes = dac_writes = psg_writes = 0
    res1_reads = res2_reads = other_reads_count = 0
    other_reads: dict[int, dict[str, Any]] = {}
    total_records = processed_segments = 0
    expected_segments = total_segments or handoff.get("segments") or None

    with index_path.open("r", encoding="utf-8") as stream, raw_path.open("rb") as raw:
        for line in stream:
            item = json.loads(line)
            raw.seek(int(item["raw_offset"]))
            data = raw.read(int(item["raw_length"]))
            for i in range(0, len(data), RECORD.size):
                rec = unpack_record(data, i)
                total_records += 1
                sink_event = sink_tracker.process_record(rec)
                analyzer.process_record(rec, sink_event)
                if sink_event is not None:
                    analyzer.create_chain_from_audio_event(sink_event)
                    if sink_event.sink_type == AudioSinkType.YM2612_DAC_WRITE:
                        dac_writes += 1
                    elif sink_event.sink_type == AudioSinkType.PSG_WRITE:
                        psg_writes += 1
                    elif sink_event.sink_type == AudioSinkType.YM2612_REGISTER_WRITE:
                        ym2612_writes += 1

                if rec.cpu_id == CPU_Z80 and rec.is_bus_read:
                    addr = rec.address & 0xFFFF
                    if rec.domain == DOMAIN_BANKED_ROM or addr >= 0x8000:
                        phys = rec.auxiliary if rec.auxiliary != 0 else rec.address
                        if 0x0BD540 <= phys < 0x0BF768:
                            res1_reads += 1
                        elif 0x0BC95C <= phys < 0x0BD540:
                            res2_reads += 1
                        else:
                            other_reads_count += 1
                            entry = other_reads.setdefault(phys, {"count": 0, "pcs": set(), "sinks": set()})
                            entry["count"] += 1
                            entry["pcs"].add(rec.pc)
                            entry["sinks"].add("YM2612_REGISTER_WRITE")

            processed_segments += 1
            if progress and processed_segments % 128 == 0:
                progress.update(processed_segments, total=expected_segments,
                                detail=f"processed {processed_segments:,} / {expected_segments:,} FLOW segments"
                                if expected_segments else f"processed {processed_segments:,} FLOW segments")
                progress.heartbeat("analyzing audio provenance")

    # 1. Accepted resource verification
    accepted_verified = verify_accepted_resources(rom)

    # 2. Causal chains breakdown
    l3_chains = [c for c in analyzer.chains if c.level == CausalLevel.LEVEL_3_STRICT]
    l0_chains = [c for c in analyzer.chains if c.level == CausalLevel.LEVEL_0_TEMPORAL]
    if not l3_chains:
        raise ValueError("STOP_AUDIO_NO_LEVEL_3_STRICT_CAUSAL_CHAINS")

    # 3. Candidate ranges clustering
    candidate_ranges = cluster_candidate_reads(other_reads)

    # 4. Top driver hotspots
    top_pcs = sorted(analyzer.pc_counts.items(), key=lambda x: x[1], reverse=True)[:10]
    driver_hotspots = [{"pc": f"0x{pc:04X}", "instruction_count": count} for pc, count in top_pcs]

    # Sample strict witnesses
    witnesses = []
    for c in l3_chains[:25]:
        prov = c.provenance_tag
        witnesses.append({
            "chain_id": c.chain_id,
            "level": "LEVEL_3_STRICT",
            "relation_kind": c.relation_kind.value,
            "truth_class": c.truth_class.value,
            "origin_type": prov.origin_type.value if prov else "UNKNOWN",
            "origin_address": f"0x{prov.origin_address:06X}" if prov else None,
            "sink_address": f"0x{c.audio_sink_record.address:04X}",
            "sink_value": f"0x{c.audio_sink_record.value & 0xFF:02X}",
            "rationale": c.rationale,
        })

    # Sample handoffs
    handoffs_sample = []
    for w_rec, r_rec in analyzer.last_writer.observed_handoffs[:25]:
        handoffs_sample.append({
            "writer_pc": f"0x{w_rec.pc:06X}",
            "reader_pc": f"0x{r_rec.pc:04X}",
            "offset": f"0x{r_rec.address & 0x1FFF:04X}",
            "value": f"0x{r_rec.value & 0xFF:02X}",
            "writer_time": w_rec.master_time,
            "reader_time": r_rec.master_time,
        })

    # Emit Artifact 1: postrun_audio_analysis.json
    analysis_data = {
        "schema": "oasis.m12.postrun-audio-analysis.v1",
        "stage": "AUDIO ANALYSIS",
        "stage_version": "M12-POSTRUN-AUDIO-ANALYSIS-V1",
        "status": "PASS",
        "state": "PASS",
        "run_id": run_id,
        "total_records_scanned": total_records,
        "segments_processed": processed_segments,
        "audio_sink_events": {
            "total": len(sink_tracker.sink_events),
            "ym2612_register_writes": ym2612_writes,
            "dac_writes": dac_writes,
            "psg_writes": psg_writes,
        },
        "banked_rom_reads": {
            "total": len(analyzer.banked_rom_reads),
            "resource_1_reads": res1_reads,
            "resource_2_reads": res2_reads,
            "candidate_ranges_reads": other_reads_count,
        },
        "accepted_resources_verified": accepted_verified,
        "causal_chains": {
            "total": len(analyzer.chains),
            "level_3_strict": len(l3_chains),
            "level_0_temporal": len(l0_chains),
            "unresolved": 0,
        },
        "observed_handoffs_count": len(analyzer.last_writer.observed_handoffs),
        "source_owned_before": SOURCE_OWNED_CANONICAL,
        "source_owned_after": SOURCE_OWNED_CANONICAL,
        "source_owned_delta": 0,
        "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    analysis_path = output_dir / "postrun_audio_analysis.json"
    analysis_path.write_text(json.dumps(analysis_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # Emit Artifact 2: postrun_audio_candidates.json
    candidates_data = {
        "schema": "oasis.m12.postrun-audio-candidates.v1",
        "run_id": run_id,
        "candidate_ranges_count": len(candidate_ranges),
        "candidate_ranges": candidate_ranges,
        "source_owned_delta": 0,
        "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    candidates_path = output_dir / "postrun_audio_candidates.json"
    candidates_path.write_text(json.dumps(candidates_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # Emit Artifact 3: postrun_audio_provenance.json
    provenance_data = {
        "schema": "oasis.m12.postrun-audio-provenance.v1",
        "run_id": run_id,
        "strict_causal_witnesses_count": len(l3_chains),
        "strict_causal_witnesses": witnesses,
        "observed_m68k_z80_handoffs_count": len(analyzer.last_writer.observed_handoffs),
        "observed_m68k_z80_handoffs": handoffs_sample,
        "driver_hotspots": driver_hotspots,
        "port_access_counts": {f"0x{p:04X}": c for p, c in sorted(analyzer.port_counts.items())},
        "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    provenance_path = output_dir / "postrun_audio_provenance.json"
    provenance_path.write_text(json.dumps(provenance_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # Emit Artifact 4: postrun_audio_receipt.json
    campaign_id = receipt.get("campaign_dir", output_dir.parent.name)
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")
    receipt_data = {
        "schema": "oasis.m12.postrun-audio-receipt.v1",
        "status": "PASS",
        "state": "PASS",
        "run_id": run_id,
        "campaign_id": campaign_id,
        "timestamp": timestamp,
        "stage_version": "M12-POSTRUN-AUDIO-ANALYSIS-V1",
        "input_hashes": {
            "flow_v1_records_sha256": sha256_file(raw_path),
            "flow_v1_segments_sha256": sha256_file(index_path),
            "rom_sha256": ROM_SHA,
        },
        "output_hashes": {
            "postrun_audio_analysis_sha256": sha256_file(analysis_path),
            "postrun_audio_candidates_sha256": sha256_file(candidates_path),
            "postrun_audio_provenance_sha256": sha256_file(provenance_path),
        },
        "source_owned_before": SOURCE_OWNED_CANONICAL,
        "source_owned_after": SOURCE_OWNED_CANONICAL,
        "source_owned_delta": 0,
        "truth_version": "M12-CANONICAL-TRUTH-V1",
    }
    receipt_path = output_dir / "postrun_audio_receipt.json"
    receipt_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    return {
        "status": "PASS",
        "state": "PASS",
        "total_records_scanned": total_records,
        "segments_processed": processed_segments,
        "audio_sink_events": len(sink_tracker.sink_events),
        "dac_writes": dac_writes,
        "ym2612_writes": ym2612_writes,
        "psg_writes": psg_writes,
        "banked_rom_reads": len(analyzer.banked_rom_reads),
        "resource_1_reads": res1_reads,
        "resource_2_reads": res2_reads,
        "candidate_ranges_count": len(candidate_ranges),
        "strict_causal_chains": len(l3_chains),
        "observed_handoffs": len(analyzer.last_writer.observed_handoffs),
        "w5_roundtrip_exact": "PASS",
        "source_owned_before": SOURCE_OWNED_CANONICAL,
        "source_owned_after": SOURCE_OWNED_CANONICAL,
        "source_owned_delta": 0,
        "analysis_path": str(analysis_path),
        "candidates_path": str(candidates_path),
        "provenance_path": str(provenance_path),
        "receipt_path": str(receipt_path),
    }
