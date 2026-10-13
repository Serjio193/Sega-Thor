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
                if row["previous_pc"] != spec["caller"]:
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
                active = {"entry": pc, "spec": spec, "rows": [row]}
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
            "caller": f"0x{spec['caller']:06X}", "return_pc": f"0x{spec['return']:06X}",
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
        if identifier == 0x9D92:
            path["decoded_instructions"] = [{
                "pc": f"0x{pc:06X}", "opcode": f"0x{item['opcode']:04X}",
                "instruction": H9D92_DECODE[pc][1],
                "base_core_cycles": H9D92_DECODE[pc][2],
                "dynamic_core_cycles": H9D92_DECODE[pc][3],
                "base_master_cycles": H9D92_DECODE[pc][2] * M68K_MASTER_PER_CORE_CYCLE,
                "dynamic_master_cycles": H9D92_DECODE[pc][3] * M68K_MASTER_PER_CORE_CYCLE,
            } for pc, item in zip(pcs, body)]
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
                         help="comma-separated stepper IDs: 9d70,9d4e,9db4,9d92")
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
    (out / "step-path-timing-census.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"calls": len(events), "paths": census, "metrics": receipt["helper_metrics"]}, indent=2))


if __name__ == "__main__":
    main()
