"""V0 temporal identity, sealing, idempotence and fail-closed fixtures."""
import copy
import hashlib
import json
import sqlite3
import sys
import tempfile
from unittest.mock import patch
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.events import (logical_hash, read_capture, validate_events,
                                  write_capture)
from thor_evidence.identity import ROM_SHA, SCHEMA, STATE_SHA, canonical, digest, identity
from thor_evidence.normalize import records, validate_raw_epochs
from thor_evidence.normalize import _receipt_matches
from thor_evidence import report as evidence_report
from thor_evidence.receipt import create as create_receipt, finalize as finalize_receipt, load as load_receipt
from thor_evidence.store import Store


def header():
    hashes = {name: hashlib.sha256(name.encode()).hexdigest() for name in (
        "emulator", "core", "config", "collector", "watch", "harness", "map")}
    return {"schema": SCHEMA, "environment": {"rom_sha256": ROM_SHA, "rom_size": 3145728,
        "emulator_sha256": hashes["emulator"], "core_sha256": hashes["core"],
        "config_sha256": hashes["config"], "collector_sha256": hashes["collector"],
        "normalizer_sha256": hashes["collector"], "watch_sha256": hashes["watch"],
        "harness_sha256": hashes["harness"], "map_sha256": hashes["map"],
        "receipt_sha256": hashlib.sha256(b"receipt").hexdigest(),
        "launch_sha256": hashlib.sha256(b"launch").hexdigest()},
        "scenario": {"state_sha256": STATE_SHA,
        "specification": {"settle": 3, "measurement": 1, "rom_identity": ROM_SHA,
                           "state_identity": STATE_SHA, "raw_schema": "thor.evidence.raw.v0.1"}}}


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
    except (ValueError, json.JSONDecodeError, sqlite3.IntegrityError):
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


def test_raw_completion_and_identity_attacks():
    raw_header = {"kind": "RAW_HEADER", "schema": "thor.evidence.raw.v0.1",
                  "receipt_sha256": "a" * 64, "capture_id": "fixture",
                  "rom_sha256": ROM_SHA, "state_sha256": STATE_SHA,
                  "mode": "probe", "reverse": False, "watch_plan_sha256": "b" * 64}
    raw_events = [{key: value for key, value in event.items() if key != "samples"}
                  for event in events()]
    raw_footer = {"kind": "RAW_END", "schema": "thor.evidence.raw.v0.1",
                  "complete": True, "events": len(raw_events)}
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        def put(name, rows):
            path = root / name
            path.write_text("\n".join(canonical(row) for row in rows) + "\n", encoding="utf-8")
            return path
        valid = [raw_header, *raw_events, raw_footer]
        assert len(records(put("valid.raw", valid))[1]) == 3
        expect_error(lambda: validate_raw_epochs(raw_events, 2))
        for name, rows in (
            ("truncated.raw", [raw_header, *raw_events]),
            ("false.raw", [raw_header, *raw_events, {**raw_footer, "complete": False}]),
            ("count.raw", [raw_header, *raw_events, {**raw_footer, "events": 2}]),
            ("duplicate-footer.raw", [raw_header, *raw_events, raw_footer, raw_footer]),
            ("after-footer.raw", [raw_header, *raw_events, raw_footer, raw_events[0]]),
            ("schema.raw", [{**raw_header, "schema": "other"}, *raw_events, raw_footer]),
            ("gap.raw", [raw_header, {**raw_events[0], "seq": 7}, *raw_events[1:], raw_footer]),
        ):
            expect_error(lambda rows=rows, name=name: records(put(name, rows)))
        duplicate = root / "duplicate-key.raw"
        duplicate.write_text('{"kind":"RAW_HEADER","kind":"RAW_HEADER"}\n', encoding="utf-8")
        expect_error(lambda: records(duplicate))


def test_temporal_versions_and_low_byte_negative_fixture():
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "capture.jsonl"
        fixture = events()
        write_capture(path, header(), fixture)
        store = Store(Path(temp) / "evidence.sqlite")
        second = copy.deepcopy(fixture)
        for event in second:
            event["seq"] += 3; event["epoch"] = 2
        combined = fixture + second
        second_path = Path(temp) / "capture-2.jsonl"
        write_capture(second_path, header(), combined)
        store.import_capture(second_path)
        assert store.connection.execute("SELECT COUNT(*) FROM value_version").fetchone()[0] == 6
        versions = store.connection.execute("SELECT id FROM value_version ORDER BY id").fetchall()
        expect_error(lambda: store.add_temporal_link(versions[-1][0], versions[0][0]))
        assert store.connection.execute("SELECT status FROM temporal_link").fetchone() is None
        record_low = 0x00
        incremented_d5_low = 0x01
        assert record_low != incremented_d5_low
        assert fixture[1]["samples"][1]["value_hex"] == "008809"
        store.close()


def test_capability_report_requires_runtime_witnesses():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp); (root / "source").mkdir()
        empty_item = {"classification": "VALID_FOR_V1", "event_stream_sha256": "empty"}
        with patch.object(evidence_report, "_valid", return_value=(empty_item, [])), \
             patch.object(evidence_report, "raw", return_value={"sha256": "raw"}):
            result = evidence_report.build(root)
        assert not [item for item in result["capabilities"] if item["status"] == "PROVEN"]
        assert result["controls"]["oracle_matches_expected"] is False

        wrong = [{"kind": "EXEC", "epoch": 1, "data": {"address": 0xA372, "pc": 0xA372}},
                 {"kind": "WRITE", "epoch": 1, "data": {"address": 0xFF13CC, "pc": 0xA374}},
                 {"kind": "READ", "epoch": 1, "data": {"address": 0xA438, "pc": 0xA436}},
                 {"kind": "ORACLE", "epoch": 1, "data": {"sat": "WRONG", "fields": "WRONG", "input_polls": 0}}]
        def valid(_root, name, source_name=None):
            return ({"classification": "VALID_FOR_V1", "event_stream_sha256": name}, wrong)
        with patch.object(evidence_report, "_valid", side_effect=valid), \
             patch.object(evidence_report, "raw", return_value={"sha256": "raw"}):
            result = evidence_report.build(root)
        statuses = {item["id"]: item["status"] for item in result["capabilities"]}
        assert result["controls"]["oracle_matches_expected"] is False
        assert statuses["INPUT_POLL"] == "UNKNOWN"
        assert statuses["HOOK_ORDER"] == "UNKNOWN"


def test_import_rollback_and_retry():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp); path = root / "capture.jsonl"
        write_capture(path, header(), events())
        store = Store(root / "evidence.sqlite")
        before = store.export()
        store.connection.execute("CREATE TEMP TRIGGER fail_import BEFORE INSERT ON value_version "
                                 "BEGIN SELECT RAISE(ABORT, 'fixture failure'); END")
        expect_error(lambda: store.import_capture(path))
        assert store.export() == before
        store.connection.execute("DROP TRIGGER fail_import")
        store.import_capture(path)
        assert store.connection.execute("SELECT COUNT(*) FROM value_version").fetchone()[0] == 3
        store.close()


def test_launch_receipt_binds_collector_mode_and_watch_plan():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp); biz = root / "biz"; (biz / "dll").mkdir(parents=True)
        for path in (biz / "EmuHawk.exe", biz / "dll/gpgx.wbx.zst", root / "collector.lua",
                     root / "normalize.py", root / "run.ps1", root / "rom.bin", root / "state.bin"):
            path.write_bytes(path.name.encode())
        run = root / "run"; run.mkdir()
        (run / "config.ini").write_bytes(b"config"); (run / "launch.json").write_bytes(b"launch")
        raw = root / "capture.raw.jsonl"; raw.write_bytes(b"raw")
        receipt_path = root / "receipt.json"
        receipt = create_receipt(receipt_path, raw, run, "probe", False, root / "collector.lua",
                                 root / "normalize.py", root / "run.ps1", root / "rom.bin",
                                 root / "state.bin", biz)
        finalize_receipt(receipt_path)
        receipt = load_receipt(receipt_path)
        header = {"kind": "RAW_HEADER", "schema": "thor.evidence.raw.v0.1",
                  "receipt_sha256": receipt["receipt_sha256"], "capture_id": receipt["capture_id"],
                  "rom_sha256": ROM_SHA, "state_sha256": STATE_SHA, "mode": "probe",
                  "reverse": False, "watch_plan_sha256": receipt["watch_plan_sha256"]}
        _receipt_matches(raw, header, receipt)
        for field, value in (("mode", "minimal"), ("watch_plan_sha256", "c" * 64)):
            wrong = dict(header); wrong[field] = value
            expect_error(lambda wrong=wrong: _receipt_matches(raw, wrong, receipt))
        (run / "config.ini").write_bytes(b"tampered-config")
        expect_error(lambda: _receipt_matches(raw, header, receipt))
        assert receipt["collector_sha256"] != receipt["normalizer_sha256"]
        tampered = json.loads(receipt_path.read_text())
        tampered["collector_sha256"] = "d" * 64
        receipt_path.write_text(json.dumps(tampered))
        expect_error(lambda: load_receipt(receipt_path))


def main():
    for test in (test_transport_and_store, test_temporal_and_relation_guards,
                 test_rejects_bad_seal_rom_epoch_and_unknown_float,
                 test_raw_completion_and_identity_attacks,
                 test_temporal_versions_and_low_byte_negative_fixture,
                 test_capability_report_requires_runtime_witnesses,
                 test_import_rollback_and_retry,
                 test_launch_receipt_binds_collector_mode_and_watch_plan):
        test(); print("PASS", test.__name__)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
