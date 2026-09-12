"""Build an exact direct-xref census for the 0x00D406 graphics loader."""
import argparse
import hashlib
import json
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-loader-census.v1"
SCREEN_SCHEMA = "oasis.m68k.screen-resource-boundary.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = 0x00D406
CALL_BYTES = b"\x4E\xB9\x00\x00\xD4\x06"
UNRESOLVED_NOTES = {
    0x038FD6: {
        "reason": "CODE_LIKE_ENTRY_WITHOUT_BOUNDED_D406_CONTINUATION",
        "observed_prefix": "MOVEM.L D0/D1/D2/D3/D4,-(A7) followed by RAM state updates",
        "requires": "closed parent/caller path or targeted runtime provenance",
    },
    0x03959A: {
        "reason": "CODE_LIKE_ENTRY_WITH_UNBOUNDED_SIBLING_D406_CALL",
        "observed_prefix": "MOVEM.L and LEA.L $039DD2,A0 followed by RAM state checks",
        "sibling_d406_call": "0x0395D8",
        "requires": "caller/A1 provenance separating the entry from the sibling call",
    },
}
EXTENDED_SCREEN_CONTINUATIONS = {
    0x038FD6: {
        "call": 0x0390A4,
        "pre_call_sha256": "53a38241102d56692baa4176d467ed204ebc2ef6a6fbb21e429f7e1c6820cb71",
    },
    0x03959A: {
        "call": 0x0395D8,
        "pre_call_sha256": "d4d24eb99db0998278f3aec1b4ba0735dcf3299f4a59aed089ce6781ec9a258d",
    },
}


def direct_call_sites(rom: bytes) -> list[int]:
    """Return every exact absolute-long JSR to TARGET, including data false positives."""
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def screen_sites(screen_report: dict) -> list[dict]:
    if screen_report.get("schema") != SCREEN_SCHEMA:
        raise ValueError("unexpected screen-resource report schema")
    result = []
    for record in screen_report.get("records", []):
        for use in record.get("uses", []):
            descriptor = int(use["descriptor"])
            result.append({
                "group": int(use["group"]),
                "index": int(use["index"]),
                "descriptor": descriptor,
                "expected_call_site": descriptor + 0x1A,
                "stream": int(record["start"]),
            })
    return result


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def nearby_call(expected: int, calls: list[int], window: int = 16) -> dict | None:
    """Report bounded forward adjacency without proving control-flow or A1."""
    candidates = [site for site in calls if expected < site <= expected + window]
    if not candidates:
        return None
    site = min(candidates)
    return {
        "site": _hex(site),
        "distance": site - expected,
        "classification": "BOUNDED_FORWARD_ADJACENCY_CANDIDATE",
        "proof_limit": "adjacency does not prove A1 provenance or unconditional control flow",
    }


def verify_continuation(rom: bytes, start: int, call: int) -> dict | None:
    """Verify the known small 68000 bridge between a descriptor and D406.

    This is deliberately a closed decoder for the exact instruction forms
    present in the canonical ROM. Unknown opcodes fail closed.
    """
    pc = start
    instructions = []
    branches = []
    writes_a1 = False
    while pc < call:
        opcode = int.from_bytes(rom[pc:pc + 2], "big")
        if opcode == 0x7400 or opcode in (0x7200, 0x7208):
            destination = 2 if opcode == 0x7400 else 1
            instructions.append((pc, "moveq", destination))
            pc += 2
        elif opcode == 0x3401:
            instructions.append((pc, "move.w", 2))
            pc += 2
        elif opcode in (0x0442, 0x0642):
            instructions.append((pc, "immediate.w", 2))
            pc += 4
        elif opcode == 0x243C:
            instructions.append((pc, "move.l", 2))
            pc += 6
        elif opcode == 0x223C:
            instructions.append((pc, "move.l", 1))
            pc += 6
        elif opcode == 0x23FC:
            address = int.from_bytes(rom[pc + 6:pc + 10], "big")
            if address == 0x00FF17B2:
                instructions.append((pc, "move.l.absolute", address))
                pc += 10
            else:
                return None
        elif opcode == 0x0281:
            instructions.append((pc, "andi", 1))
            pc += 6
        elif opcode == 0x6A04:
            target = pc + 2 + 4
            if target != call:
                return None
            branches.append({"site": _hex(pc), "target": _hex(target),
                             "kind": "conditional_to_call"})
            pc += 2
        else:
            return None
    if pc != call or rom[call:call + len(CALL_BYTES)] != CALL_BYTES:
        return None
    return {
        "site": _hex(call),
        "classification": "VERIFIED_SCREEN_DESCRIPTOR_CONTINUATION",
        "instruction_count_before_call": len(instructions),
        "branch_edges": branches,
        "control_flow": "all decoded paths reach the exact D406 call",
        "a1_written_before_call": writes_a1,
        "proof_limit": "inherits the screen-root descriptor/A1 entry contract; no ROM ownership promotion",
    }


def verify_slice_continuation(rom: bytes, slice_report: dict,
                              start: int, call: int) -> dict | None:
    """Verify a longer continuation using the exact bounded CFG report.

    The report is produced by the developer-only ``re_slice_decoder``. Calls
    are treated as returning to their fall-through instruction; conditional
    branches keep both successors. A return, unsupported instruction, or
    missing successor before the target fails closed.
    """
    if slice_report.get("schema") != "oasis.m68k.re-slice.v1":
        return None
    if int(slice_report.get("entry_point", "-1"), 16) != start:
        return None
    instructions = slice_report.get("instructions", [])
    by_address = {int(item["address"], 16): item for item in instructions}
    target_edges = [edge for edge in slice_report.get("direct_control_flow", [])
                    if int(edge["source"], 16) == call and
                    int(edge["target"], 16) == TARGET]
    target_instruction = by_address.get(call)
    if not target_edges or target_instruction is None:
        return None
    if target_instruction.get("bytes", "").lower() != CALL_BYTES.hex():
        return None
    expected = EXTENDED_SCREEN_CONTINUATIONS.get(start)
    if expected is None or expected["call"] != call:
        return None
    if hashlib.sha256(rom[start:call]).hexdigest() != expected["pre_call_sha256"]:
        return None

    edge_map = {int(edge["source"], 16): int(edge["target"], 16)
                for edge in slice_report.get("direct_control_flow", [])}
    reachable = set()
    pending = [start]
    reached_call = False
    while pending:
        address = pending.pop()
        if address == call:
            reached_call = True
            continue
        if address in reachable:
            continue
        instruction = by_address.get(address)
        if instruction is None or not instruction.get("supported", False):
            return None
        if instruction.get("flow") == "return":
            return None
        reachable.add(address)
        next_address = address + len(bytes.fromhex(instruction["bytes"]))
        if instruction.get("flow") == "direct_branch":
            if instruction.get("mnemonic", "").lower() not in ("bra", "jmp"):
                pending.append(next_address)
            branch_target = edge_map.get(address)
            if branch_target is not None:
                pending.append(branch_target)
        elif next_address in by_address:
            pending.append(next_address)
        elif instruction.get("flow") != "direct_call":
            return None
        if any(successor != call and successor not in by_address
               for successor in pending):
            return None
    if not reached_call:
        return None
    return {
        "site": _hex(call),
        "classification": "VERIFIED_SCREEN_DESCRIPTOR_EXTENDED_CONTINUATION",
        "instruction_count_before_call": len(reachable),
        "control_flow": "exact re_slice CFG reaches the D406 call on every reachable path",
        "a1_written_before_call": False,
        "pre_call_sha256": expected["pre_call_sha256"],
        "proof_limit": "inherits the screen-root descriptor/A1 entry contract; no ROM ownership promotion",
    }


def descriptor_shaped_calls(rom: bytes, calls: list[int], screen_calls: set[int]) -> list[dict]:
    """Find unmatched calls whose preceding 26 bytes match the D406 record shape."""
    result = []
    for site in calls:
        if site in screen_calls or site < 0x1A:
            continue
        record = site - 0x1A
        pointer = int.from_bytes(rom[record + 4:record + 8], "big")
        ids = tuple(rom[record + 8:record + 12])
        if pointer == 0 or pointer >= len(rom) or any(value >= 108 for value in ids):
            continue
        result.append({
            "call_site": _hex(site),
            "record_start": _hex(record),
            "record_end": _hex(site),
            "record_size": 0x1A,
            "rom_pointer": _hex(pointer),
            "resource_ids": list(ids),
            "classification": "D406_PRECEDING_DESCRIPTOR_SHAPED_CANDIDATE",
            "promotion": False,
            "proof_limit": "record shape and pointer do not prove caller A1 or exact family boundary",
        })
    return result


def build_report(rom: bytes, screen_report: dict,
                 slice_reports: dict[int, dict] | None = None) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    calls = direct_call_sites(rom)
    uses = screen_sites(screen_report)
    call_set = set(calls)
    expected = [use["expected_call_site"] for use in uses]
    expected_set = set(expected)
    matched = sorted(call_set & expected_set)
    unmatched = sorted(call_set - expected_set)
    missing = [use for use in uses if use["expected_call_site"] not in call_set]
    missing_records = []
    for use in missing:
        item = {**use}
        item["expected_call_site"] = _hex(use["expected_call_site"])
        item["descriptor"] = _hex(use["descriptor"])
        item["stream"] = _hex(use["stream"])
        item["nearby_direct_call"] = nearby_call(use["expected_call_site"], calls)
        if item["nearby_direct_call"] is not None:
            call = int(item["nearby_direct_call"]["site"], 16)
            item["verified_continuation"] = verify_continuation(
                rom, use["expected_call_site"], call)
        else:
            item["verified_continuation"] = None
        if item["verified_continuation"] is None and slice_reports:
            extended = EXTENDED_SCREEN_CONTINUATIONS.get(
                use["expected_call_site"])
            if extended is not None:
                item["verified_continuation"] = verify_slice_continuation(
                    rom, slice_reports.get(use["expected_call_site"], {}),
                    use["expected_call_site"], extended["call"])
        if item["verified_continuation"] is None:
            item["unresolved_blocker"] = UNRESOLVED_NOTES.get(
                use["expected_call_site"], {
                    "reason": "NO_VERIFIED_CONTINUATION",
                    "requires": "new bounded control-flow evidence",
                })
        missing_records.append(item)
    verified = [item for item in missing_records
                if item["verified_continuation"] is not None]
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "rom_size": len(rom),
        "target": _hex(TARGET),
        "encoding": "JSR abs.l target bytes 4E B9 00 00 D4 06",
        "direct_call_count": len(calls),
        "direct_call_sites": [_hex(site) for site in calls],
        "screen_descriptor_use_count": len(uses),
        "screen_descriptor_unique_count": len({use["descriptor"] for use in uses}),
        "screen_descriptor_expected_call_count": len(expected),
        "screen_descriptor_expected_unique_call_count": len(expected_set),
        "screen_descriptor_direct_call_count": len(matched),
        "screen_descriptor_missing_direct_call_count": len(missing),
        "unmatched_direct_call_count": len(unmatched),
        "unmatched_direct_call_sites": [_hex(site) for site in unmatched],
        "screen_descriptor_direct_call_sites": [_hex(site) for site in matched],
        "screen_descriptor_missing_direct_calls": missing_records,
        "screen_descriptor_verified_continuation_count": len(verified),
        "screen_descriptor_unresolved_continuation_count": len(missing) - len(verified),
        "screen_descriptor_unresolved_blockers": [
            item for item in missing_records
            if item["verified_continuation"] is None],
        "unmatched_descriptor_shaped_candidates": descriptor_shaped_calls(
            rom, unmatched, set(matched)),
        "classification": {
            "screen_descriptor_direct": len(matched),
            "screen_descriptor_missing_direct": len(missing),
            "unmatched_direct": len(unmatched),
        },
        "promotion": {
            "performed": False,
            "reason": "xref census does not prove A1 provenance, ownership, or resource boundaries",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("screen_report", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--slice-report", nargs=2, action="append", metavar=("ENTRY", "PATH"),
        help="exact re_slice JSON for an extended screen continuation")
    args = parser.parse_args()
    slice_reports = {
        int(entry, 0): json.loads(Path(path).read_text())
        for entry, path in (args.slice_report or [])
    }
    report = build_report(
        args.rom.read_bytes(), json.loads(args.screen_report.read_text()),
        slice_reports)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        "direct_calls": report["direct_call_count"],
        "screen_direct": report["screen_descriptor_direct_call_count"],
        "screen_missing": report["screen_descriptor_missing_direct_call_count"],
        "unmatched": report["unmatched_direct_call_count"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
