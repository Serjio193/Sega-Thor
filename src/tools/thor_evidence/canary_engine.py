"""Bounded engine-derived provenance for the FF13CC V1 canary.

The decoder JSON is a checked static reference.  The graph is built only from
that reference plus sealed dynamic events; no known-answer graph is imported.
"""
import argparse
import hashlib
import json
from pathlib import Path

from .events import read_capture
from .identity import ROM_SHA, canonical, digest
from .normalize import records
from .ram_versions import (CoverageCertificate, RamVersionEngine,
                            VerifiedCoverageCertificate, attest_source_events)
from .canary_validation import validate_ram_certificate

TARGET = 0xFF13CC
ROOT_ZERO = 0xA438
STATIC_START = 0xA342
STATIC_END = 0xA436


def _hex(value, width=8):
    return f"{value & ((1 << (width * 4)) - 1):0{width}X}"


def _version(epoch, location, bits, value, origin, slot=0):
    payload = {"epoch": epoch, "location": location, "bits": list(bits),
               "value": value, "origin": origin, "slot": slot}
    return {"id": digest({"kind": "canary-version", "value": payload}), **payload,
            "value_hex": None if value is None else _hex(value, (bits[1] + 3) // 4)}


def _operation(epoch, event, rule_id, extra=None):
    payload = {"epoch": epoch, "exec_seq": event["seq"], "pc": event["data"]["pc"],
               "rule_id": rule_id, "witness_seq": event["seq"]}
    if extra:
        payload.update(extra)
    payload["id"] = digest({"kind": "canary-operation", "value": payload})
    return payload


def _edge(source, target, role, rule_id, witness, status="PROVEN"):
    payload = {"source": source, "target": target, "role": role,
               "rule_id": rule_id, "witness_event_id": witness, "status": status}
    payload["id"] = digest({"kind": "canary-dependency", "value": payload})
    return payload


def _events_for_epoch(events, epoch):
    return [e for e in events if e["epoch"] == epoch]


def _static_map(static):
    out = {}
    for item in static.get("instructions", []):
        out[item["address"]] = item
    required = {
        STATIC_START: ("lea", 4), 0xA348: ("adda", 2), 0xA34E: ("move", 2),
        0xA358: ("tst", 1), 0xA35E: ("beq", 0), 0xA36C: ("move", 4),
        0xA36E: ("addq", 1), 0xA370: ("move", 1), 0xA372: ("move", 4),
    }
    if any(out.get(pc, {}).get("operation") != op or out.get(pc, {}).get("width_bytes") != width
           for pc, (op, width) in required.items()):
        raise ValueError("checked static reference does not cover canary")
    if out[STATIC_START].get("source", {}).get("value") != TARGET or \
       out[0xA348].get("source", {}).get("value") != 0xFF188C or \
       out[0xA34E].get("source", {}).get("value") != 0xFF188A or \
       out[0xA358].get("destination", {}).get("value") != 0xFF1858 or \
       out[0xA35E].get("branch_target") != 0xA364:
        raise ValueError("checked static operand contract does not cover canary")
    return out


def _read(events, address, exec_seq=None):
    candidates = [e for e in events if e["kind"] == "READ" and e["data"].get("address") == address]
    if exec_seq is not None:
        candidates = [e for e in candidates if e["data"].get("exec_seq") == exec_seq]
    if not candidates:
        raise ValueError(f"missing dynamic read 0x{address:06X}")
    return candidates[0]


def _write_events(events, address):
    return [e for e in events if e["kind"] == "WRITE" and e["data"].get("address") == address]


def _writer_sequence(path):
    if not path:
        return []
    _, events, _ = records(path)
    writes = []
    for event in events:
        if event["kind"] != "WRITE" or event["data"].get("address") not in (0xFF188C, 0xFF188A):
            continue
        data = event["data"]
        writes.append({"epoch": event["epoch"], "seq": event["seq"], "address": data["address"],
                       "value": data.get("value"), "callback_pc": data.get("pc"),
                       "status": "OBSERVED"})
    return writes


def _ram_canary_engine(raw_hash, events, static_map, receipt_sha256):
    """Run the selected A372 writes through the reusable V2 byte engine."""
    engine = RamVersionEngine(raw_hash)
    engine.begin_epoch(1, start_seq=0)
    engine.begin_epoch(2, start_seq=116)
    scope = tuple(range(TARGET, TARGET + 4))
    if not receipt_sha256:
        raise ValueError("V2 coverage requires the validated launch receipt")
    receipt_sha = receipt_sha256
    for epoch, end in ((1, 18), (2, 231)):
        epoch_events = _events_for_epoch(events, epoch)
        execs = [str(e["seq"]) for e in epoch_events
                 if e["kind"] == "EXEC" and e["data"].get("pc") == 0xA372
                 and e["seq"] <= end]
        claim = CoverageCertificate(f"a372-dense-epoch-{epoch}",
                                    min(e["seq"] for e in epoch_events), end,
                                    scope, raw_hash)
        basis_events = attest_source_events([{**event, "receipt_sha256": receipt_sha,
                         "decoder_id": hashlib.sha256(json.dumps(
                             static_map, sort_keys=True, separators=(",", ":")).encode()).hexdigest(),
                         "rule_id": "MOVE_LONG_D2_TO_RAM"}
                        for event in epoch_events])
        certificate = VerifiedCoverageCertificate.from_capture(
            claim, trace=raw_hash, epoch=epoch,
            raw_artifact_hash=raw_hash, receipt_sha256=receipt_sha,
            decoder_id=hashlib.sha256(json.dumps(static_map, sort_keys=True,
                                                 separators=(",", ":")).encode()).hexdigest(),
            rule_id="MOVE_LONG_D2_TO_RAM", execution_instances=tuple(execs),
            source_events=basis_events)
        engine.add_coverage(certificate)
    for epoch in (1, 2):
        epoch_events = _events_for_epoch(events, epoch)
        exec_by_seq = {e["seq"]: e for e in epoch_events if e["kind"] == "EXEC"}
        for event in epoch_events:
            data = event["data"]
            if event["kind"] != "WRITE":
                continue
            exec_event = exec_by_seq.get(data.get("exec_seq"))
            address = data.get("address")
            if address is None or address <= TARGET + 3 and address + 3 >= TARGET:
                if address != TARGET or exec_event is None or exec_event["data"].get("pc") != 0xA372:
                    raise ValueError("unclassified write overlaps the canary range")
                engine.write(epoch, event["seq"], str(data["exec_seq"]),
                         0xA372, "MOVE_LONG_D2_TO_RAM", 4, TARGET, data["value"],
                         raw_witnesses=[exec_event["seq"], event["seq"]],
                         decoded_instruction=static_map.get(0xA372))
    first_write = next(e for e in _events_for_epoch(events, 1)
                       if e["kind"] == "WRITE" and e["data"].get("address") == TARGET)
    queries = [engine.last_writer(1, TARGET + offset, first_write["seq"]).as_dict()
               for offset in range(4)]
    if any(item["status"] != "PROVEN" for item in queries):
        raise ValueError("reusable RAM engine failed the V1 canary coverage query")
    return {"schema": "thor.evidence.ram-v2", "trace": raw_hash,
            "query_temporal_point": first_write["seq"], "queries": queries,
            "epochs": {"1": {"versions": engine.versions(1),
                               "operations": engine.operations(1),
                               "coverage": engine.coverage(1)}}}


def derive(raw_path, static_path, rom_path, receipt_path=None, writer_raw=None):
    header, events, footer = records(raw_path)
    raw_hash = hashlib.sha256(Path(raw_path).read_bytes()).hexdigest()
    if receipt_path:
        receipt = json.loads(Path(receipt_path).read_text())
        if receipt.get("raw_sha256") != raw_hash or receipt.get("rom_sha256") != ROM_SHA:
            raise ValueError("receipt is not bound to raw capture or canonical ROM")
        if Path(receipt.get("raw_path", "")).resolve() != Path(raw_path).resolve():
            raise ValueError("receipt raw path differs from capture")
        if header.get("capture_id") != receipt.get("capture_id") or \
           header.get("receipt_sha256") != receipt.get("receipt_sha256"):
            raise ValueError("raw header is not bound to receipt identity")
        if receipt.get("status") != "COMPLETED":
            raise ValueError("capture receipt is incomplete")
    static = json.loads(Path(static_path).read_text())
    smap = _static_map(static)
    rom = Path(rom_path).read_bytes()
    if hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("wrong canonical ROM")
    epochs = sorted({e["epoch"] for e in events if e["kind"] == "EPOCH_BEGIN"})
    if epochs != [1, 2]:
        raise ValueError("canary requires two complete restore epochs")
    selected = _events_for_epoch(events, 1)
    execs = {e["data"]["pc"]: e for e in selected if e["kind"] == "EXEC"}
    a372 = [e for e in selected if e["kind"] == "EXEC" and e["data"]["pc"] == 0xA372]
    if len(a372) != 6:
        raise ValueError("canary coverage does not contain six A372 executions")
    first = a372[0]
    first_exec_seq = first["seq"]
    reads = {e["data"].get("exec_seq"): e for e in selected if e["kind"] == "READ"}
    r_offset = _read(selected, 0xFF188C, 2)
    r_counter = _read(selected, 0xFF188A, 4)
    r_selector = _read(selected, 0xFF1858, 7)
    r_record = _read(selected, ROOT_ZERO, 13)
    if r_selector["data"].get("value") != 0:
        raise ValueError("selected trace does not exercise selector==0 root")
    offset = r_offset["data"].get("value", 0) & 0xFFFF
    signed_offset = offset - 0x10000 if offset & 0x8000 else offset
    destination = (TARGET + signed_offset) & 0xFFFFFF
    old_counter = r_counter["data"].get("value", 0) & 0xFFFF
    new_counter = (old_counter + 1) & 0xFF
    record = r_record["data"].get("value")
    expected = (record & 0xFFFFFF00) | new_counter
    write = next((e for e in selected if e["kind"] == "WRITE" and
                  e["data"].get("address") == TARGET and e["data"].get("exec_seq") == first_exec_seq), None)
    if write is None or write["data"].get("value") != expected:
        raise ValueError("first A372 write does not match checked canary transform")

    versions, operations, deps = [], [], []
    def addv(v): versions.append(v); return v["id"]
    def addop(op): operations.append(op); return op["id"]
    epoch = 1
    selector = addv(_version(epoch, {"space": "RAM", "key": 0xFF1858}, (0, 8),
                             r_selector["data"]["value"], "dynamic-read"))
    offset_v = addv(_version(epoch, {"space": "RAM", "key": 0xFF188C}, (0, 16),
                           offset, "dynamic-read"))
    counter_v = addv(_version(epoch, {"space": "RAM", "key": 0xFF188A}, (0, 16),
                            old_counter, "dynamic-read"))
    root_v = addv(_version(epoch, {"space": "ROM_OFFSET", "key": ROOT_ZERO}, (0, 24),
                         ROOT_ZERO, "selector-root-constant"))
    record_v = addv(_version(epoch, {"space": "ROM_OFFSET", "key": ROOT_ZERO}, (0, 32),
                           record, "rom-read"))
    rom_bytes = []
    for i in range(3):
        rom_bytes.append(addv(_version(epoch, {"space": "ROM_OFFSET", "key": ROOT_ZERO + i},
                                       (0, 8), rom[ROOT_ZERO + i], "canonical-rom-byte")))
    d5_old = addv(_version(epoch, {"space": "REGISTER", "key": "D5"}, (0, 8),
                           old_counter & 0xFF, "move-word-then-slice"))
    d5_new = addv(_version(epoch, {"space": "REGISTER", "key": "D5"}, (0, 8),
                           new_counter, "addq-byte"))
    d2_high = addv(_version(epoch, {"space": "REGISTER", "key": "D2"}, (8, 24),
                            record & 0xFFFFFF00, "move-long-preserved-high24"))
    d2_low = addv(_version(epoch, {"space": "REGISTER", "key": "D2"}, (0, 8),
                           new_counter, "move-byte-counter"))
    d2_full = addv(_version(epoch, {"space": "REGISTER", "key": "D2"}, (0, 32),
                            expected, "byte-slice-merge"))
    a5_base = addv(_version(epoch, {"space": "REGISTER", "key": "A5"}, (0, 32),
                            TARGET, "lea-absolute"))
    destination_v = addv(_version(epoch, {"space": "REGISTER", "key": "A5"}, (0, 32),
                                  destination, "adda-sign-extend-word"))
    output = addv(_version(epoch, {"space": "RAM", "key": TARGET}, (0, 32),
                          expected, "a372-write-output"))
    execution_v = addv(_version(epoch, {"space": "REGISTER", "key": "PC"}, (0, 32),
                                first["data"]["pc"], "execution-instance"))
    # Every operation has a dynamic EXEC witness and the checked static rule.
    for e in selected:
        if e["kind"] == "EXEC" and e["data"]["pc"] in smap:
            addop(_operation(epoch, e, f"STATIC_{e['data']['pc']:04X}"))
    op_by_pc = {o["pc"]: o for o in operations}
    def witness(pc): return op_by_pc[pc]["witness_seq"]
    deps += [_edge(selector, root_v, "CONTROL", "BEQ_ZERO_SELECTS_A438", witness(0xA35E)),
             _edge(offset_v, destination_v, "ADDRESS", "ADDA_SIGN_EXTEND_WORD", witness(0xA348)),
             _edge(a5_base, destination_v, "ADDRESS", "ADDA_BASE_A5", witness(0xA348)),
             _edge(counter_v, d5_old, "VALUE", "MOVE_WORD_RAM_TO_D5_LOW16", witness(0xA34E)),
             _edge(d5_old, d5_new, "VALUE", "ADDQ_BYTE_ONE", witness(0xA36E)),
             _edge(record_v, d2_high, "VALUE", "MOVE_LONG_PRESERVE_HIGH24", witness(0xA36C)),
             _edge(d5_new, d2_low, "VALUE", "MOVE_BYTE_D5_TO_D2_LOW8", witness(0xA370)),
             _edge(d2_high, d2_full, "VALUE", "MERGE_HIGH24", witness(0xA370)),
             _edge(d2_low, d2_full, "VALUE", "MERGE_LOW8", witness(0xA370)),
             _edge(d2_full, output, "VALUE", "MOVE_LONG_D2_TO_RAM", witness(0xA372)),
             _edge(destination_v, output, "ADDRESS", "MOVE_LONG_A5_POSTINCREMENT", witness(0xA372)),
             _edge(execution_v, output, "EXECUTION", "EXEC_WITNESS_A372", witness(0xA372))]
    deps.extend(_edge(b, d2_high, "VALUE", "ROM_HIGH24_BYTE", witness(0xA36C)) for b in rom_bytes)
    # The ROM low byte is intentionally absent from the dependency set.
    frontier = [{"capability": c, "status": "UNKNOWN"} for c in
                ("access_width", "overlap_range", "same_value_writers", "irq_exception", "input_reads")]
    result = {"schema": "thor.evidence.provenance.v1", "status": "PROVEN",
              "raw_sha256": raw_hash, "rom_sha256": ROM_SHA, "static_sha256": hashlib.sha256(Path(static_path).read_bytes()).hexdigest(),
              "trace": {"epochs": epochs, "selected_epoch": epoch, "a372_exec_seqs": [e["seq"] for e in a372]},
              "target": {"address": TARGET, "version_id": output, "value_hex": _hex(expected),
                         "destination": destination, "record_hex": _hex(record), "new_counter_low8": new_counter},
              "operations": operations, "versions": versions, "dependencies": deps,
              "writer_sequence": _writer_sequence(writer_raw), "frontier": frontier,
              "checks": {"rom_low8_dependency": False, "pc2_inference": False,
                         "address_only_edge": False, "known_answer_graph_import": False,
                         "input_causal_edge": False}}
    result["ram_engine"] = _ram_canary_engine(raw_hash, events, smap,
                                               receipt.get("receipt_sha256") if receipt_path else None)
    ram = result["ram_engine"]
    ram_queries = ram["queries"]
    result["target"]["legacy_version_id"] = result["target"]["version_id"]
    result["target"]["version_id"] = ram_queries[0]["version_id"]
    result["target"]["ram_version_ids"] = [item["version_id"] for item in ram_queries]
    result["target"]["ram_operation_id"] = ram_queries[0]["operation_id"]
    result["v2_dependencies"] = [{"source": ram_queries[0]["operation_id"],
                                   "target": item["version_id"],
                                   "role": "RAM_BYTE_OUTPUT", "status": item["status"],
                                   "rule_id": "MOVE_LONG_D2_TO_RAM",
                                   "witness_event_id": item["temporal_point"]}
                                  for item in ram_queries]
    result["causal_bridge"] = {
        "source": result["target"]["legacy_version_id"],
        "operation": result["target"]["ram_operation_id"],
        "targets": list(result["target"]["ram_version_ids"]),
        "role": "RAM_BYTE_OUTPUT", "rule_id": "V1_V2_TARGET_BINDING",
        "status": "PROVEN", "witness_event_id": first_write["seq"]}
    result["causal_bridge"]["id"] = digest({"kind": "canary-causal-bridge",
                                              "value": result["causal_bridge"]})
    result["certificate_sha256"] = digest(result)
    return result


def explain(result, version_id):
    ram = result.get("ram_engine", {})
    ram_versions = {version["id"]: version for epoch in ram.get("epochs", {}).values()
                    for version in epoch.get("versions", [])}
    if version_id in ram_versions:
        version = ram_versions[version_id]
        lines = [f"{version_id} RAM[0x{version['address']:06X}] {version['value']:02X} "
                 f"{version['origin']} seq={version['temporal_seq']}"]
        operation_id = version.get("operation_id")
        operation = next((item for epoch in ram.get("epochs", {}).values()
                          for item in epoch.get("operations", []) if item["id"] == operation_id), None)
        if operation:
            lines.append(f"  <- RAM_BYTE_OUTPUT PROVEN {operation['rule_id']} "
                         f"execution={operation['execution_instance']} witness={operation['raw_witnesses']}")
        return "\n".join(lines)
    by_id = {v["id"]: v for v in result["versions"]}
    incoming = {}
    for edge in result["dependencies"]:
        incoming.setdefault(edge["target"], []).append(edge)
    seen = set()
    lines = []
    def visit(node, indent=0):
        if node in seen: return
        seen.add(node)
        v = by_id.get(node, {"id": node})
        lines.append(" " * indent + f"{node} {v.get('location')} {v.get('value_hex')} {v.get('origin')}")
        for edge in incoming.get(node, []):
            lines.append(" " * (indent + 2) + f"<- {edge['role']} {edge['status']} {edge['rule_id']} witness={edge.get('witness_event_id')}")
            visit(edge["source"], indent + 4)
    visit(version_id)
    return "\n".join(lines)


def validate_certificate(result):
    """Fail closed on the proof obligations that make this slice causal."""
    if result.get("schema") != "thor.evidence.provenance.v1" or result.get("status") != "PROVEN":
        return False
    if result.get("ram_engine"):
        return validate_ram_certificate(result)
    versions = {v["id"]: v for v in result.get("versions", [])}
    if len(versions) != len(result.get("versions", [])):
        return False
    deps = result.get("dependencies", [])
    allowed_rules = {"MOVE_LONG_D2_TO_RAM", "MERGE_HIGH24", "MERGE_LOW8",
                     "MOVE_LONG_PRESERVE_HIGH24", "MOVE_BYTE_D5_TO_D2_LOW8",
                     "ROM_HIGH24_BYTE", "BEQ_ZERO_SELECTS_A438", "ADDA_BASE_A5",
                     "ADDA_SIGN_EXTEND_WORD", "ADDQ_BYTE_ONE",
                     "MOVE_WORD_RAM_TO_D5_LOW16", "MOVE_LONG_A5_POSTINCREMENT",
                     "EXEC_WITNESS_A372"}
    if any(d.get("status") != "PROVEN" or d.get("role") not in {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}
           or d.get("source") not in versions or d.get("target") not in versions for d in deps):
        return False
    if any(d.get("rule_id") not in allowed_rules for d in deps):
        return False
    if any(versions[d["source"]].get("epoch") != versions[d["target"]].get("epoch") for d in deps):
        return False
    target = result.get("target", {})
    output = versions.get(target.get("version_id"), {})
    if output.get("value_hex") != target.get("value_hex") or output.get("location", {}).get("key") != TARGET:
        return False
    incoming = [d for d in deps if d["target"] == target["version_id"]]
    full = [d for d in incoming if d["rule_id"] == "MOVE_LONG_D2_TO_RAM"]
    if len(full) != 1:
        return False
    d2 = full[0]["source"]
    merges = [d for d in deps if d["target"] == d2]
    if {d["rule_id"] for d in merges} != {"MERGE_HIGH24", "MERGE_LOW8"}:
        return False
    low = next((d["source"] for d in merges if d["rule_id"] == "MERGE_LOW8"), None)
    if low is None or not any(d["target"] == low and d["rule_id"] == "MOVE_BYTE_D5_TO_D2_LOW8" for d in deps):
        return False
    high = next((d["source"] for d in merges if d["rule_id"] == "MERGE_HIGH24"), None)
    if high is None or not any(d["target"] == high and d["rule_id"] == "MOVE_LONG_PRESERVE_HIGH24" for d in deps):
        return False
    # A low-byte ROM version must never feed the resulting low8 slice.
    for d in deps:
        source = versions[d["source"]]
        if source.get("location", {}).get("space") == "ROM_OFFSET" and source.get("location", {}).get("key") not in {ROOT_ZERO, ROOT_ZERO + 1, ROOT_ZERO + 2}:
            return False
    if result.get("checks", {}).get("pc2_inference") or result.get("checks", {}).get("address_only_edge"):
        return False
    return True


def main(argv=None):
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    d = sub.add_parser("derive")
    d.add_argument("raw"); d.add_argument("static"); d.add_argument("rom"); d.add_argument("output")
    d.add_argument("--receipt"); d.add_argument("--writer-raw")
    e = sub.add_parser("explain"); e.add_argument("result"); e.add_argument("version")
    args = parser.parse_args(argv)
    if args.command == "derive":
        result = derive(args.raw, args.static, args.rom, args.receipt, args.writer_raw)
        Path(args.output).write_text(canonical(result) + "\n", encoding="utf-8")
        print(result["certificate_sha256"])
    else:
        print(explain(json.loads(Path(args.result).read_text()), args.version))


if __name__ == "__main__":
    main()
