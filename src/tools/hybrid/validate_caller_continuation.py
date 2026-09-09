"""Validate paired local M11.56 evidence. No ROM or run data belongs in Git."""
import json
import hashlib
import sys
from pathlib import Path


def require(value, message):
    if not value:
        raise ValueError(message)


def validate(first, second, rom_path):
    raw = (first / "caller_continuation.jsonl").read_bytes()
    require(raw == (second / "caller_continuation.jsonl").read_bytes(), "trace repeat mismatch")
    records = [json.loads(line) for line in raw.splitlines()]
    require(records[0] == {"schema": "oasis.hybrid.caller-continuation.v1",
                          "entries": 1, "complete": True, "truncated": False}, "capture incomplete")
    events = records[1:]
    execute = [event for event in events if event["type"] == 1]
    by_pc = {event["address"]: event for event in execute}
    start = next(i for i, event in enumerate(events) if event["type"] == 1
                 and event["address"] == 0x604F0)
    pcs = [0x604F0, 0x604F6, 0x604BC, 0x604C2, 0x604C8, 0x604CE, 0x604D4,
           0x604D8, 0x604DA, 0x604DC, 0x604DE, 0x604E4, 0x604FA, 0x60500,
           0x60506, 0x6050C, 0x60512, 0x611D6, 0x611D8, 0x611DC, 0x611DE, 0x424]
    require([e["address"] for e in events[start:] if e["type"] == 1] == pcs,
            "candidate/return execution path mismatch")
    require(by_pc[0x60004]["previous_execute"] == 0x41E, "parent caller mismatch")
    require(by_pc[0x604F0]["previous_execute"] == 0x604EA, "internal predecessor mismatch")
    rom = rom_path.read_bytes()
    require(hashlib.sha256(rom).hexdigest() ==
            "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263", "ROM identity mismatch")
    encodings = {0x41E: "4eb900060004", 0x60004: "60000424", 0x6042A: "40e7",
                 0x6042C: "007c0700", 0x60430: "48e77ffe", 0x604EA: "08ed00000000",
                 0x604F0: "51f900ff0012", 0x604F6: "6100ffc4", 0x604FA: "51f900ff0010",
                 0x60500: "51f900ff0011", 0x60506: "51f900ff0013", 0x6050C: "51f900ff0014",
                 0x60512: "60000cc2", 0x611D6: "7000", 0x611D8: "4cdf7ffe",
                 0x611DC: "46df", 0x611DE: "4e75"}
    for pc, encoding in encodings.items():
        require(rom[pc:pc + len(encoding) // 2].hex() == encoding, f"opcode mismatch {pc:X}")
    original = by_pc[0x60004]["registers"]
    entry = by_pc[0x604F0]["registers"]
    stack = entry[15]
    require(original[15] - stack == 58, "missing 56-byte save plus SR")
    require(entry[13] == 0xFF001A and stack == 0xFF0BB0, "natural address contract mismatch")
    saved = bytes(by_pc[0x604F0]["stack_bytes"])
    for i in range(14):
        require(int.from_bytes(saved[i * 4:i * 4 + 4], "big") == original[i + 1],
                f"saved register {i + 1} mismatch")
    require(int.from_bytes(saved[56:58], "big") == original[17] == 0x2114, "saved SR mismatch")
    require(int.from_bytes(saved[58:62], "big") == 0x424, "original return mismatch")
    returned = by_pc[0x424]["registers"]
    require(returned[0] == 0 and returned[1:15] == original[1:15], "enclosing register restore mismatch")
    require(returned[15] == original[15] + 4 and returned[17] == original[17], "enclosing A7/SR mismatch")
    require(by_pc[0x604BC]["registers"][15] == stack - 4, "nested call push mismatch")
    require(by_pc[0x604FA]["registers"][15] == stack, "nested return mismatch")
    journal = []
    current = 0
    for event in events[start:]:
        if event["type"] == 1:
            current = event["address"]
        if event["type"] in (2, 4):
            journal.append((current, event["type"], event["width"], event["address"], event["value"]))
    expected = [(0x604F0, 4, 1, 0xFF0012, 0), (0x604F6, 4, 4, stack - 4, 0x604FA)]
    for pc, address in ((0x604C2, 0xFF0628), (0x604CE, 0xFF06F2)):
        expected.extend([(pc, 2, 1, address, 0x10), (pc, 4, 1, address, 0x10)])
    expected.extend((pc, 4, 1, entry[13] + offset, 0)
                    for pc, offset in ((0x604D8, 5), (0x604DA, 6), (0x604DC, 7)))
    expected.extend([(0x604DE, 4, 1, 0xFF0016, 0), (0x604E4, 2, 4, stack - 4, 0x604FA)])
    expected.extend((pc, 4, 1, address, 0) for pc, address in
                    ((0x604FA, 0xFF0010), (0x60500, 0xFF0011), (0x60506, 0xFF0013), (0x6050C, 0xFF0014)))
    expected.extend((0x611D8, 2, 4, stack + i * 4, original[i + 1]) for i in range(14))
    # The pinned GPGX MOVEM performs an additional word read before the SR pop.
    expected.extend([(0x611D8, 2, 2, stack + 56, original[17]),
                     (0x611DC, 2, 2, stack + 56, original[17]),
                     (0x611DE, 2, 4, stack + 58, 0x424)])
    require(journal == expected, "ordered memory journal mismatch")
    for directory in (first, second):
        summary = json.loads((directory / "summary.json").read_text())
        require(summary["state_checkpoints_sha256"] ==
                "251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d", "checkpoint mismatch")
        require(summary["video_sequence_sha256"] ==
                "5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58", "video mismatch")
        require(summary["scenario_completed"] and summary["total_guest_instruction_executions"] == 6488773,
                "scenario/accounting mismatch")
    print(f"PASS: deterministic parent-frame restore, {len(pcs) - 1} path instructions, "
          f"{len(journal)} ordered accesses; THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: validate_caller_continuation.py <run-a> <run-b> <canonical-rom>")
    validate(*(Path(argument) for argument in sys.argv[1:]))
