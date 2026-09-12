"""V0 temporal identity, sealing, idempotence and fail-closed fixtures."""
import copy
import hashlib
import json
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.events import (logical_hash, read_capture, validate_events,
                                  write_capture)
from thor_evidence.identity import ROM_SHA, SCHEMA, digest, identity
from thor_evidence.store import Store


def header():
    hashes = {name: hashlib.sha256(name.encode()).hexdigest() for name in (
        "emulator", "core", "config", "collector", "watch", "harness", "map")}
    return {"schema": SCHEMA, "environment": {"rom_sha256": ROM_SHA, "rom_size": 3145728,
        "emulator_sha256": hashes["emulator"], "core_sha256": hashes["core"],
        "config_sha256": hashes["config"], "collector_sha256": hashes["collector"],
        "watch_sha256": hashes["watch"], "harness_sha256": hashes["harness"],
        "map_sha256": hashes["map"]}, "scenario": {"state_sha256": hashlib.sha256(b"state").hexdigest(),
        "specification": {"settle": 3, "measurement": 1}}}


def sample(space, key, value, width=32, offset=0, status="OBSERVED"):
    return {"location": {"space": space, "key": key}, "bit_offset": offset,
            "bit_width": width, "value_hex": value, "status": status}


def events():
    state = header()["scenario"]["state_sha256"]
    return [
        {"seq": 0, "epoch": 1, "frame": 2117, "actor": "M68K", "phase": "RAW",
         "kind": "EPOCH_BEGIN", "data": {"state_sha256": state}, "samples": []},
        {"seq": 1, "epoch": 1, "frame": 2120, "actor": "M68K", "phase": "RAW",
         "kind": "WRITE", "data": {"address": 0xFF13CC}, "samples": [
             sample("M68K_BUS", 0xFF13CC, "00880901"),
             sample("REGISTER", "D2", "008809", 24, 8),
             sample("REGISTER", "D2", "01", 8, 0)]},
        {"seq": 2, "epoch": 1, "frame": 2121, "actor": "M68K", "phase": "RAW",
         "kind": "EPOCH_END", "data": {"reason": "COMPLETE"}, "samples": []},
    ]


def expect_error(call):
    try:
        call()
    except (ValueError, json.JSONDecodeError):
        return
    raise AssertionError("expected fail-closed rejection")


def test_transport_and_store():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        first = root / "capture.jsonl"
        write_capture(first, header(), events())
        loaded = read_capture(first)
        assert loaded[2] == logical_hash(header(), events())
        store = Store(root / "evidence.sqlite")
        trace = store.import_capture(first)
        before = store.export()
        assert trace == store.import_capture(first)
        assert before == store.export()
        rows = store.connection.execute("SELECT id FROM value_version ORDER BY id").fetchall()
        assert len(rows) == 3
        relation = store.connection.execute("SELECT location_id FROM value_version WHERE event_id IN "
            "(SELECT id FROM event WHERE kind='WRITE') ORDER BY location_id").fetchall()
        assert len(relation) == 3
        store.close()


def test_temporal_and_relation_guards():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = root / "capture.jsonl"
        write_capture(path, header(), events())
        store = Store(root / "evidence.sqlite")
        store.import_capture(path)
        versions = store.connection.execute("SELECT id FROM value_version ORDER BY id").fetchall()
        assert len(versions) == 3
        expect_error(lambda: store.add_temporal_link(versions[1][0], versions[0][0]))
        locations = store.connection.execute("SELECT DISTINCT location_id FROM value_version").fetchall()
        event = store.connection.execute("SELECT id FROM event WHERE kind='WRITE'").fetchone()[0]
        rid = store.add_relation(locations[0][0], locations[1][0], "OBSERVED_ACCESS", {"scope": "fixture"}, event)
        assert rid == store.add_relation(locations[0][0], locations[1][0], "OBSERVED_ACCESS", {"scope": "fixture"}, event)
        store.close()


def test_rejects_bad_seal_rom_epoch_and_unknown_float():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        good = root / "good.jsonl"
        write_capture(good, header(), events())
        lines = good.read_text(encoding="utf-8").splitlines()
        bad_seal = root / "bad-seal.jsonl"
        lines[-1] = lines[-1].replace('"complete":true', '"complete":false')
        bad_seal.write_text("\n".join(lines) + "\n", encoding="utf-8")
        expect_error(lambda: read_capture(bad_seal))
        wrong = header(); wrong["environment"]["rom_sha256"] = "0" * 64
        expect_error(lambda: validate_events(wrong, events()))
        rewound = events(); rewound[2]["frame"] = 1
        expect_error(lambda: validate_events(header(), rewound))
        expect_error(lambda: digest({"float": 1.5}))


def main():
    for test in (test_transport_and_store, test_temporal_and_relation_guards,
                 test_rejects_bad_seal_rom_epoch_and_unknown_float):
        test(); print("PASS", test.__name__)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
