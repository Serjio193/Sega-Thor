"""Sealed JSONL transport: validation is integrity, never causal proof."""
import hashlib
import json
from pathlib import Path

from .identity import (ROM_SHA, ROM_SIZE, SCHEMA, STATUSES, canonical, digest,
                       location_key, require_hash)

MAX_EVENTS = 1_000_000
MAX_BYTES = 256 * 1024 * 1024
KINDS = {"EPOCH_BEGIN", "EPOCH_END", "EXEC", "READ", "WRITE", "SNAPSHOT",
         "INPUT", "PEEK", "ORACLE", "NOTE"}


def _object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def parse_json(text):
    value = json.loads(text, object_pairs_hook=_object)
    canonical(value)
    return value


def validate_header(header):
    if set(header) != {"schema", "environment", "scenario"} or header["schema"] != SCHEMA:
        raise ValueError("unknown capture schema/header")
    env = header["environment"]
    hashes = {"rom_sha256", "emulator_sha256", "core_sha256", "config_sha256",
              "collector_sha256", "watch_sha256", "harness_sha256", "map_sha256"}
    if set(env) != hashes | {"rom_size"}:
        raise ValueError("incomplete environment identity")
    for key in hashes:
        require_hash(env[key])
    if env["rom_sha256"] != ROM_SHA or type(env["rom_size"]) is not int or env["rom_size"] != ROM_SIZE:
        raise ValueError("wrong canonical ROM")
    scenario = header["scenario"]
    if set(scenario) != {"state_sha256", "specification"} or not isinstance(scenario["specification"], dict):
        raise ValueError("incomplete scenario identity")
    require_hash(scenario["state_sha256"])
    canonical(header)


def validate_sample(sample, header):
    if set(sample) != {"location", "bit_offset", "bit_width", "value_hex", "status"}:
        raise ValueError("invalid sample fields")
    env = header["environment"]
    location_key(env["rom_sha256"], env["map_sha256"], sample["location"])
    offset, width = sample["bit_offset"], sample["bit_width"]
    if type(offset) is not int or type(width) is not int or offset < 0 or not 1 <= width <= 64:
        raise ValueError("invalid sample bit slice")
    if offset + width > 64 or sample["status"] not in STATUSES:
        raise ValueError("unverified sample cannot be promoted")
    if sample["location"]["space"] == "REGISTER":
        limit = 16 if sample["location"]["key"] == "SR" else 32
        if offset + width > limit:
            raise ValueError("register slice exceeds register")
    value = sample["value_hex"]
    if value is not None:
        if not isinstance(value, str) or len(value) != (width + 3) // 4:
            raise ValueError("value/width mismatch")
        if any(c not in "0123456789ABCDEF" for c in value) or int(value, 16) >= 1 << width:
            raise ValueError("invalid sample value")


def validate_events(header, events):
    validate_header(header)
    if not 1 <= len(events) <= MAX_EVENTS:
        raise ValueError("capture event budget")
    epoch, active, last_frame = 0, False, None
    for seq, event in enumerate(events):
        if set(event) != {"seq", "epoch", "frame", "actor", "phase", "kind", "data", "samples"}:
            raise ValueError("invalid event envelope")
        if type(event["seq"]) is not int or event["seq"] != seq:
            raise ValueError("event order/sequence gap")
        if type(event["epoch"]) is not int or type(event["frame"]) is not int or event["frame"] < 0:
            raise ValueError("invalid epoch/frame")
        if event["kind"] not in KINDS or event["phase"] != "RAW" or event["actor"] != "M68K":
            raise ValueError("unknown event capability/type")
        if not isinstance(event["data"], dict) or not isinstance(event["samples"], list):
            raise ValueError("invalid event payload")
        if event["kind"] == "EPOCH_BEGIN":
            if active or event["epoch"] != epoch + 1:
                raise ValueError("missing restore epoch boundary")
            epoch, active, last_frame = epoch + 1, True, None
            if event["data"].get("state_sha256") != header["scenario"]["state_sha256"]:
                raise ValueError("epoch state mismatch")
        if not active or event["epoch"] != epoch:
            raise ValueError("cross-epoch event")
        if last_frame is not None and event["frame"] < last_frame:
            raise ValueError("frame rewind without epoch")
        last_frame = event["frame"]
        if event["kind"] == "EPOCH_END":
            active = False
        for sample in event["samples"]:
            validate_sample(sample, header)
        canonical(event)
    if active:
        raise ValueError("incomplete execution epoch")


def logical_hash(header, events):
    result = hashlib.sha256()
    for value in (header, *events):
        result.update((canonical(value) + "\n").encode("utf-8"))
    return result.hexdigest()


def event_stream_hash(events):
    """Compare observations while keeping environment/trace identity separate."""
    result = hashlib.sha256()
    for event in events:
        result.update((canonical(event) + "\n").encode("utf-8"))
    return result.hexdigest()


def write_capture(path, header, events):
    validate_events(header, events)
    checksum = logical_hash(header, events)
    footer = {"kind": "SEAL", "event_count": len(events), "complete": True,
              "logical_sha256": checksum}
    path = Path(path)
    if path.exists():
        existing = read_capture(path)
        if existing[2] == checksum:
            return checksum
        raise ValueError("sealed artifact already exists with a different payload")
    with path.open("x", encoding="utf-8", newline="\n") as output:
        for item in (header, *events, footer):
            output.write(canonical(item) + "\n")
    return checksum


def read_capture(path):
    path = Path(path)
    if path.stat().st_size > MAX_BYTES:
        raise ValueError("capture byte budget")
    raw = path.read_bytes()
    records = [parse_json(line) for line in raw.decode("utf-8").splitlines()]
    if len(records) < 3:
        raise ValueError("unsealed capture")
    header, events, footer = records[0], records[1:-1], records[-1]
    validate_events(header, events)
    expected = {"kind": "SEAL", "event_count": len(events), "complete": True,
                "logical_sha256": logical_hash(header, events)}
    if canonical(footer) != canonical(expected):
        raise ValueError("missing/invalid seal")
    return header, events, expected["logical_sha256"], hashlib.sha256(raw).hexdigest()
