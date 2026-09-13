"""Bounded FF13CC V1-gate certificates; never builds a provenance graph."""
import argparse
import hashlib
import json
from pathlib import Path

from .normalize import records

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
A372 = 0x00A372
A374 = 0x00A374
TARGET = 0x00FF13CC
RAW_SCHEMA = "thor.evidence.raw.v0.1"
CLASSIFICATIONS = {"NO_MEMORY_WRITE", "WRITE_DISJOINT", "WRITE_TARGET_OVERLAP",
                   "UNKNOWN_MEMORY_EFFECT"}


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _hx(value, width=6):
    return f"0x{value:0{width}X}"


def static_certificate(rom_path, tool_path=None):
    rom = Path(rom_path).read_bytes()
    if len(rom) != ROM_SIZE or file_hash(rom_path) != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    if rom[A372:A374] != bytes.fromhex("2AC2"):
        raise ValueError("A372 instruction bytes changed")
    from m12_a372_shadow_sat_producer import parse_producer
    producer = parse_producer(rom)
    if producer["routine"]["range"] != ["0x00A342", "0x00A438"]:
        raise ValueError("checked producer boundary changed")
    root = Path(tool_path or Path(__file__).parents[1] / "m12_a372_shadow_sat_producer.py")
    decoder = Path(__file__).parents[1] / "re_slice_decoder.cpp"
    return {"status": "PROVEN", "rom_sha256": ROM_SHA,
            "instruction": {"address": _hx(A372), "bytes": "2AC2",
                             "mnemonic": "MOVE.L", "form": "MOVE.L D2,(A5)+",
                             "width_bytes": 4, "effective_address": "A5 postincrement",
                             "length_bytes": 2, "next_pc": _hx(A374)},
            "routine": producer["routine"],
            "checked_static": {"producer_tool": str(root), "producer_sha256": file_hash(root),
                                "decoder_source": str(decoder), "decoder_sha256": file_hash(decoder),
                                "contract": "existing exact M68K/static producer evidence"}}


def validate_static(cert):
    instruction = cert.get("instruction", {})
    expected = {"address": _hx(A372), "bytes": "2AC2", "mnemonic": "MOVE.L",
                "form": "MOVE.L D2,(A5)+", "width_bytes": 4,
                "effective_address": "A5 postincrement", "length_bytes": 2,
                "next_pc": _hx(A374)}
    if cert.get("status") != "PROVEN" or cert.get("rom_sha256") != ROM_SHA:
        return False
    return instruction == expected


def _registers(event):
    value = event.get("data", {}).get("registers")
    if not isinstance(value, dict):
        raise ValueError("runtime event has no register snapshot")
    return value


def pairing_from_capture(raw_path):
    header, events, _ = records(raw_path)
    candidates = [event for event in events
                  if event["kind"] == "EXEC" and event["data"].get("address") == A372
                  and event["data"].get("window") == "test"
                  and event["data"].get("right") is True]
    if not candidates:
        raise ValueError("no bounded A372 test instance")
    pre = candidates[0]
    after = events[events.index(pre) + 1:]
    writes = [event for event in after if event["kind"] == "WRITE"
              and event["data"].get("address") == _registers(pre).get("A5")]
    if not writes:
        raise ValueError("A372 instance has no following destination write")
    write = writes[0]
    following = [event for event in after if event["kind"] == "EXEC"
                 and event["data"].get("address") == A374
                 and event["seq"] > write["seq"]]
    if not following:
        raise ValueError("A372 instance has no post/next boundary")
    post = following[0]
    pre_regs, post_regs = _registers(pre), _registers(write)
    destination = pre_regs.get("A5")
    if destination != TARGET or post_regs.get("A5") != TARGET + 4:
        raise ValueError("selected A372 destination is not FF13CC postincrement")
    value = write["data"].get("value")
    if type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
        raise ValueError("selected write has no four-byte value")
    return {"status": "PROVEN", "trace": file_hash(raw_path), "epoch": pre["epoch"],
            "execution_instance": f"epoch-{pre['epoch']}-seq-{pre['seq']}",
            "exec_seq": pre["seq"], "write_seq": write["seq"], "post_seq": post["seq"],
            "callback_pc": _hx(write["data"].get("pc", 0)), "exec_pc": _hx(A372),
            "post_pc": _hx(post["data"].get("pc", 0)), "destination": _hx(destination),
            "value": f"{value:08X}", "pre": {"pc": _hx(pre["data"].get("pc", 0)),
            "A5": _hx(pre_regs["A5"]), "D2": f"{pre_regs['D2'] & 0xFFFFFFFF:08X}"},
            "post": {"pc": _hx(post["data"].get("pc", 0)),
                     "A5": _hx(post_regs.get("A5", 0))},
            "raw_witnesses": {"header_schema": header["schema"],
                              "events": [pre["seq"], write["seq"], post["seq"]]}}


def validate_pairing(cert, static):
    return (validate_static(static) and cert.get("status") == "PROVEN"
            and cert.get("exec_pc") == _hx(A372)
            and cert.get("destination") == _hx(TARGET)
            and cert.get("callback_pc") == _hx(A374)
            and cert.get("post_pc") == _hx(A374)
            and cert.get("exec_seq", 0) < cert.get("write_seq", 0) < cert.get("post_seq", 0)
            and cert.get("value") and cert.get("pre", {}).get("A5") == _hx(TARGET)
            and cert.get("post", {}).get("A5") == _hx(TARGET + 4))


def coverage_certificate(coverage):
    """Return local completeness only when dense coverage is explicitly supplied."""
    instructions = coverage.get("instructions", [])
    if (not coverage.get("complete") or not coverage.get("target_range_complete")
            or not coverage.get("same_value_writes_included")
            or coverage.get("instruction_count") != len(instructions)):
        return {"status": "BLOCKED", "reason": "dense instruction coverage is absent",
                "interval": coverage.get("interval"),
                "executed_instruction_count": coverage.get("instruction_count"),
                "classifications": coverage.get("classifications", []),
                "unknown_effects": ["unobserved instructions in local interval"]}
    if any(item.get("classification") not in CLASSIFICATIONS for item in instructions):
        return {"status": "BLOCKED", "reason": "invalid memory-effect classification"}
    unknown = [item for item in instructions
               if item.get("classification") == "UNKNOWN_MEMORY_EFFECT"]
    if unknown:
        return {"status": "BLOCKED", "reason": "unknown memory effect may overlap target",
                "interval": coverage.get("interval"),
                "executed_instruction_count": len(instructions),
                "classifications": instructions, "unknown_effects": unknown}
    target_writers = [item for item in instructions
                      if item.get("classification") == "WRITE_TARGET_OVERLAP"]
    if not target_writers:
        return {"status": "BLOCKED", "reason": "no target writer is covered",
                "interval": coverage.get("interval"),
                "executed_instruction_count": len(instructions),
                "classifications": instructions, "unknown_effects": []}
    for item in target_writers:
        span = item.get("range")
        try:
            start, end = (int(value, 0) if isinstance(value, str) else int(value)
                          for value in span)
        except (TypeError, ValueError):
            start, end = 0, 0
        if (not isinstance(span, list) or len(span) != 2 or start >= end
                or end <= TARGET or start >= TARGET + 4):
            return {"status": "BLOCKED", "reason": "target writer lacks a valid range",
                    "classifications": instructions}
    return {"status": "PROVEN", "interval": coverage.get("interval"),
            "executed_instruction_count": len(instructions),
            "classifications": instructions, "unknown_effects": []}


def interruption_certificate(interruption):
    if interruption.get("status") == "NONE" and interruption.get("control_flow_continuous") is True:
        return {"status": "PROVEN", "evidence": interruption}
    if interruption.get("status") == "INCLUDED" and interruption.get("handlers_complete") is True:
        return {"status": "PROVEN", "evidence": interruption}
    return {"status": "BLOCKED", "evidence": interruption,
            "reason": "interruption boundary is not established"}


def value_versions(pairing):
    return [{"version_id": f"{pairing['trace']}:{pairing['epoch']}:{pairing['execution_instance']}:{i}",
             "trace": pairing["trace"], "epoch": pairing["epoch"],
             "execution_instance": pairing["execution_instance"],
             "decoded_operation": "MOVE.L D2,(A5)+", "byte_offset": i,
             "destination": pairing["destination"], "value": pairing["value"][i * 2:i * 2 + 2],
             "pre": pairing["pre"], "post": pairing["post"],
             "raw_witnesses": pairing["raw_witnesses"]} for i in range(4)]


def validate_versions(versions, pairing):
    return (len(versions) == 4 and len({item["version_id"] for item in versions}) == 4
            and all(item["trace"] == pairing["trace"] and item["epoch"] == pairing["epoch"]
                    and item["execution_instance"] == pairing["execution_instance"]
                    and item["destination"] == pairing["destination"] for item in versions))


def gate(rom_path, raw_path):
    static = static_certificate(rom_path)
    pairing = pairing_from_capture(raw_path)
    versions = value_versions(pairing)
    coverage = coverage_certificate({"complete": False,
        "interval": {"start_seq": pairing["exec_seq"], "end_seq": pairing["post_seq"]},
        "instruction_count": None, "classifications": [], "instructions": []})
    interruption = interruption_certificate({"status": "UNKNOWN",
                                             "control_flow_continuous": False,
                                             "source": "V0.1 exact-address capture"})
    statuses = {"static_form": validate_static(static),
                "pairing": validate_pairing(pairing, static),
                "value_version": validate_versions(versions, pairing),
                "writer_completeness": coverage["status"] == "PROVEN",
                "interruption": interruption["status"] == "PROVEN"}
    return {"schema": "thor.evidence.v1-gate.local.v0.1", "decision": "PARTIAL / V1 BLOCKED",
            "static": static, "pairing": pairing, "value_versions": versions,
            "writer_coverage": coverage, "interruption": interruption,
            "statuses": statuses, "source_owned": {"before": 1475368, "after": 1475368, "delta": 0},
            "stop_reason": "pairing closed; dense writer coverage and interruption boundary unavailable"}


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("rom"); parser.add_argument("raw"); parser.add_argument("output")
    args = parser.parse_args(argv)
    result = gate(args.rom, args.raw)
    Path(args.output).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
