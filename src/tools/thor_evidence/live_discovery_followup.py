"""Rank persisted live novelties and create one focused register-slice request."""
import argparse
import hashlib
import json
import subprocess
from collections import Counter, defaultdict
from pathlib import Path


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
AF02_CONTRACT = bytes.fromhex(
    "362e0018206e001ad0c048403a10670000e8")


def load_raw(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line.strip()]


def rank_investigations(report, raw):
    events = [item for item in report["investigations_created"]
              if item["current_classification"] == "NEW_ROM_ACTIVITY"]
    by_pc = defaultdict(list)
    for item in events:
        by_pc[item["pc"]].append(item)
    ranked = []
    for pc, items in by_pc.items():
        addresses = sorted({item["rom_ram_address"] for item in items})
        deltas = [right - left for left, right in zip(addresses, addresses[1:])]
        mode_delta, mode_count = Counter(deltas).most_common(1)[0] if deltas else (None, 0)
        regularity = mode_count / len(deltas) if deltas else 0.0
        contract = pc == 0xAF22
        unknown_spans = [item["span"][1] - item["span"][0] for item in items
                         if item["span"][0] is not None]
        score = (1000 * regularity + 10 * len(addresses) +
                 min(200, max(0, max(addresses) - min(addresses) + 2) / 8) +
                 (500 if contract else 0))
        ranked.append({
            "pc": pc, "observations": len(items), "target_addresses": addresses,
            "target_span": [min(addresses), max(addresses) + 2],
            "dominant_stride": mode_delta, "stride_regularity": regularity,
            "unknown_span_bytes": max(unknown_spans, default=0),
            "static_contract_candidate": contract, "information_score": round(score, 3),
            "investigation_ids": [item["id"] for item in items],
        })
    return sorted(ranked, key=lambda item: (-item["information_score"], item["pc"]))


def static_request(candidate, rom_path, executable, report_path, text_path):
    pc = candidate["pc"]
    entry = 0xAF00 if pc == 0xAF22 else max(0, pc - 0x20)
    command = [executable, rom_path, str(report_path), str(text_path), hex(entry), "0x200"]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    static = json.loads(Path(report_path).read_text())
    if pc == 0xAF22:
        rom = Path(rom_path).read_bytes()
        if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
            raise ValueError("canonical ROM identity mismatch")
        if rom[0xAF02:0xAF02 + len(AF02_CONTRACT)] != AF02_CONTRACT:
            raise ValueError("AUTO63 static consumer contract changed")
        dependency = {
            "effective_address_register": "A0",
            "definition_pc": 0xAF06,
            "definition_form": "MOVEA.L 0x1A(A6),A0",
            "source_register": "A6",
            "source_offset": 0x1A,
            "consumer_pc": 0xAF20,
            "consumer_form": "MOVE.W (A0),D6",
            "loop_counter": "D3",
            "record_stride": 6,
            "static_root_status": "A6_INHERITED_AT_ENTRY",
        }
    else:
        dependency = {"static_root_status": "UNRESOLVED", "effective_address_register": None}
    return {
        "entry_point": entry, "command": command, "stdout": completed.stdout,
        "instruction_count": len(static.get("instructions", [])),
        "dependency": dependency,
        "static_report": str(Path(report_path).resolve()),
        "static_report_sha256": hashlib.sha256(Path(report_path).read_bytes()).hexdigest(),
    }


def lua_plan(candidate, static, output):
    addresses = candidate["target_addresses"]
    if candidate["pc"] == 0xAF22:
        # Keep the live callback narrow. The complete static slice is persisted
        # separately; only the dependency chain is needed at runtime.
        watched = [0xAF02, 0xAF06, 0xAF0A, 0xAF20, 0xAF22]
        plan = {
            "schema": "oasis.m68k.m12-focused-register-plan.v1",
            "focus_pc": candidate["pc"], "watched_exec": sorted(set(watched)),
            "rom_start": min(addresses), "rom_end": max(addresses) + 2,
            "source_register": "A6", "source_offset": 0x1A,
            "requested_frames": 20,
        }
    else:
        plan = {"schema": "oasis.m68k.m12-focused-register-plan.v1",
                "focus_pc": candidate["pc"], "watched_exec": [candidate["pc"]],
                "rom_start": min(addresses), "rom_end": max(addresses) + 2,
                "source_register": None, "source_offset": 0,
                "requested_frames": 20}
    lines = ["return {", f'  schema = "{plan["schema"]}",',
             f'  focus_pc = {plan["focus_pc"]},',
             f'  rom_start = {plan["rom_start"]},',
             f'  rom_end = {plan["rom_end"]},',
             f'  source_offset = {plan["source_offset"]},',
             f'  requested_frames = {plan["requested_frames"]},',
             "  watched_exec = {"]
    lines.extend(f"    [{pc}] = true," for pc in plan["watched_exec"])
    lines.extend(["  },", "}\n"])
    lua_bytes = "\n".join(lines).encode()
    plan["lua_sha256"] = hashlib.sha256(lua_bytes).hexdigest()
    Path(output).write_bytes(lua_bytes)
    return plan


def run(args):
    report = json.loads(Path(args.report).read_text())
    raw = load_raw(args.raw)
    ranked = rank_investigations(report, raw)
    if not ranked:
        raise ValueError("no persisted NEW_ROM_ACTIVITY investigations")
    selected = ranked[0]
    static = static_request(selected, args.rom, args.slice_executable,
                            args.static_report, args.static_text)
    plan = lua_plan(selected, static, args.lua)
    request = {
        "schema": "oasis.m68k.m12-focused-evidence-request.v1",
        "source_capture": str(Path(args.raw).resolve()),
        "source_report": str(Path(args.report).resolve()),
        "ranking": ranked,
        "selected": selected,
        "known_before": {
            "auto62_investigation_count": len(report["investigations_created"]),
            "source_owned_change": report["source_owned_change"],
        },
        "missing": "register-based A0 source and inherited A6 definition",
        "question": "Does A6+0x1A define A0 for the 0xAF20 ROM read, what D3 count is active, and where is A6 established?",
        "static_pre_analysis": static,
        "watch_plan": plan,
        "expected_resolution": "prove RAM-source-read -> A0 definition -> ROM-read ordering, or retain A6 as an explicit unresolved frontier",
        "status": "REQUESTED",
    }
    Path(args.output).write_text(json.dumps(request, indent=2) + "\n")
    print(json.dumps(request, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", required=True)
    parser.add_argument("--raw", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--slice-executable", required=True)
    parser.add_argument("--static-report", required=True)
    parser.add_argument("--static-text", required=True)
    parser.add_argument("--lua", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
