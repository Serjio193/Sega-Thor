"""Build a small machine-readable V0 capability matrix from local artifacts."""
import argparse
import hashlib
import json
from pathlib import Path

from .events import event_stream_hash, read_capture
from .identity import canonical


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def sealed(root, name, raw_name=None):
    path = root / (name + ".sealed.jsonl")
    header, events, trace_hash, raw_hash = read_capture(path)
    return {"path": str(path), "sha256": file_hash(path), "trace_sha256": trace_hash,
            "event_stream_sha256": event_stream_hash(events), "events": len(events),
            "header": header}


def raw(root, name):
    path = root / (name + ".raw.jsonl")
    return {"path": str(path), "sha256": file_hash(path), "bytes": path.stat().st_size}


def build(root):
    root = Path(root)
    sources = root / "source"
    artifacts = {"minimal-v0": "minimal-a", "probe-v0-a": "probe-a",
                 "probe-v0-b": "probe-b", "reverse-v0": "reverse-a",
                 "uninstrumented-v0": "uninstrumented-a"}
    return {
        "schema": "thor.evidence.capability-matrix.v0",
        "status_vocabulary": ["PROVEN", "OBSERVED", "UNSUPPORTED", "UNKNOWN", "CONFLICT"],
        "artifacts": {sealed_name: {"raw": raw(root, raw_name),
                                     "sealed": sealed(root, sealed_name)}
                      for sealed_name, raw_name in artifacts.items()},
        "source_hashes": {path.name: file_hash(path) for path in sorted(sources.glob("*.cs"))},
        "capabilities": [
            {"id": "EXEC_PHASE", "status": "PROVEN", "scope": "BizHawk 2.11.1 API",
             "conclusion": "execution callback is documented immediately before execution"},
            {"id": "EXEC_PC", "status": "PROVEN", "scope": "selected M68K sites",
             "conclusion": "callback register PC matched watched execution address, including A372"},
            {"id": "WRITE_PHASE", "status": "PROVEN", "scope": "BizHawk 2.11.1 API",
             "conclusion": "write callback is documented immediately before the write"},
            {"id": "WRITE_PC_PAIRING", "status": "OBSERVED", "scope": "A372/FF13CC",
             "conclusion": "register PC was A374 for the A372-associated FF13CC callback; no universal subtraction rule"},
            {"id": "READ_PHASE", "status": "PROVEN", "scope": "BizHawk 2.11.1 API",
             "conclusion": "read callback is documented immediately before the read"},
            {"id": "READ_PC", "status": "OBSERVED", "scope": "selected RAM/ROM addresses",
             "conclusion": "register PC was captured at each read callback; complete pairing contract is not generalized"},
            {"id": "ACCESS_WIDTH", "status": "UNKNOWN", "scope": "GPGX 2.11.1 callback",
             "conclusion": "all live flags contained access bits only; size bits were absent"},
            {"id": "OVERLAP_RANGE", "status": "UNKNOWN", "scope": "exact-address hooks",
             "conclusion": "exact watched starts fired, but callback width/range matching cannot prove a long overlap"},
            {"id": "SAME_VALUE_WRITE", "status": "UNKNOWN", "scope": "bounded probe",
             "conclusion": "repeated callbacks were observed, but pre-write values were not an independent same-value oracle"},
            {"id": "HOOK_ORDER", "status": "PROVEN", "scope": "duplicate FF13CC hook",
             "conclusion": "callbacks followed installation order; reverse control changed w0/duplicate-start order"},
            {"id": "RESTORE_EPOCH", "status": "PROVEN", "scope": "same-process two-epoch probe",
             "conclusion": "hooks survived restore and collector emitted separate epochs 1 and 2"},
            {"id": "PEEK_REENTRY", "status": "OBSERVED", "scope": "selected hooks",
             "conclusion": "memory.read_bytes_as_array peeks caused zero matching callback reentries in this probe"},
            {"id": "IRQ_EXCEPTION", "status": "UNKNOWN", "scope": "V0 budget",
             "conclusion": "no bounded independent IRQ/exception experiment was run"},
            {"id": "INPUT_POLL", "status": "PROVEN", "scope": "measured frame 2120→2121",
             "conclusion": "zero input-poll callbacks; Right installation is not a causal game-read proof"},
        ],
        "controls": {
            "probe_raw_identical": file_hash(root / "probe-a.raw.jsonl") == file_hash(root / "probe-b.raw.jsonl"),
            "probe_event_stream_identical": sealed(root, "probe-v0-a")["event_stream_sha256"] == sealed(root, "probe-v0-b")["event_stream_sha256"],
            "reverse_event_stream_differs": sealed(root, "probe-v0-a")["event_stream_sha256"] != sealed(root, "reverse-v0")["event_stream_sha256"],
            "oracle_sat": "0088090187810088",
            "oracle_fields": "00100080",
            "source_owned_delta": 0,
        },
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root")
    parser.add_argument("output")
    args = parser.parse_args(argv)
    payload = build(args.root)
    Path(args.output).write_text(canonical(payload) + "\n", encoding="utf-8")
    print(hashlib.sha256(canonical(payload).encode()).hexdigest())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
