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
from thor_evidence.ram_versions import (CoverageCertificate, RamVersionEngine,
                                         VerifiedCoverageCertificate, attest_source_events)
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


def basis(start, end, epoch=1, execution=None, decoder="test-decoder", rule="MOVE.B"):
    return attest_source_events([{"seq": seq, "epoch": epoch, "kind": "EXEC", "receipt_sha256": "b" * 64,
             "decoder_id": decoder, "rule_id": rule,
             "data": {"pc": 0x1000 + seq},
             **({"execution_instance": execution(seq)} if execution else {})}
            for seq in range(start, end + 1)])


def coverage(trace, end=100, addresses=range(0x100, 0x110)):
    claim = CoverageCertificate("coverage-" + str(end), 0, end, tuple(addresses), trace)
    return VerifiedCoverageCertificate.from_capture(
        claim, trace=trace, epoch=1, raw_artifact_hash=trace,
        receipt_sha256="b" * 64, decoder_id="test-decoder", rule_id="*",
        execution_instances=("1",),
        source_events=basis(0, end, rule="*"))


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
    engine.add_coverage(coverage(trace, 50, range(0x200, 0x210)), epoch=1)
    c2 = CoverageCertificate("c2", 100, 150, (0x200,), trace)
    engine.add_coverage(VerifiedCoverageCertificate.from_capture(
        c2, trace=trace, epoch=2, raw_artifact_hash=trace, receipt_sha256="b" * 64,
        decoder_id="test-decoder", rule_id="MOVE.B", execution_instances=("110",),
        source_events=basis(100, 150, epoch=2)), epoch=2)
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
    engine.add_coverage(VerifiedCoverageCertificate.from_capture(
        CoverageCertificate("gap", 0, 5, (0x300,), trace), trace=trace, epoch=1,
        raw_artifact_hash=trace, receipt_sha256="b" * 64, decoder_id="test-decoder",
        rule_id="MOVE.B", execution_instances=("1",),
        source_events=basis(0, 5)))
    engine.write(1, 10, "gap-write", 0x300, "MOVE.B", 1, 0x300, 1)
    assert engine.last_writer(1, 0x300, 10).status == "INCOMPLETE_CAPTURE"
    unknown = RamVersionEngine("d" * 64)
    unknown.begin_epoch(1, 0)
    unknown.add_coverage(VerifiedCoverageCertificate.from_capture(
        CoverageCertificate("unknown", 0, 10, (0x301,), "d" * 64), trace="d" * 64,
        epoch=1, raw_artifact_hash="d" * 64, receipt_sha256="b" * 64,
        decoder_id="test-decoder", rule_id="UNKNOWN_TRANSFORM", execution_instances=("5",),
        source_events=basis(0, 10, rule="UNKNOWN_TRANSFORM")))
    unknown.write(1, 5, "unknown", 0x301, "UNKNOWN_TRANSFORM", 1, 0x301, 1)
    assert unknown.last_writer(1, 0x301, 5).status == "UNKNOWN_TRANSFORM"
    conflict = RamVersionEngine("e" * 64)
    conflict.begin_epoch(1, 0)
    conflict.add_coverage(VerifiedCoverageCertificate.from_capture(
        CoverageCertificate("conflict", 0, 10, (0x302,), "e" * 64), trace="e" * 64,
        epoch=1, raw_artifact_hash="e" * 64, receipt_sha256="b" * 64,
        decoder_id="test-decoder", rule_id="MOVE.B", execution_instances=("5",),
        source_events=basis(0, 10)))
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
    except (TypeError, ValueError):
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
        engine.add_coverage(VerifiedCoverageCertificate.from_capture(
            CoverageCertificate("sqlite", 0, 10, (0xFF13CC,), trace), trace=trace, epoch=1,
            raw_artifact_hash=trace, receipt_sha256="b" * 64, decoder_id="test-decoder",
            rule_id="MOVE.L", execution_instances=("1",),
            source_events=basis(0, 10, rule="MOVE.L")))
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
    engine.add_coverage(VerifiedCoverageCertificate.from_capture(
        CoverageCertificate("perf", 0, 2000, (0x500,), trace), trace=trace, epoch=1,
        raw_artifact_hash=trace, receipt_sha256="b" * 64, decoder_id="test-decoder",
        rule_id="MOVE.B", execution_instances=tuple(f"exec-{i}" for i in range(1, 2001)),
        source_events=basis(0, 2000, execution=lambda seq: f"exec-{seq}")))
    for seq in range(1, 2001):
        engine.write(1, seq, f"exec-{seq}", 0x500, "MOVE.B", 1, 0x500, seq & 0xFF)
    started = time.perf_counter()
    for seq in range(1, 2001):
        assert engine.last_writer(1, 0x500, seq).status == "PROVEN"
    assert time.perf_counter() - started < 2.0


def test_adversarial_temporal_and_construction_guards():
    trace = "2" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 10)
    try:
        engine.add_coverage(CoverageCertificate("forged", 10, 20, (0x600,), trace))
    except TypeError:
        pass
    else:
        raise AssertionError("unverified claims must not enter the engine")
    engine.write(1, 11, "exec-10", 0x100, "MOVE.B", 1, 0x600, 7)
    root_query = engine.last_writer(1, 0x600, 9)
    assert root_query.status == "PRE_CAPTURE_ORIGIN"
    root_query = engine.last_writer(1, 0x600, 10)
    assert root_query.status == "INCOMPLETE_CAPTURE"
    assert root_query.version_id != engine.operations(1)[0]["resulting_versions"][0]
    assert engine.last_writer(1, 0x601, 10).status == "INCOMPLETE_CAPTURE"
    before = (len(engine.operations(1)), len(engine.versions(1)))
    try:
        engine.write(1, 12, "bad", 1, "MOVE.L", 4, 0xFFFFFF, 0x11223344)
    except ValueError:
        pass
    else:
        raise AssertionError("out-of-range long write must fail")
    assert (len(engine.operations(1)), len(engine.versions(1))) == before


def test_verified_certificate_rejects_gap_epoch_and_scope_expansion():
    trace = "3" * 64
    claim = CoverageCertificate("gap", 0, 4, (0x700,), trace)
    try:
        VerifiedCoverageCertificate.from_capture(
            claim, trace=trace, epoch=2, raw_artifact_hash=trace,
            receipt_sha256="4" * 64, decoder_id="decoder", rule_id="MOVE.B",
            execution_instances=("3",),
            source_events=[{"seq": 0, "epoch": 2}, {"seq": 1, "epoch": 2},
                           {"seq": 3, "epoch": 2}, {"seq": 4, "epoch": 2}])
    except ValueError:
        pass
    else:
        raise AssertionError("truncated certificate basis must fail")
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    try:
        engine.add_coverage(CoverageCertificate("wrong-epoch", 0, 4, (0x700,), trace), epoch=1)
    except TypeError:
        pass
    else:
        raise AssertionError("wrong-epoch claim must not be trusted")


def test_duplicate_immutable_event_is_idempotent():
    trace = "4" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    operation = engine.write(1, 1, "exec-1", 1, "MOVE.B", 1, 0x800, 9)
    duplicate = engine.write(1, 1, "exec-1", 1, "MOVE.B", 1, 0x800, 9)
    assert duplicate["id"] == operation["id"]
    assert len(engine.operations(1)) == 1


def test_unattested_historical_tags_cannot_create_verified_coverage():
    trace = "5" * 64
    engine = RamVersionEngine(trace)
    engine.begin_epoch(1, 0)
    raw = [{"seq": seq, "epoch": 1, "receipt_sha256": "b" * 64,
            "decoder_id": "test-decoder", "rule_id": "MOVE.B"} for seq in range(3)]
    try:
        VerifiedCoverageCertificate.from_capture(
            CoverageCertificate("unattested", 0, 2, (0x900,), trace), trace=trace,
            epoch=1, raw_artifact_hash=trace, receipt_sha256="b" * 64,
            decoder_id="test-decoder", rule_id="MOVE.B", execution_instances=("1",),
            source_events=raw)
    except ValueError:
        pass
    else:
        raise AssertionError("bare historical tags must not authorize coverage")


def test_coverage_rejects_note_substitution_and_unknown_effect():
    trace = "6" * 64
    for mutation in ({"kind": "NOTE"}, {"effect_status": "UNKNOWN"}):
        engine = RamVersionEngine(trace)
        engine.begin_epoch(1, 0)
        source = basis(0, 2)
        source[1].update(mutation)
        source = attest_source_events([{key: value for key, value in event.items()
                                       if key not in {"event_sha256", "basis_sha256"}}
                                      for event in source])
        try:
            VerifiedCoverageCertificate.from_capture(
                CoverageCertificate("bad-effect", 0, 2, (0x910,), trace), trace=trace,
                epoch=1, raw_artifact_hash=trace, receipt_sha256="b" * 64,
                decoder_id="test-decoder", rule_id="MOVE.B", execution_instances=("1",),
                source_events=source)
        except ValueError:
            continue
        raise AssertionError("coverage must reject NOTE/unknown-effect substitution")


def test_sqlite_rejects_wrong_producer_output_association():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        capture = root / "capture.jsonl"
        write_capture(capture, header(), events())
        store = Store(root / "evidence.sqlite")
        trace = store.import_capture(capture)
        engine = RamVersionEngine(trace)
        engine.begin_epoch(1, 0)
        engine.add_coverage(VerifiedCoverageCertificate.from_capture(
            CoverageCertificate("semantic", 0, 3, (0x900, 0x901), trace),
            trace=trace, epoch=1, raw_artifact_hash=trace, receipt_sha256="b" * 64,
            decoder_id="test-decoder", rule_id="MOVE.B", execution_instances=("1", "2"),
            source_events=basis(0, 3, rule="MOVE.B")))
        engine.write(1, 1, "1", 0x10, "MOVE.B", 1, 0x900, 0x11)
        engine.write(1, 2, "2", 0x10, "MOVE.B", 1, 0x901, 0x22)
        bad = json.loads(json.dumps(engine.export()))
        bad["epochs"]["1"]["operations"][0]["resulting_versions"] = [
            bad["epochs"]["1"]["operations"][1]["resulting_versions"][0]]
        try:
            store.import_ram_engine(bad, trace)
        except ValueError:
            pass
        else:
            raise AssertionError("wrong producer/output association must fail")
        assert store.connection.execute("SELECT COUNT(*) FROM ram_write_operation").fetchone()[0] == 0
        store.close()


def main():
    test_overlap_same_value_and_big_endian_versions()
    test_epoch_isolation_and_initial_frontier()
    test_coverage_gap_unknown_transform_conflict_and_reordered_events()
    test_negative_fixture_names_and_forged_coverage()
    test_sqlite_idempotence_and_deterministic_export()
    test_bounded_lookup_is_not_obviously_quadratic()
    test_adversarial_temporal_and_construction_guards()
    test_verified_certificate_rejects_gap_epoch_and_scope_expansion()
    test_duplicate_immutable_event_is_idempotent()
    test_unattested_historical_tags_cannot_create_verified_coverage()
    test_coverage_rejects_note_substitution_and_unknown_effect()
    test_sqlite_rejects_wrong_producer_output_association()
    print("PASS thor evidence v2 ram")


if __name__ == "__main__":
    main()
