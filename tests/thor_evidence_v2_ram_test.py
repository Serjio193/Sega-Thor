"""Fail-closed V2 RAM byte-version and last-writer tests."""
import json
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "tests"))

from thor_evidence.events import write_capture
from thor_evidence.ram_versions import CoverageCertificate, RamVersionEngine
from thor_evidence.store import Store
from thor_evidence_v0_test import header, events


def trace_id():
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "capture.jsonl"
        write_capture(path, header(), events())
        store = Store(Path(temp) / "evidence.sqlite")
        value = store.import_capture(path)
        store.close()
        return value


def coverage(trace, end=100, addresses=range(0x100, 0x110)):
    return CoverageCertificate("coverage-" + str(end), 0, end, tuple(addresses), trace)


def populated():
    trace = "a" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    engine.add_coverage(coverage(trace))
    return engine, trace


def test_overlap_same_value_and_big_endian_versions():
    engine, _ = populated()
    long_op = engine.write(1, 10, "long-1", 0x100, "MOVE.L", 4, 0x100, 0x11223344)
    byte_op = engine.write(1, 20, "byte-1", 0x101, "MOVE.B", 1, 0x101, 0x99)
    word_op = engine.write(1, 30, "word-1", 0x102, "MOVE.W", 2, 0x102, 0xAABB)
    same_op = engine.write(1, 40, "same-value", 0x103, "MOVE.B", 1, 0x103, 0xBB)
    assert [item["value"] for item in engine.versions(1) if item["temporal_seq"] == 10] == [0x11, 0x22, 0x33, 0x44]
    assert byte_op["previous_versions"][0] == long_op["resulting_versions"][1]
    assert word_op["previous_versions"] == long_op["resulting_versions"][2:4]
    assert same_op["resulting_versions"][0] != word_op["resulting_versions"][1]
    assert engine.last_writer(1, 0x100, 40).operation_id == long_op["id"]
    assert engine.last_writer(1, 0x101, 40).operation_id == byte_op["id"]
    assert engine.last_writer(1, 0x102, 40).operation_id == word_op["id"]
    assert engine.last_writer(1, 0x103, 40).operation_id == same_op["id"]


def test_epoch_isolation_and_initial_frontier():
    trace = "b" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    engine.begin_epoch(2, 100)
    engine.add_coverage(coverage(trace, 50), epoch=1)
    engine.add_coverage(CoverageCertificate("c2", 100, 150, (0x200,), trace), epoch=2)
    first = engine.write(1, 10, "e1", 0x200, "MOVE.B", 1, 0x200, 7)
    second = engine.write(2, 110, "e2", 0x200, "MOVE.B", 1, 0x200, 7)
    assert first["resulting_versions"][0] != second["resulting_versions"][0]
    assert engine.last_writer(2, 0x200, 110).operation_id == second["id"]
    assert engine.last_writer(1, 0x200, 0).status == "EXTERNAL_STATE"
    assert engine.last_writer(1, 0x200, 60).status == "INCOMPLETE_CAPTURE"


def test_coverage_gap_unknown_transform_conflict_and_reordered_events():
    trace = "c" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    engine.add_coverage(CoverageCertificate("gap", 0, 5, (0x300,), trace))
    engine.write(1, 10, "gap-write", 0x300, "MOVE.B", 1, 0x300, 1)
    assert engine.last_writer(1, 0x300, 10).status == "INCOMPLETE_CAPTURE"
    unknown = RamVersionEngine("d" * 64)
    unknown.begin_epoch(1, 0)
    unknown.add_coverage(CoverageCertificate("unknown", 0, 10, (0x301,), "d" * 64))
    unknown.write(1, 5, "unknown", 0x301, "UNKNOWN_TRANSFORM", 1, 0x301, 1)
    assert unknown.last_writer(1, 0x301, 5).status == "UNKNOWN_TRANSFORM"
    conflict = RamVersionEngine("e" * 64)
    conflict.begin_epoch(1, 0)
    conflict.add_coverage(CoverageCertificate("conflict", 0, 10, (0x302,), "e" * 64))
    conflict.write(1, 5, "c1", 0x302, "MOVE.B", 1, 0x302, 1)
    conflict.write(1, 5, "c2", 0x302, "MOVE.B", 1, 0x302, 2)
    assert conflict.last_writer(1, 0x302, 5).status == "CONFLICT"
    try:
        conflict.write(1, 4, "reordered", 0x303, "MOVE.B", 1, 0x303, 1)
    except ValueError:
        pass
    else:
        raise AssertionError("reordered writes must fail closed")


def test_negative_fixture_names_and_forged_coverage():
    fixture = json.loads((ROOT / "tests/fixtures/thor_evidence_v2_ram/negative_cases.json").read_text())
    assert len(fixture["cases"]) >= 12
    engine = RamVersionEngine("f" * 64)
    engine.begin_epoch(1, 0)
    try:
        engine.add_coverage(CoverageCertificate("forged", 0, 10, (0x400,), "0" * 64))
    except ValueError:
        pass
    else:
        raise AssertionError("coverage from another evidence identity must fail")


def test_sqlite_idempotence_and_deterministic_export():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        capture = root / "capture.jsonl"
        write_capture(capture, header(), events())
        store = Store(root / "evidence.sqlite")
        trace = store.import_capture(capture)
        engine = RamVersionEngine(trace)
        engine.begin_epoch(1, 0)
        engine.add_coverage(CoverageCertificate("sqlite", 0, 10, (0xFF13CC,), trace))
        engine.write(1, 1, "write-1", 0xA372, "MOVE.L", 4, 0xFF13CC, 0x00880901)
        result = engine.export()
        store.import_ram_engine(result, trace)
        before = store.export()
        store.import_ram_engine(result, trace)
        assert store.export() == before
        store.close()


def test_bounded_lookup_is_not_obviously_quadratic():
    trace = "1" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    engine.add_coverage(CoverageCertificate("perf", 0, 2000, (0x500,), trace))
    for seq in range(1, 2001):
        engine.write(1, seq, f"exec-{seq}", 0x500, "MOVE.B", 1, 0x500, seq & 0xFF)
    started = time.perf_counter()
    for seq in range(1, 2001):
        assert engine.last_writer(1, 0x500, seq).status == "PROVEN"
    assert time.perf_counter() - started < 2.0


def main():
    test_overlap_same_value_and_big_endian_versions()
    test_epoch_isolation_and_initial_frontier()
    test_coverage_gap_unknown_transform_conflict_and_reordered_events()
    test_negative_fixture_names_and_forged_coverage()
    test_sqlite_idempotence_and_deterministic_export()
    test_bounded_lookup_is_not_obviously_quadratic()
    print("PASS thor evidence v2 ram")


if __name__ == "__main__":
    main()
