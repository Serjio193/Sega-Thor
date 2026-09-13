"""Bounded execution, call/return and local control facts for V3."""
from dataclasses import dataclass, asdict

from .identity import digest, require_hash


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value


@dataclass(frozen=True)
class ExecutionInstance:
    id: str
    trace: str
    epoch: int
    seq: int
    pc: int
    rule_id: str


@dataclass(frozen=True)
class Relation:
    id: str
    kind: str
    source: str
    target: str
    status: str
    witness: tuple = ()


@dataclass(frozen=True)
class ControlFact:
    id: str
    execution_id: str
    condition_rule: str
    branch_execution_id: str
    taken: bool | None
    status: str
    witness: tuple = ()
    flags: tuple = ()


class ExecutionModel:
    """Execution identity is (trace, epoch, sequence, PC), never PC alone."""

    def __init__(self, trace):
        self.trace = require_hash(trace)
        self.instances = {}
        self.relations = {}
        self.controls = {}

    def record(self, epoch, seq, pc, rule_id="EXEC"):
        if type(epoch) is not int or epoch < 1 or type(seq) is not int or seq < 0:
            raise ValueError("invalid execution identity")
        if type(pc) is not int or not 0 <= pc <= 0xFFFFFF or not rule_id:
            raise ValueError("invalid execution witness")
        payload = {"trace": self.trace, "epoch": epoch, "seq": seq,
                   "pc": pc, "rule_id": rule_id}
        ident = digest({"kind": "thor-v3-execution-instance", "value": payload})
        item = ExecutionInstance(ident, self.trace, epoch, seq, pc, rule_id)
        prior = self.instances.get(ident)
        if prior and prior != item:
            raise ValueError("execution identity collision")
        self.instances[ident] = item
        return item

    def _relation(self, kind, source, target, status="OBSERVED", witness=()):
        if kind not in {"CALL", "RETURN"}:
            raise ValueError("unsupported execution relation")
        payload = {"kind": kind, "source": source, "target": target,
                   "status": status, "witness": list(witness)}
        ident = digest({"kind": "thor-v3-execution-relation", "value": payload})
        relation = Relation(ident, kind, source, target, status, tuple(witness))
        self.relations[ident] = relation
        return relation

    def call(self, caller, callee, witness=()):
        return self._relation("CALL", caller.id if hasattr(caller, "id") else caller,
                              callee.id if hasattr(callee, "id") else callee, witness=witness)

    call_instance = call

    def ret(self, callee, continuation, witness=()):
        return self._relation("RETURN", callee.id if hasattr(callee, "id") else callee,
                              continuation.id if hasattr(continuation, "id") else continuation,
                              witness=witness)

    return_instance = ret

    def branch(self, condition_execution, branch_execution, condition_rule,
               taken, status="OBSERVED", witness=(), flags=None):
        if status not in {"OBSERVED", "PROVISIONAL", "UNKNOWN", "CONFLICT"}:
            raise ValueError("invalid control status")
        if taken is not None and type(taken) is not bool:
            raise ValueError("branch outcome must be bool or None")
        payload = {"execution_id": getattr(condition_execution, "id", condition_execution),
                   "condition_rule": condition_rule,
                   "branch_execution_id": getattr(branch_execution, "id", branch_execution),
                   "taken": taken, "status": status, "witness": list(witness),
                   "flags": dict(flags or {})}
        if any(name not in {"N", "Z", "V", "C", "X"} or type(value) is not bool
               for name, value in payload["flags"].items()):
            raise ValueError("unsupported CCR fact")
        ident = digest({"kind": "thor-v3-control-fact", "value": payload})
        fact = ControlFact(ident, payload["execution_id"], condition_rule,
                           payload["branch_execution_id"], taken, status, tuple(witness),
                           tuple(sorted(payload["flags"].items())))
        self.controls[ident] = fact
        return fact

    control_fact = branch

    def condition(self, condition_execution, branch_execution, rule, taken=None,
                  status="OBSERVED", witness=(), flags=None):
        return self.branch(condition_execution, branch_execution, rule, taken, status, witness, flags)

    def export(self):
        return {"schema": "thor.evidence.v3.execution", "trace": self.trace,
                "instances": [_plain(asdict(v)) for v in sorted(self.instances.values(), key=lambda x: x.id)],
                "relations": [_plain(asdict(v)) for v in sorted(self.relations.values(), key=lambda x: x.id)],
                "controls": [_plain(asdict(v)) for v in sorted(self.controls.values(), key=lambda x: x.id)]}
