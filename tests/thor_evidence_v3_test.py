"""V3 build skeleton tests: structure, temporal identity and persistence."""
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "tests"))

from thor_evidence.execution_model import ExecutionModel
from thor_evidence.ram_versions import RamVersionEngine
from thor_evidence.register_versions import RegisterVersionEngine
from thor_evidence.store import Store
from thor_evidence_v0_test import events, header
from thor_evidence.events import write_capture
from thor_evidence.v3_graph import V3Graph


TRACE = "9" * 64


def chain(trace=TRACE):
    execution = ExecutionModel(trace)
    registers = RegisterVersionEngine(trace, execution_model=execution)
    registers.begin_epoch(1, 0, {"D2": 0x12000000, "D5": 0x000000FE})
    e1 = execution.record(1, 1, 0x100, "MOVEQ")
    d5 = registers.addq_b(1, 1, e1.id, e1.pc, "D5", 1)
    e2 = execution.record(1, 2, 0x104, "MOVE.B")
    registers.move(1, 2, e2.id, e2.pc, "D2", "D5", 8)
    e3 = execution.record(1, 3, 0x108, "LEA")
    a5 = registers.lea(1, 3, e3.id, e3.pc, "A5", 0xFF13CC, ("FF188C",))
    callee = execution.record(1, 4, 0x200, "JSR")
    execution.call(e3, callee, (3, 4))
    continuation = execution.record(1, 5, 0x10C, "RTS")
    execution.ret(callee, continuation, (4, 5))
    branch = execution.record(1, 6, 0x110, "BNE")
    execution.branch(e2, branch, "TST", True, "PROVISIONAL", (2, 6))
    return registers, execution, d5, a5


def test_partial_register_semantics_and_identity():
    registers, execution, _, _ = chain()
    assert registers.current(1, "D2") == 0x120000FF
    low_ids = registers.read(1, "D2", 0, 8)[1]
    high_ids = registers.read(1, "D2", 8, 24)[1]
    assert low_ids and high_ids and set(low_ids).isdisjoint(high_ids)
    assert execution.record(1, 7, 0x100).id != execution.record(1, 8, 0x100).id
    assert registers.movea_w(1, 7, "movea", 0x114, "A0", 0x8001).value == 0xFFFF8001
    first = registers.current(1, "D5")
    registers.addq_b(1, 8, "later", 0x118, "D5", 1)
    assert first != registers.current(1, "D5")


def test_supported_rules_and_unknown_frontier():
    registers, _, _, _ = chain()
    registers.add(1, 9, "add", 0x120, "D2", 1)
    registers.subi(1, 10, "sub", 0x124, "D2", 1)
    registers.and_(1, 11, "and", 0x128, "D2", 0xFFFF)
    registers.or_(1, 12, "or", 0x12C, "D2", 1)
    registers.eor(1, 13, "eor", 0x130, "D2", 1)
    unknown = registers.unsupported(1, 14, "unknown", 0x134, "D2")
    assert unknown.status == "UNKNOWN" and unknown.value is None
    registers.clear(1, 15, "clear", 0x138, "D2")
    assert registers.current(1, "D2") == 0


def test_ram_interop_graph_explain_and_roles():
    ram = RamVersionEngine(TRACE)
    ram.begin_epoch(1, 0)
    execution = ExecutionModel(TRACE)
    registers = RegisterVersionEngine(TRACE, ram_engine=ram, execution_model=execution)
    registers.begin_epoch(1, 0, {"D2": 0x00880901})
    instance = execution.record(1, 1, 0xA372, "MOVE.L")
    operation = registers.write_ram(1, 1, instance.id, instance.pc, "MOVE.L", 0xFF13CC, "D2")
    graph = V3Graph(TRACE, registers, execution, ram)
    result = graph.explain(operation["resulting_versions"][0])
    roles = {edge["role"] for edge in graph.export()["dependencies"]}
    assert result["target"]["address"] == 0xFF13CC
    assert "VALUE" in roles and "ADDRESS" not in roles
    assert all(edge["role"] in {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}
               for edge in graph.export()["dependencies"])


def test_persistence_reopen_and_duplicate():
    with tempfile.TemporaryDirectory() as temp:
        capture = Path(temp) / "capture.jsonl"
        write_capture(capture, header(), events())
        store = Store(Path(temp) / "evidence.sqlite")
        trace = store.import_capture(capture)
        registers, execution, _, _ = chain(trace)
        graph = V3Graph(trace, registers, execution)
        payload = graph.export()
        store.import_v3(payload, trace)
        before = store.export_v3(trace)
        store.import_v3(payload, trace)
        assert store.export_v3(trace) == before
        store.close()


def test_epoch_isolation_unknown_control_and_roles():
    execution = ExecutionModel(TRACE)
    registers = RegisterVersionEngine(TRACE, execution_model=execution)
    registers.begin_epoch(1, 0, {"D0": 7})
    registers.begin_epoch(2, 100, {"D0": 7})
    first = registers.clear(1, 1, "epoch-1", 0x140, "D0")
    second = registers.clear(2, 101, "epoch-2", 0x140, "D0")
    assert first.id != second.id
    condition = execution.record(1, 2, 0x144, "CMPI")
    branch = execution.record(1, 3, 0x148, "BNE")
    fact = execution.condition(condition, branch, "UNKNOWN_CONTROL", None, "UNKNOWN", flags={"Z": True})
    assert fact.status == "UNKNOWN" and fact.taken is None
    assert dict(fact.flags) == {"Z": True}
    address = registers.lea(1, 4, "address", 0x14C, "A5", 0xFF13CC, ("constant",))
    op = registers.operations(1)[-1]
    assert address.register == "A5" and op.destination == "A5"
    assert op.dependency_roles == ("ADDRESS",)


def main():
    test_partial_register_semantics_and_identity()
    test_supported_rules_and_unknown_frontier()
    test_ram_interop_graph_explain_and_roles()
    test_persistence_reopen_and_duplicate()
    test_epoch_isolation_unknown_control_and_roles()
    print("PASS thor evidence v3")


if __name__ == "__main__":
    main()
