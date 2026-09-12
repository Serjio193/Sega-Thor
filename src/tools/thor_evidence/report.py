"""Build an evidence-backed V0.1 capability and precondition matrix."""
import argparse
import hashlib
import json
from pathlib import Path

from .events import event_stream_hash, read_capture
from .identity import canonical
from .normalize import records

EXPECTED_SAT, EXPECTED_FIELDS = "0088090187810088", "00100080"
SOURCE_REQUIREMENTS = {
    "EXEC_PHASE": ("EventsLuaLibrary.cs", "immediately before the given address is executed"),
    "WRITE_PHASE": ("EventsLuaLibrary.cs", "immediately before the given address is written"),
    "READ_PHASE": ("EventsLuaLibrary.cs", "immediately before the given address is read"),
}


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def sealed(root, name):
    path = root / (name + ".sealed.jsonl")
    if not path.is_file():
        return {"path": str(path), "classification": "MISSING"}
    try:
        header, events, trace_hash, _ = read_capture(path)
    except (ValueError, json.JSONDecodeError, UnicodeDecodeError) as error:
        return {"path": str(path), "sha256": file_hash(path),
                "classification": "VALID_ONLY_FOR_V0_HISTORICAL_EVIDENCE",
                "reason": str(error)}
    return {"path": str(path), "sha256": file_hash(path), "trace_sha256": trace_hash,
            "event_stream_sha256": event_stream_hash(events), "events": len(events),
            "header": header, "classification": "VALID_FOR_V1"}


def raw(root, name):
    path = root / (name + ".raw.jsonl")
    if not path.is_file():
        return {"path": str(path), "classification": "MISSING"}
    return {"path": str(path), "sha256": file_hash(path), "bytes": path.stat().st_size}


def raw_event_hash(root, name):
    try:
        _, events, _ = records(root / (name + ".raw.jsonl"))
    except (OSError, ValueError, json.JSONDecodeError):
        return None
    return hashlib.sha256((canonical(events) + "\n").encode()).hexdigest()


def _source_receipts(root):
    source = root / "source"
    result = {filename: {"phrases": []} for filename, _ in SOURCE_REQUIREMENTS.values()}
    for filename, phrase in SOURCE_REQUIREMENTS.values():
        path = source / filename
        if path.is_file() and phrase in path.read_text(encoding="utf-8"):
            result[filename]["phrases"].append(phrase)
            result[filename]["sha256"] = file_hash(path)
    return {name: receipt for name, receipt in result.items() if receipt.get("sha256")}


def _valid(root, name, source_name=None):
    source_name = source_name or name
    item = sealed(root, source_name)
    if item.get("classification") != "VALID_FOR_V1":
        return item, []
    lines = [json.loads(line) for line in (root / (source_name + ".sealed.jsonl")).read_text().splitlines()]
    return item, lines[1:-1]


def _capability(cap_id, status, scope, conclusion, witnesses):
    return {"id": cap_id, "status": status, "scope": scope,
            "conclusion": conclusion, "witnesses": witnesses}


def build(root):
    root = Path(root)
    source = _source_receipts(root)
    names = {"minimal-v0": ("minimal-a", "minimal-v0"), "probe-v0-a": ("probe-a", "probe-v0-a"),
             "probe-v0-b": ("probe-b", "probe-v0-b"), "reverse-v0": ("reverse-a", "reverse-v0"),
             "uninstrumented-v0": ("uninstrumented-a", "uninstrumented-v0"),
             "probe-v01-a": ("v01r3-probe-a", "v01r3-probe-a"),
             "probe-v01-b": ("v01r3-probe-b", "v01r3-probe-b"),
             "reverse-v01-c": ("v01r3-reverse", "v01r3-reverse")}
    artifacts = {}
    captures = {}
    for sealed_name, (raw_name, source_name) in names.items():
        item, events = _valid(root, sealed_name, source_name)
        artifacts[sealed_name] = {"raw": raw(root, raw_name), "sealed": item}
        captures[sealed_name] = events
    a, ae = captures["probe-v01-a"], captures["probe-v01-a"]
    b, be = captures["probe-v01-b"], captures["probe-v01-b"]
    reverse, revents = captures["reverse-v01-c"], captures["reverse-v01-c"]
    valid_a = bool(a)
    oracle_events = [event for event in ae if event["kind"] == "ORACLE"]
    oracle_ok = valid_a and len(oracle_events) == 2 and all(
        event["data"].get("sat") == EXPECTED_SAT and event["data"].get("fields") == EXPECTED_FIELDS
        for event in oracle_events)
    execs = [event for event in ae if event["kind"] == "EXEC"]
    writes = [event for event in ae if event["kind"] == "WRITE"]
    reads = [event for event in ae if event["kind"] == "READ"]
    exec_a372 = [event for event in execs if event["data"].get("address") == 0xA372]
    ff_writes = [event for event in writes if event["data"].get("address") == 0xFF13CC]
    order_a = [event["data"].get("hook") for event in ae
               if event["kind"] == "WRITE" and event["data"].get("address") == 0xFF13CC]
    order_r = [event["data"].get("hook") for event in revents
               if event["kind"] == "WRITE" and event["data"].get("address") == 0xFF13CC]
    source_witness = [f"source/{name}" for name in source]
    capabilities = [
        _capability("EXEC_PHASE", "PROVEN" if source.get("EventsLuaLibrary.cs") and execs else "UNKNOWN",
                    "BizHawk API + V0.1 capture", "pre-execution contract and runtime EXEC witness",
                    source_witness + ["probe-v01-a EXEC"] if execs else source_witness),
        _capability("EXEC_PC", "PROVEN" if exec_a372 else "UNKNOWN", "selected M68K sites",
                    "A372 callback PC equals watched address" if exec_a372 else "no A372 witness",
                    ["probe-v01-a EXEC A372"] if exec_a372 else []),
        _capability("WRITE_PHASE", "PROVEN" if source.get("EventsLuaLibrary.cs") and ff_writes else "UNKNOWN",
                    "BizHawk API + FF13CC", "pre-write contract and FF13CC witness",
                    source_witness + ["probe-v01-a WRITE FF13CC"] if ff_writes else source_witness),
        _capability("WRITE_PC_PAIRING", "OBSERVED" if any(e["data"].get("pc") == 0xA374 for e in ff_writes) else "UNKNOWN",
                    "A372/FF13CC", "A374 observed; no universal PC-2 rule", ["probe-v01-a WRITE"] if ff_writes else []),
        _capability("READ_PHASE", "PROVEN" if source.get("EventsLuaLibrary.cs") and reads else "UNKNOWN",
                    "BizHawk API + selected reads", "pre-read contract and runtime READ witness",
                    source_witness + ["probe-v01-a READ"] if reads else source_witness),
        _capability("ACCESS_WIDTH", "UNKNOWN", "GPGX callback", "size flags are absent", []),
        _capability("OVERLAP_RANGE", "UNKNOWN", "exact-address hooks", "selected starts do not prove overlap", []),
        _capability("SAME_VALUE_WRITE", "UNKNOWN", "bounded probe", "no independent before-value writer oracle", []),
        _capability("HOOK_ORDER", "PROVEN" if order_a and order_r and order_a != order_r else "UNKNOWN",
                    "duplicate FF13CC hook", "reversed installation changes observed order",
                    ["probe-v01-a order", "reverse-v01 order"] if order_a and order_r and order_a != order_r else []),
        _capability("RESTORE_EPOCH", "PROVEN" if len({e["epoch"] for e in ae}) == 2 else "UNKNOWN",
                    "same-process restore", "two distinct epochs in corrected capture", ["probe-v01-a epochs"] if ae else []),
        _capability("PEEK_REENTRY", "OBSERVED" if any(e["kind"] == "PEEK" for e in ae) else "UNKNOWN",
                    "selected hooks", "peek result is present; re-entry is scope-limited", ["probe-v01-a PEEK"] if ae else []),
        _capability("IRQ_EXCEPTION", "UNKNOWN", "V0.1 budget", "not measured", []),
        _capability("INPUT_POLL", "PROVEN" if oracle_ok and all(e["data"].get("input_polls") == 0 for e in oracle_events) else "UNKNOWN",
                    "measured frame", "zero input-poll callbacks; no causal input claim", ["probe-v01-a ORACLE"] if oracle_ok else []),
    ]
    return {"schema": "thor.evidence.capability-matrix.v0.1", "status_vocabulary":
            ["PROVEN", "OBSERVED", "UNSUPPORTED", "UNKNOWN", "CONFLICT", "ERROR"],
            "artifacts": artifacts, "source_receipts": source, "capabilities": capabilities,
            "controls": {"probe_raw_identical": raw_event_hash(root, "v01r3-probe-a") == raw_event_hash(root, "v01r3-probe-b") and valid_a and bool(b),
                         "probe_event_stream_identical": bool(a) and bool(b) and artifacts["probe-v01-a"]["sealed"].get("event_stream_sha256") == artifacts["probe-v01-b"]["sealed"].get("event_stream_sha256"),
                         "reverse_event_stream_differs": bool(a) and bool(reverse) and artifacts["probe-v01-a"]["sealed"].get("event_stream_sha256") != artifacts["reverse-v01-c"]["sealed"].get("event_stream_sha256"),
                         "oracle_matches_expected": oracle_ok,
                         "oracle_sat": oracle_events[0]["data"].get("sat") if oracle_events else None,
                         "oracle_fields": oracle_events[0]["data"].get("fields") if oracle_events else None,
                         "source_owned_delta": 0},
            "v1_preconditions": {"instruction_instances": "SAFE_ALTERNATIVE" if exec_a372 else "BLOCKED",
                                 "a372_write_pairing": "BLOCKED", "selected_operation_width": "SAFE_ALTERNATIVE",
                                 "relevant_write_completeness": "BLOCKED", "interruption_boundary": "BLOCKED",
                                 "input_causality": "UNKNOWN_ALLOWED", "pre_savestate": "UNKNOWN_ALLOWED",
                                 "dma_vram": "UNKNOWN_ALLOWED"}}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root"); parser.add_argument("output")
    args = parser.parse_args(argv)
    payload = build(args.root)
    Path(args.output).write_text(canonical(payload) + "\n", encoding="utf-8")
    print(hashlib.sha256(canonical(payload).encode()).hexdigest())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
