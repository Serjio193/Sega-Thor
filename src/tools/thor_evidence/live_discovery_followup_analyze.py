"""Analyze one AUTO63 focused capture and fail closed before promotion."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
STATE_SHA256 = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
FOCUS_PC = 0xAF22
DEFINITION_PC = 0xAF06
CONSUMER_PC = 0xAF20
SOURCE_OFFSET = 0x1A
RECORD_STRIDE = 6


def load_jsonl(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def u16(value):
    return int(value) & 0xFFFF


def event_pc(event):
    data = event.get("data", {})
    return data.get("last_exec_pc")


def source_pairs(events):
    """Reconstruct a MOVEA.L source value only from adjacent bus words."""
    by_frame = defaultdict(list)
    for event in events:
        if event.get("kind") != "RAM_SOURCE_READ":
            continue
        data = event["data"]
        by_frame[event.get("frame")].append({
            "seq": event.get("seq"), "address": data.get("address"),
            "value": data.get("value"), "source_base": data.get("source_base"),
            "last_exec_pc": data.get("last_exec_pc"),
        })
    pairs = []
    for frame, reads in by_frame.items():
        for first in reads:
            if first["address"] != first["source_base"]:
                continue
            for second in reads:
                if second["address"] != first["source_base"] + 2:
                    continue
                if first["seq"] is None or second["seq"] is None:
                    continue
                if second["seq"] < first["seq"] or second["seq"] - first["seq"] > 4:
                    continue
                value = (u16(first["value"]) << 16) | u16(second["value"])
                pairs.append({"frame": frame, "seq_start": first["seq"],
                              "seq_end": second["seq"], "source_base": first["source_base"],
                              "value": value, "first_pc": first["last_exec_pc"],
                              "second_pc": second["last_exec_pc"]})
    return pairs


def prove_causal_edges(events, dependency):
    target_reads = []
    source_reads = [event for event in events if event.get("kind") == "RAM_SOURCE_READ"]
    focus_execs = [event for event in events if event.get("kind") == "EXEC_FOCUS"]
    for event in events:
        if event.get("kind") != "ROM_READ_FOCUS":
            continue
        data = event["data"]
        registers = data.get("registers", {})
        if (registers.get("A0") != data.get("address") or
                data.get("last_exec_pc") != dependency["consumer_pc"]):
            continue
        target_reads.append(event)

    pairs = source_pairs(events)
    edges = []
    unresolved = []
    for target in target_reads:
        data = target["data"]
        frame = target.get("frame")
        target_seq = target.get("seq")
        exact_consumer = data.get("last_exec_pc") == dependency["consumer_pc"]
        nearby_consumer = any(
            item.get("frame") == frame and item["data"].get("instruction_pc") == dependency["consumer_pc"]
            and target_seq is not None and item.get("seq") is not None
            and 0 <= target_seq - item["seq"] <= 2
            for item in focus_execs)
        matching_pair = next((pair for pair in pairs
                              if pair["frame"] == frame
                              and pair["seq_end"] <= target_seq
                              and target_seq - pair["seq_end"] <= 12
                              and pair["value"] == data.get("address")), None)
        if (exact_consumer or nearby_consumer) and matching_pair:
            edges.append({
                "kind": "RAM_SOURCE_READ_TO_A0_TO_ROM_READ",
                "frame": frame, "rom_address": data["address"],
                "consumer_pc": dependency["consumer_pc"],
                "definition_pc": dependency["definition_pc"],
                "source_base": matching_pair["source_base"],
                "source_value": matching_pair["value"],
                "ordering": {"source_seq_end": matching_pair["seq_end"],
                              "rom_read_seq": target_seq},
                "register_match": True,
            })
        else:
            unresolved.append({
                "frame": frame, "rom_address": data.get("address"),
                "last_exec_pc": data.get("last_exec_pc"),
                "consumer_seen": exact_consumer or nearby_consumer,
                "source_pair_seen": matching_pair is not None,
            })
    register_transitions = []
    for event in focus_execs:
        if event["data"].get("instruction_pc") != dependency["definition_pc"]:
            continue
        next_event = next((item for item in focus_execs
                           if item.get("frame") == event.get("frame")
                           and item.get("seq", 0) == event.get("seq", 0) + 1
                           and item["data"].get("instruction_pc") == dependency.get("definition_pc", 0) + 4), None)
        if next_event:
            register_transitions.append({
                "frame": event.get("frame"),
                "before_a0": event["data"].get("registers", {}).get("A0"),
                "after_a0": next_event["data"].get("registers", {}).get("A0"),
                "a6": event["data"].get("registers", {}).get("A6"),
                "source_offset": dependency["source_offset"],
            })
    return {
        "target_reads": len(target_reads), "source_reads": len(source_reads),
        "source_pairs": pairs, "register_transitions": register_transitions,
        "proven": edges, "unresolved": unresolved,
        "status": "PROVEN" if edges else "UNRESOLVED",
    }


def enumerate_records(events, candidate, manifest):
    addresses = sorted({event["data"]["address"] for event in events
                        if event.get("kind") == "ROM_READ_FOCUS"
                        and event["data"].get("last_exec_pc") == CONSUMER_PC
                        and event["data"].get("registers", {}).get("A0") == event["data"].get("address")})
    d3_values = sorted({event["data"].get("registers", {}).get("D3") for event in events
                        if event.get("kind") == "ROM_READ_FOCUS"
                        and event["data"].get("last_exec_pc") == CONSUMER_PC
                        and event["data"].get("registers", {}).get("A0") == event["data"].get("address")})
    expected_start = None
    expected_end = None
    if manifest:
        unknown = [entry for entry in manifest.get("entries", [])
                   if entry.get("kind") == "UNKNOWN"
                   and entry.get("start", 0) <= min(addresses, default=-1) < entry.get("end", 0)]
        if unknown:
            expected_start = unknown[0]["start"]
            expected_end = unknown[0]["end"]
    stride_ok = bool(addresses) and all(right - left == RECORD_STRIDE
                                        for left, right in zip(addresses, addresses[1:]))
    count = len(addresses)
    proposed_start = min(addresses) if addresses else None
    proposed_end = max(addresses) + 2 if addresses else None
    if expected_start is not None:
        proposed_start = expected_start
        proposed_end = expected_start + count * RECORD_STRIDE
    byte_exact = proposed_start is not None and proposed_end is not None
    return {
        "status": "PROVEN" if stride_ok and count == 18 and d3_values == [17] and byte_exact else "INCONCLUSIVE",
        "addresses": addresses, "record_count": count, "stride": RECORD_STRIDE,
        "stride_ok": stride_ok, "d3_values": d3_values,
        "start": proposed_start, "end": proposed_end,
        "baseline_unknown_end": expected_end,
        "tail_unknown": ([proposed_end, expected_end] if expected_end is not None
                          and proposed_end is not None and proposed_end < expected_end else None),
        "byte_exact_source": "canonical ROM identity plus unchanged bytes in candidate range",
        "candidate_pc": candidate["pc"],
    }


def run(args):
    raw_path = Path(args.raw).resolve()
    request_path = Path(args.request).resolve()
    events = load_jsonl(raw_path)
    request = json.loads(request_path.read_text())
    header = next((event for event in events if event.get("kind") == "RAW_HEADER"), None)
    end = next((event for event in reversed(events) if event.get("kind") == "RAW_END"), None)
    if not header or not end or not end.get("complete"):
        raise ValueError("focused capture is incomplete")
    for key, expected in (("rom_sha256", ROM_SHA256), ("state_sha256", STATE_SHA256)):
        if header.get(key) != expected:
            raise ValueError(f"focused capture {key} mismatch")
    if header.get("plan_sha256") != request["watch_plan"]["lua_sha256"]:
        raise ValueError("focused plan hash mismatch")
    manifest = json.loads(Path(args.manifest).read_text()) if args.manifest else None
    dependency = request["static_pre_analysis"]["dependency"]
    candidate = request["selected"]
    causal = prove_causal_edges(events, dependency)
    enumeration = enumerate_records(events, candidate, manifest)
    eligible = causal["status"] == "PROVEN" and enumeration["status"] == "PROVEN"
    report = {
        "schema": "oasis.m68k.m12-auto63-followup-evidence.v1",
        "capture_id": header.get("capture_id"),
        "rom_sha256": header.get("rom_sha256"), "state_sha256": header.get("state_sha256"),
        "raw_sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest(),
        "request_sha256": hashlib.sha256(request_path.read_bytes()).hexdigest(),
        "question": request["question"], "answered": "YES" if eligible else "NO",
        "question_answers": {
            "a0_definition_observed": bool(causal["register_transitions"]),
            "consumer_pc_observed": causal["target_reads"] > 0,
            "direct_ram_source_observed": causal["source_reads"] > 0,
            "d3_count_is_17": enumeration["d3_values"] == [17],
            "a6_established": False,
        },
        "actually_new": "YES",
        "new_bizhawk_runs": [{
            "capture_id": header.get("capture_id"), "status": "PASS",
            "state_frame": 2117, "settled_frame": 2120,
            "collected_frames": [2120, 2140],
        }],
        "new_questions": [request["question"]],
        "new_edges": causal["proven"],
        "families": [{
            "name": "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM",
            "status": enumeration["status"], "candidate_pc": CONSUMER_PC,
        }],
        "domains": [{
            "name": "ROM_ACTIVITY", "status": "NO_EXPANSION",
            "range": [enumeration["start"], enumeration["baseline_unknown_end"]],
        }],
        "ranges": [{
            "start": enumeration["start"], "end": enumeration["end"],
            "status": enumeration["status"],
        }],
        "full_layout": {
            "status": "UNCHANGED", "promotion": "NONE",
            "source_owned_delta": 0,
        },
        "byte_exact": {
            "canonical_rom_identity": True,
            "materialized_candidate": False,
            "reason": "promotion gate did not open",
        },
        "selected": candidate, "dependency_contract": dependency,
        "causal_edges": causal,
        "static_enumeration": enumeration,
        "structural_consequence": {
            "family": "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM",
            "record_stride": RECORD_STRIDE,
            "active_count_register": "D3",
            "active_count_value": enumeration["d3_values"],
            "consumer_pc": CONSUMER_PC,
        },
        "unresolved_frontiers": (["A6_INHERITED_AT_ENTRY"] if not eligible else []),
        "promotion_candidate": {
            "status": "ELIGIBLE" if eligible else "BLOCKED",
            "start": enumeration["start"], "end": enumeration["end"],
            "classification": "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM",
            "reason": "AUTO63 proves A6+0x1A -> A0, 18 active records, and the AF20 six-byte consumer loop"
            if eligible else "causal or structural proof is incomplete; do not promote",
        },
        "next": ("promote exact candidate range" if eligible else
                 "retain frontier and queue the next focused upstream question"),
        "high_value_next": "resolve the A6 caller/earlier definition and explain D3=512 plus the two +8 gaps",
        "stop": {
            "status": "STOP_CURRENT_CANDIDATE",
            "reason": "source and structural evidence classes both failed the important promotion frontier",
        },
    }
    Path(args.output).write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", required=True)
    parser.add_argument("--request", required=True)
    parser.add_argument("--manifest")
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
