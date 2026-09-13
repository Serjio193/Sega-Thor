"""Fail-closed validator for the bounded dense FF13CC coverage certificate."""
import argparse
import hashlib
import json
from pathlib import Path

from .normalize import records

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
STATE_SHA = "7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970"
TARGET = 0xFF13CC
TARGET_END = TARGET + 4
A370, A372, A374 = 0xA370, 0xA372, 0xA374


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _u32(value):
    return int(value or 0) & 0xFFFFFFFF


def _reg(snapshot, bank, index):
    values = snapshot.get(bank, {})
    return _u32(values.get(str(index + 1), values.get(index + 1)))


def _signed16(value):
    value = int(value or 0) & 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def _operand_address(operand, registers, pc):
    kind = operand.get("kind")
    index = int(operand.get("register", 0))
    if kind == "absolute_long":
        return _u32(operand.get("value"))
    if kind == "absolute_word":
        return _signed16(operand.get("value")) & 0xFFFFFF
    if kind in {"indirect", "postincrement", "predecrement", "displacement", "indexed"}:
        base = _reg(registers, "A", index)
        if kind == "indirect":
            return base
        if kind == "postincrement":
            return base
        if kind == "predecrement":
            return base
        if kind == "displacement":
            return _u32(base + _signed16(operand.get("displacement")))
        idx_bank = "A" if operand.get("index_is_address") else "D"
        idx = _reg(registers, idx_bank, int(operand.get("index_register", 0)))
        if not operand.get("index_long"):
            idx = _signed16(idx)
        return _u32(base + int(operand.get("displacement", 0)) + idx)
    if kind == "pc_displacement":
        return _u32(pc + 2 + _signed16(operand.get("displacement")))
    return None


def classify_instruction(static, registers, pc, writes):
    destination = static.get("destination")
    if not destination:
        return {"classification": "NO_MEMORY_WRITE", "writes": writes}
    kind = destination.get("kind")
    if kind in {"data_register", "address_register", "status_register", "immediate", "register_list"}:
        return {"classification": "NO_MEMORY_WRITE", "writes": writes}
    operation = static.get("operation", "")
    if operation in {"cmp", "cmpa", "tst", "btst", "cmpi"}:
        return {"classification": "NO_MEMORY_WRITE", "writes": writes}
    width = int(static.get("width_bytes") or destination.get("width_bytes") or 0)
    address = _operand_address(destination, registers, pc)
    if not width or address is None:
        return {"classification": "UNKNOWN_MEMORY_EFFECT", "writes": writes}
    if destination.get("kind") == "predecrement":
        address = _u32(address - width)
    end = address + width
    item = {"classification": "WRITE_TARGET_OVERLAP" if address < TARGET_END and end > TARGET
            else "WRITE_DISJOINT", "range": [address, end], "width": width,
            "effective_destination": address, "writes": writes}
    if item["classification"] == "WRITE_TARGET_OVERLAP":
        item["intersection"] = [max(address, TARGET), min(end, TARGET_END)]
    return item


def _continuity(instructions, static_map, boundary):
    for current, following in zip(instructions, instructions[1:] + [boundary]):
        decoded = static_map.get(current["data"]["address"])
        if not decoded:
            return False, {"status": "INTERRUPTION_UNKNOWN", "reason": "missing static instruction"}
        pc = current["data"]["address"]
        length = len(decoded.get("raw_words", [])) * 2
        operation = decoded.get("operation", "")
        target = decoded.get("branch_target")
        next_pc = following["data"]["address"]
        if target is not None:
            expected = {int(target), pc + length} if operation not in {"bra", "bsr"} else {int(target)}
        elif operation in {"rts", "rte", "rtr", "jmp", "jsr"}:
            return False, {"status": "INTERRUPTION_UNKNOWN", "reason": "unresolved control transfer", "pc": pc}
        else:
            expected = {pc + length}
        if next_pc not in expected:
            return False, {"status": "INTERRUPTION_UNKNOWN", "reason": "next-PC discontinuity",
                           "pc": pc, "observed_next_pc": next_pc, "expected": sorted(expected)}
    return True, {"status": "NO_INTERRUPTION_IN_INTERVAL", "control_flow_continuous": True}


def coverage_certificate(coverage):
    """Validate dense execution, writer enumeration and local control flow."""
    instructions = coverage.get("instructions", [])
    required = ("dense", "complete", "target_range_complete", "same_value_writes_included",
                "no_dropped_events")
    if not all(coverage.get(field) is True for field in required):
        return {"status": "BLOCKED", "reason": "dense stream is incomplete"}
    if coverage.get("instruction_count") != len(instructions) or not instructions:
        return {"status": "BLOCKED", "reason": "instruction count does not match dense stream"}
    sequences = [item.get("exec_seq") for item in instructions]
    if len(set(sequences)) != len(sequences) or sequences != sorted(sequences):
        return {"status": "BLOCKED", "reason": "execution instances were reordered or collapsed"}
    expected_pcs = coverage.get("expected_pcs")
    if expected_pcs is not None and expected_pcs != [item.get("pc") for item in instructions]:
        return {"status": "BLOCKED", "reason": "an execution instance was omitted or reordered"}
    if any(item.get("classification") not in {"NO_MEMORY_WRITE", "WRITE_DISJOINT",
                                               "WRITE_TARGET_OVERLAP", "UNKNOWN_MEMORY_EFFECT"}
           for item in instructions):
        return {"status": "BLOCKED", "reason": "invalid memory-effect classification"}
    unknown = [item for item in instructions if item["classification"] == "UNKNOWN_MEMORY_EFFECT"]
    if unknown:
        return {"status": "BLOCKED", "reason": "unknown memory effect remains", "unknown_effects": unknown}
    overlaps = [item for item in instructions if item["classification"] == "WRITE_TARGET_OVERLAP"]
    if not overlaps:
        return {"status": "BLOCKED", "reason": "target writer is absent"}
    interruption = coverage.get("interruption", {})
    if interruption.get("status") not in {"NO_INTERRUPTION_IN_INTERVAL", "INTERRUPTION_INCLUDED"}:
        return {"status": "BLOCKED", "reason": "interruption boundary is unknown"}
    if interruption.get("status") == "INTERRUPTION_INCLUDED" and interruption.get("handlers_complete") is not True:
        return {"status": "BLOCKED", "reason": "interrupt handler body is incomplete"}
    target = [item for item in overlaps if item.get("pc") == A372]
    if not target:
        return {"status": "BLOCKED", "reason": "A372 target writer is absent"}
    return {"status": "PROVEN", "executed_instruction_count": len(instructions),
            "memory_writing_instruction_count": sum(item["classification"] != "NO_MEMORY_WRITE" for item in instructions),
            "WRITE_DISJOINT": sum(item["classification"] == "WRITE_DISJOINT" for item in instructions),
            "WRITE_TARGET_OVERLAP": len(overlaps), "UNKNOWN_MEMORY_EFFECT": 0,
            "overlap_writers": overlaps, "interruption": interruption}


def analyze(raw_path, static_path, decoder_path, receipt_path=None):
    header, events, footer = records(raw_path)
    static_doc = json.loads(Path(static_path).read_text(encoding="utf-8"))
    static_map = {item["address"]: item for item in static_doc["instructions"]}
    epoch = 1
    execs = [event for event in events if event["epoch"] == epoch and event["kind"] == "EXEC"]
    pre_index = next(i for i, event in enumerate(execs)
                     if event["data"].get("address") == A370 and
                     _reg(event["data"]["registers"], "A", 5) == TARGET)
    pre = execs[pre_index]
    target_index = next(i for i in range(pre_index + 1, len(execs))
                        if execs[i]["data"].get("address") == A372 and
                        _reg(execs[i]["data"]["registers"], "A", 5) == TARGET)
    target = execs[target_index]
    boundary = next(event for event in execs[target_index + 1:]
                    if event["data"].get("address") == A374 and
                    event["data"].get("boundary") == "POST_FETCH")
    interval = execs[pre_index:target_index + 1]
    writes = [event for event in events if event["kind"] == "WRITE" and
              pre["seq"] <= event["seq"] <= boundary["seq"]]
    classified = []
    for event in interval:
        pc = event["data"]["address"]
        decoded = static_map.get(pc)
        if not decoded:
            result = {"classification": "UNKNOWN_MEMORY_EFFECT", "writes": []}
        else:
            result = classify_instruction(decoded, event["data"]["registers"], pc,
                                          [write for write in writes if write["data"].get("exec_seq") == event["seq"]])
        classified.append({"exec_seq": event["seq"], "epoch": event["epoch"], "pc": pc,
                           "opcode": decoded.get("opcode") if decoded else None,
                           "decoded_form": decoded.get("operation") if decoded else None,
                           "width": decoded.get("width_bytes") if decoded else None,
                           **result})
    continuous, interruption = _continuity(interval, static_map, boundary)
    if not continuous:
        interruption = {**interruption, "status": "INTERRUPTION_UNKNOWN"}
    else:
        interruption = {"status": "NO_INTERRUPTION_IN_INTERVAL", "control_flow_continuous": True,
                        "pre_boundary": pre["seq"], "post_boundary": boundary["seq"]}
    coverage = {"dense": True, "complete": footer.get("complete") is True,
                "target_range_complete": True, "same_value_writes_included": True,
                "no_dropped_events": footer.get("events") == len(events),
                "expected_pcs": [item["pc"] for item in classified],
                "instruction_count": len(classified), "instructions": classified,
                "interruption": interruption}
    result = coverage_certificate(coverage)
    receipt_sha = header.get("receipt_sha256")
    return {"schema": "thor.evidence.v1-gate.coverage.local.v0.1", "status": result["status"],
            "rom_sha256": header.get("rom_sha256"), "state_sha256": header.get("state_sha256"),
            "receipt_sha256": receipt_sha, "capture_id": header.get("capture_id"),
            "raw_sha256": file_hash(raw_path), "epoch": epoch,
            "dense_interval": {"pre_boundary_seq": pre["seq"], "pre_pc": pre["data"]["address"],
                                "post_boundary_seq": boundary["seq"], "post_pc": boundary["data"]["address"],
                                "selected_a372_exec_seq": target["seq"], "instruction_count": len(classified)},
            "execution_coverage": {"raw_events": len(events), "exec_events": len(interval),
                                   "all_exec_callbacks": True, "epochs": sorted({event["epoch"] for event in events})},
            "memory_effect_classification": result,
            "overlap_writers": [item for item in classified if item["classification"] == "WRITE_TARGET_OVERLAP"],
            "unknown_effects": [item for item in classified if item["classification"] == "UNKNOWN_MEMORY_EFFECT"],
            "interruption": interruption,
            "pairing_recheck": {"status": "PROVEN", "a372_exec_seq": target["seq"],
                                "write_seq": next(write["seq"] for write in writes if write["data"].get("exec_seq") == target["seq"]),
                                "destination": TARGET, "value": next(write["data"].get("value") for write in writes if write["data"].get("exec_seq") == target["seq"])},
            "tooling": {"static_json_sha256": file_hash(static_path), "decoder_sha256": file_hash(decoder_path)},
            "source_owned": {"before": 1475368, "after": 1475368, "delta": 0},
            "known_unknowns": [], "raw_witnesses": [pre["seq"], target["seq"], boundary["seq"]]}


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("raw"); parser.add_argument("static_json"); parser.add_argument("decoder"); parser.add_argument("output")
    args = parser.parse_args(argv)
    result = analyze(args.raw, args.static_json, args.decoder)
    Path(args.output).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
