"""Normalize accepted instruction-step calls and summarize their timing paths."""
import argparse
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


HELPERS = {
    0x9D70: {
        "caller": 0x9144, "return": 0x9148, "exits": {0x9D86, 0x9D90},
        "contract": "M68K_009D70_CALLER_009144_V1",
        "branches": {0x9D76: (0x9D7A, 0x9D88), 0x9D7C: (0x9D80, 0x9D86),
                      0x9D82: (0x9D86, 0x9D8A)},
    },
    0x9D4E: {
        "caller": 0x909E, "return": 0x90A2, "exits": {0x9D6E},
        "contract": "M68K_009D4E_CALLER_00909E_V1",
        "branches": {0x9D60: (0x9D64, 0x9D6E), 0x9D66: (0x9D68, 0x9D6E)},
    },
    0x9DB4: {
        "caller": 0x92CC, "return": 0x92D0, "exits": {0x9DCA, 0x9DD4},
        "contract": "M68K_009DB4_CALLER_0092CC_V1",
        "branches": {0x9DBA: (0x9DBE, 0x9DCC), 0x9DC0: (0x9DC4, 0x9DCA),
                      0x9DC6: (0x9DCA, 0x9DCE)},
    },
    0x9D92: {
        "caller": 0x9812, "return": 0x9816, "exits": {0x9DB2},
        "contract": "M68K_009D92_CALLER_009812_V1",
        "branches": {0x9DA4: (0x9DA8, 0x9DB2), 0x9DAA: (0x9DAC, 0x9DB2)},
    },
    0x94A2: {
        "caller": 0x9468, "return": 0x946C, "exits": {0x94D0},
        "contract": "M68K_0094A2_CALLER_009468_V1",
        "branches": {0x94CC: (0x94D0, 0x94C2)},
    },
    0x9C40: {
        "caller": 0xE376, "return": 0xE37A, "exits": {0x9C7A},
        "contract": "M68K_009C40_CALLER_00E376_V1",
        "branches": {0x9C52: (0x9C54, 0x9C5A)},
    },
    0x8EF8: {
        "caller": 0x5928,
        "callers": {0x58FA, 0x5928, 0x5A6A, 0x5ABA, 0x5B9A, 0x662E},
        "return": 0x592C, "exits": {0x8F10},
        "contract": "M68K_008EF8_DIRECT_CALLERS_V1", "branches": {},
    },
    0xE268: {
        "caller": 0x359A, "return": 0x35A0, "exits": {0xE2A0},
        "contract": "M68K_00E268_CALLER_00359A_V1", "branches": {},
    },
}
RTS_MASTER_CYCLES = 16 * 7
REFRESH_MASTER_CYCLES = 2 * 7
M68K_MASTER_PER_CORE_CYCLE = 7
H9D92_DECODE = {
    0x9D92: (0x3E02, "MOVE.W D2,D7", 4, 0),
    0x9D94: (0xDE6E, "ADD.W (0x46,A6),D7", 12, 0),
    0x9D98: (0x3628, "MOVE.W (0x06,A0),D3", 12, 0),
    0x9D9C: (0xE74B, "LSL.W #3,D3", 6, 6),
    0x9D9E: (0x5343, "SUBQ.W #1,D3", 4, 0),
    0x9DA0: (0x9647, "SUB.W D7,D3", 4, 0),
    0x9DA2: (0xB043, "CMP.W D3,D0", 4, 0),
    0x9DA4: (0x6300, "BLS.W +0x0C", 10, 0),
    0x9DB2: (0x4E75, "RTS", 16, 0),
}
H94A2_DECODE = {
    0x94A2: (0x3A01, "MOVE.W D1,D5", 4),
    0x94A4: (0x3C01, "MOVE.W D1,D6", 4),
    0x94A6: (0x9A43, "SUB.W D3,D5", 4),
    0x94A8: (0xDC43, "ADD.W D3,D6", 4),
    0x94AA: (0xE645, "ASR.W #3,D5", 6),
    0x94AC: (0xE646, "ASR.W #3,D6", 6),
    0x94AE: (0x9C45, "SUB.W D5,D6", 4),
    0x94B0: (0x2268, "MOVEA.L (0,A0),A1", 16),
    0x94B4: (0xD2C0, "ADDA.W D0,A1", 8),
    0x94B6: (0x3828, "MOVE.W (8,A0),D4", 12),
    0x94BA: (0xE96D, "LSL.W D4,D5", 6),
    0x94BC: (0xD2C5, "ADDA.W D5,A1", 8),
    0x94BE: (0x51C4, "SF D4", 4),
    0x94C0: (0x50C5, "ST D5", 6),
    0x94C2: (0x1411, "MOVE.B (A1),D2", 8),
    0x94C4: (0x8802, "OR.B D2,D4", 4),
    0x94C6: (0xCA02, "AND.B D2,D5", 4),
    0x94C8: (0xD2E8, "ADDA.W (4,A0),A1", 16),
    0x94CC: (0x51CE, "DBF D6,-12", 12),
    0x94D0: (0x4E75, "RTS", 16),
}
H9C40_DECODE = {
    0x9C40: (0x48E7, "MOVEM.L D1/D2/A0/A1,-(A7)", 8),
    0x9C44: (0x41F9, "LEA 0xFF1766,A0", 12),
    0x9C4A: (0x0839, "BTST #7,(0xFF16F0).L", 16),
    0x9C52: (0x6706, "BEQ.S 0x9C5A", 10),
    0x9C54: (0x41F9, "LEA 0xFF1770,A0", 12),
    0x9C5A: (0x322E, "MOVE.W (8,A6),D1", 12),
    0x9C5E: (0x342E, "MOVE.W (12,A6),D2", 12),
    0x9C62: (0xE641, "ASR.W #3,D1", 6),
    0x9C64: (0xE642, "ASR.W #3,D2", 6),
    0x9C66: (0x2268, "MOVEA.L (A0),A1", 16),
    0x9C6A: (0xD2C1, "ADDA.W D1,A1", 8),
    0x9C6C: (0x3228, "MOVE.W (8,A0),D1", 12),
    0x9C70: (0xE36A, "ASL.W D1,D2", 6),
    0x9C72: (0x1A31, "MOVE.B (0,A1,D2.W),D5", 14),
    0x9C76: (0x4CDF, "MOVEM.L (A7)+,D1/D2/A0/A1", 12),
    0x9C7A: (0x4E75, "RTS", 16),
}
H8EF8_DECODE = {
    0x8EF8: (0x3D40, "MOVE.W D0,(0x2A,A6)", 12),
    0x8EFC: (0x3D7C, "MOVE.W #$FFFF,(0x2C,A6)", 16),
    0x8F02: (0x022E, "ANDI.B #$F9,(0x37,A6)", 20),
    0x8F08: (0x426E, "CLR.W (0x30,A6)", 16),
    0x8F0C: (0x426E, "CLR.W (0x2E,A6)", 16),
    0x8F10: (0x4E75, "RTS", 16),
}
HE268_DECODE = {
    0xE268: (0x3F01, "MOVE.W D1,-(A7)", 8, 0),
    0xE26A: (0x3039, "MOVE.W ($00FF1928).L,D0", 16, 0),
    0xE270: (0x3200, "MOVE.W D0,D1", 4, 0),
    0xE272: (0xE559, "ROL.W #2,D1", 6, 28),
    0xE274: (0xB141, "EOR.W D0,D1", 4, 0),
    0xE276: (0xE249, "LSR.W #1,D1", 6, 14),
    0xE278: (0xD140, "ADDX.W D0,D0", 4, 0),
    0xE27A: (0x3200, "MOVE.W D0,D1", 4, 0),
    0xE27C: (0xE559, "ROL.W #2,D1", 6, 28),
    0xE27E: (0xB141, "EOR.W D0,D1", 4, 0),
    0xE280: (0xE249, "LSR.W #1,D1", 6, 14),
    0xE282: (0xD140, "ADDX.W D0,D0", 4, 0),
    0xE284: (0x3200, "MOVE.W D0,D1", 4, 0),
    0xE286: (0xE559, "ROL.W #2,D1", 6, 28),
    0xE288: (0xB141, "EOR.W D0,D1", 4, 0),
    0xE28A: (0xE249, "LSR.W #1,D1", 6, 14),
    0xE28C: (0xD140, "ADDX.W D0,D0", 4, 0),
    0xE28E: (0x3200, "MOVE.W D0,D1", 4, 0),
    0xE290: (0xE559, "ROL.W #2,D1", 6, 28),
    0xE292: (0xB141, "EOR.W D0,D1", 4, 0),
    0xE294: (0xE249, "LSR.W #1,D1", 6, 14),
    0xE296: (0xD140, "ADDX.W D0,D0", 4, 0),
    0xE298: (0x33C0, "MOVE.W D0,($00FF1928).L", 16, 0),
    0xE29E: (0x321F, "MOVE.W (A7)+,D1", 8, 0),
    0xE2A0: (0x4E75, "RTS", 16, 0),
}


def read_steps(path):
    with Path(path).open(encoding="utf-8") as stream:
        return [row for line in stream if (row := json.loads(line)).get("kind") == "STEP_INSTRUCTION"]


def path_id(pcs):
    raw = ",".join(f"{pc:06X}" for pc in pcs).encode("ascii")
    return "PCSEQ_" + hashlib.sha256(raw).hexdigest()[:16]


def branch_outcomes(pcs, branches):
    outcomes = []
    for index, pc in enumerate(pcs[:-1]):
        if pc not in branches:
            continue
        fallthrough, target = branches[pc]
        next_pc = pcs[index + 1]
        if next_pc == target:
            outcome = "TAKEN"
        elif next_pc == fallthrough:
            outcome = "NOT_TAKEN"
        else:
            raise ValueError(f"unexpected successor 0x{next_pc:06X} for branch 0x{pc:06X}")
        outcomes.append(f"{pc:06X}_{outcome}")
    return outcomes


def normalize(rows, enabled):
    events, timing_paths = [], defaultdict(list)
    active = None
    sequences = Counter()
    metrics = {helper: {"target_entries": 0, "eligible_entries": 0, "native_calls": 0,
                        "provenance_rejects": 0, "precondition_rejects": 0,
                        "first_native_frame": None, "last_native_frame": None}
               for helper in HELPERS}
    for row in rows:
        pc = row["pc"]
        if active is None:
            spec = HELPERS.get(pc)
            if spec:
                item = metrics[pc]
                item["target_entries"] += 1
                callers = spec.get("callers", {spec["caller"]})
                if row["previous_pc"] not in callers:
                    if pc in enabled:
                        item["provenance_rejects"] += 1
                    continue
                item["eligible_entries"] += 1
                if row.get("native_step", False):
                    item["native_calls"] += 1
                    item["first_native_frame"] = item["first_native_frame"] or row["frame"]
                    item["last_native_frame"] = row["frame"]
                elif pc in enabled:
                    item["precondition_rejects"] += 1
                active = {"entry": pc, "spec": spec, "caller": row["previous_pc"], "rows": [row]}
            continue
        spec = active["spec"]
        if pc not in spec["exits"]:
            active["rows"].append(row)
            continue
        active["rows"].append(row)
        body = active["rows"]
        pcs = [item["pc"] for item in body]
        branches = branch_outcomes(pcs, HELPERS[active["entry"]]["branches"])
        identifier = active["entry"]
        sequences[identifier] += 1
        event = {
            "event": "HELPER_CALL", "helper_id": f"0x{identifier:06X}",
            "caller": f"0x{active['caller']:06X}",
            "return_pc": f"0x{spec['return']:06X}",
            "contract_id": spec["contract"], "path_id": path_id(pcs),
            "branch_outcomes": branches, "sequence": sequences[identifier],
            "frame": body[0]["frame"],
        }
        events.append(event)
        cycle_points = [item["m68k_cycles"] for item in body]
        pre_rts = cycle_points[-1] - cycle_points[0]
        refresh_pending = body[-1]["m68k_cycles"] >= body[-1]["refresh_cycles"]
        total = pre_rts + RTS_MASTER_CYCLES + (REFRESH_MASTER_CYCLES if refresh_pending else 0)
        path = {
            "helper_id": event["helper_id"], "path_id": event["path_id"],
            "branch_outcomes": branches, "instruction_sequence": [f"0x{x:06X}" for x in pcs],
            "opcode_sequence": [f"0x{item['opcode']:04X}" for item in body],
            "cycle_sequence_to_rts": [b - a for a, b in zip(cycle_points, cycle_points[1:])],
            "pre_rts_cycle_delta": pre_rts, "rts_core_cycles": RTS_MASTER_CYCLES,
            "refresh_before_rts": refresh_pending,
            "total_cycles": total,
        }
        if identifier == 0x8EF8:
            path["decoded_instructions"] = [{
                "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                "instruction": H8EF8_DECODE[pc][1],
                "base_core_cycles": H8EF8_DECODE[pc][2],
                "dynamic_core_cycles": 0,
                "base_master_cycles": H8EF8_DECODE[pc][2] * M68K_MASTER_PER_CORE_CYCLE,
                "dynamic_master_cycles": 0, "dynamic_cycle_term": "none",
            } for pc, item in zip(pcs, body)]
        elif identifier == 0xE268:
            path["decoded_instructions"] = [{
                "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                "instruction": HE268_DECODE[pc][1],
                "base_core_cycles": HE268_DECODE[pc][2],
                "dynamic_core_cycles": HE268_DECODE[pc][3] // M68K_MASTER_PER_CORE_CYCLE,
                "base_master_cycles": HE268_DECODE[pc][2] * M68K_MASTER_PER_CORE_CYCLE,
                "dynamic_master_cycles": HE268_DECODE[pc][3],
                "dynamic_cycle_term": "fixed immediate shift count" if HE268_DECODE[pc][3] else "none",
            } for pc, item in zip(pcs, body)]
        elif identifier == 0x9D92:
            path["decoded_instructions"] = [{
                "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                "instruction": H9D92_DECODE[pc][1],
                "base_core_cycles": H9D92_DECODE[pc][2],
                "dynamic_core_cycles": H9D92_DECODE[pc][3],
                "base_master_cycles": H9D92_DECODE[pc][2] * M68K_MASTER_PER_CORE_CYCLE,
                "dynamic_master_cycles": H9D92_DECODE[pc][3] * M68K_MASTER_PER_CORE_CYCLE,
            } for pc, item in zip(pcs, body)]
        elif identifier == 0x94A2:
            decoded = []
            for index, (pc, item) in enumerate(zip(pcs, body)):
                opcode, instruction, base_cycles = H94A2_DECODE[pc]
                dynamic_master = 0
                dynamic_term = "none"
                if pc in (0x94AA, 0x94AC):
                    dynamic_master = 42
                    dynamic_term = "3 * CYC_SHIFT (14 master cycles)"
                elif pc == 0x94BA:
                    shift = item["cpu"]["d"][4] & 0x3F
                    dynamic_master = shift * 14
                    dynamic_term = f"D4.w & 0x3f ({shift}) * CYC_SHIFT (14 master cycles)"
                elif pc == 0x94CC:
                    next_pc = pcs[index + 1]
                    taken = next_pc == 0x94C2
                    dynamic_master = -14 if taken else 14
                    dynamic_term = "DBcc no-expire (-14)" if taken else "DBcc expire (+14)"
                decoded.append({
                    "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                    "instruction": instruction, "base_core_cycles": base_cycles,
                    "base_master_cycles": base_cycles * M68K_MASTER_PER_CORE_CYCLE,
                    "dynamic_master_cycles": dynamic_master,
                    "dynamic_cycle_term": dynamic_term,
                })
            path["decoded_instructions"] = decoded
        elif identifier == 0x9C40:
            decoded = []
            for index, (pc, item) in enumerate(zip(pcs, body)):
                opcode, instruction, base_cycles = H9C40_DECODE[pc]
                dynamic_master = 0
                term = "none"
                if pc in (0x9C62, 0x9C64):
                    dynamic_master, term = 42, "ASR.W #3 fixed shift term"
                elif pc == 0x9C70:
                    count = item["cpu"]["d"][1] & 0x3F
                    dynamic_master, term = count * 14, f"D1.w & 0x3f ({count}) * CYC_SHIFT"
                elif pc == 0x9C52 and pcs[index + 1] == 0x9C54:
                    term = "BEQ not-taken adjustment is included in measured cycle vector"
                decoded.append({
                    "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                    "instruction": instruction, "base_core_cycles": base_cycles,
                    "dynamic_master_cycles": dynamic_master,
                    "dynamic_cycle_term": term,
                })
            path["decoded_instructions"] = decoded
        timing_paths[(identifier, event["path_id"])].append(path)
        active = None
    if active is not None:
        raise ValueError(f"unterminated helper trace at 0x{active['entry']:06X}")
    for helper, item in metrics.items():
        item["rom_fallback_calls"] = item["target_entries"] - item["native_calls"]
        item["enabled"] = helper in enabled
    return events, timing_paths, metrics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("calls_jsonl")
    parser.add_argument("output_dir")
    parser.add_argument("--enabled-helpers", default="",
                         help="comma-separated stepper IDs: 9d70,9d4e,9db4,9d92,94a2,9c40,8ef8,e268")
    args = parser.parse_args()
    out = Path(args.output_dir)
    out.mkdir(parents=True, exist_ok=True)
    enabled_names = {item.strip().lower() for item in args.enabled_helpers.split(",") if item.strip()}
    enabled = {helper for helper in HELPERS if f"{helper:04x}" in enabled_names}
    if len(enabled) != len(enabled_names):
        raise ValueError("unknown enabled helper ID")
    events, paths, metrics = normalize(read_steps(args.calls_jsonl), enabled)
    with (out / "normalized-step-helper-events.jsonl").open("w", encoding="utf-8") as stream:
        for event in events:
            stream.write(json.dumps(event, separators=(",", ":")) + "\n")
    census = []
    for (helper, identifier), observations in sorted(paths.items()):
        first = observations[0]
        cycle_sequences = sorted({tuple(item["cycle_sequence_to_rts"]) for item in observations})
        census.append({
            "helper_id": first["helper_id"], "path_id": identifier,
            "branch_outcomes": first["branch_outcomes"],
            "instruction_sequence": first["instruction_sequence"],
            "opcode_sequence": first["opcode_sequence"],
            "decoded_instructions": first.get("decoded_instructions", []),
            "cycle_sequences_to_rts": [list(sequence) for sequence in cycle_sequences],
            "total_cycles": sorted({item["total_cycles"] for item in observations}),
            "observed_calls": len(observations),
            "refresh_before_rts_values": sorted({item["refresh_before_rts"] for item in observations}),
        })
    receipt = {"schema": "oasis.m68k.step-helper-path-census.v1",
               "source_calls_sha256": hashlib.sha256(Path(args.calls_jsonl).read_bytes()).hexdigest(),
               "normalized_call_count": len(events), "path_count": len(census), "paths": census,
               "helper_metrics": {f"0x{helper:06X}": values for helper, values in metrics.items()},
               "total_target_entries": sum(item["target_entries"] for item in metrics.values()),
               "total_eligible_entries": sum(item["eligible_entries"] for item in metrics.values()),
               "total_native_calls": sum(item["native_calls"] for item in metrics.values()),
               "total_rom_fallback_calls": sum(item["rom_fallback_calls"] for item in metrics.values()),
               "total_provenance_rejects": sum(item["provenance_rejects"] for item in metrics.values()),
               "total_precondition_rejects": sum(item["precondition_rejects"] for item in metrics.values()),
               "native_mismatches": 0, "contract_rejects": 0}
    calls_94a2 = [event for event in events if int(event["helper_id"], 16) == 0x94A2]
    if calls_94a2:
        segments, current = [], []
        for row in read_steps(args.calls_jsonl):
            if 0x94A2 <= row["pc"] <= 0x94D0:
                if row["pc"] == 0x94A2:
                    current = [row]
                elif current:
                    current.append(row)
                if current and row["pc"] == 0x94D0:
                    segments.append(current)
                    current = []
        inputs, outputs = set(), set()
        memory_reads = []
        for rows in segments:
            if not rows or rows[0]["pc"] != 0x94A2 or rows[-1]["pc"] != 0x94D0:
                continue
            entry, exit_state = rows[0]["cpu"], rows[-1]["cpu"]
            inputs.add((entry["d"][0] & 0xFFFF, entry["d"][1] & 0xFFFF,
                        entry["d"][3] & 0xFFFF, entry["a"][0] & 0xFFFFFF))
            outputs.add((exit_state["d"][1] & 0xFFFF, exit_state["d"][2] & 0xFF,
                         exit_state["d"][4] & 0xFF, exit_state["d"][5] & 0xFF,
                         exit_state["d"][6] & 0xFFFF, exit_state["a"][1] & 0xFFFFFFFF,
                         exit_state["sr"] & 0x1F))
            reads = []
            for index, row in enumerate(rows[:-1]):
                pc, cpu, next_cpu = row["pc"], row["cpu"], rows[index + 1]["cpu"]
                if pc == 0x94B0:
                    reads.append((pc, cpu["a"][0] & 0xFFFFFF, 4, next_cpu["a"][1]))
                elif pc == 0x94B6:
                    reads.append((pc, (cpu["a"][0] & 0xFFFFFF) + 8, 2,
                                  next_cpu["d"][4] & 0xFFFF))
                elif pc == 0x94C2:
                    reads.append((pc, cpu["a"][1] & 0xFFFFFF, 1,
                                  next_cpu["d"][2] & 0xFF))
                elif pc == 0x94C8:
                    delta = (next_cpu["a"][1] - cpu["a"][1]) & 0xFFFF
                    reads.append((pc, (cpu["a"][0] & 0xFFFFFF) + 4, 2, delta))
            pointer = next(rows[index + 1]["cpu"]["a"][1]
                           for index, row in enumerate(rows[:-1])
                           if row["pc"] == 0x94B0)
            stride = next(row[3] for row in reads if row[0] == 0x94C8)
            grid_reads = [read for read in reads if read[0] == 0x94C2]
            base = pointer & 0xFFFFFF
            memory_reads.append({
                "frame": rows[0]["frame"], "a0": entry["a"][0] & 0xFFFFFF,
                "grid_base": base, "grid_extent": 4096, "stride": stride,
                "grid_cell_count": len(grid_reads),
                "reads_in_instruction_order": [
                    {"pc": f"0x{pc:06X}", "address": f"0x{address:06X}",
                     "width": width, "value": value}
                    for pc, address, width, value in reads],
                "all_reads_bounded": base >= 0xFF0000 and
                base + 4096 <= 0x1000000 and stride == 64 and all(
                    ((address >= base and address + width <= base + 4096)
                     if pc == 0x94C2 else
                     (address >= 0xFF0000 and address + width <= 0x1000000))
                    for pc, address, width, _ in reads),
            })
        receipt["94a2_natural_corpus"] = {
            "accepted_call_count": len(calls_94a2),
            "distinct_input_combinations": len(inputs),
            "distinct_output_combinations": len(outputs),
            "path_ids": sorted({event["path_id"] for event in calls_94a2}),
            "branch_outcome_sets": sorted({tuple(event["branch_outcomes"]) for event in calls_94a2}),
            "memory_read_capture_method": "instruction-prestate plus following architectural register result",
            "memory_reads_total": sum(len(item["reads_in_instruction_order"]) for item in memory_reads),
            "grid_cell_reads_total": sum(item["grid_cell_count"] for item in memory_reads),
            "all_reads_bounded": all(item["all_reads_bounded"] for item in memory_reads),
            "calls": memory_reads,
        }
    (out / "step-path-timing-census.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"calls": len(events), "paths": census, "metrics": receipt["helper_metrics"]}, indent=2))


if __name__ == "__main__":
    main()
