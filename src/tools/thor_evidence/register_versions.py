"""Temporal M68K register slices used by the V3 build skeleton."""
from dataclasses import asdict, dataclass

from .identity import digest, require_hash


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value

REGISTERS = {*(f"D{i}" for i in range(8)), *(f"A{i}" for i in range(8))}
WIDTHS = {8, 16, 24, 32}
ROLES = {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}
STATUSES = {"OBSERVED", "PROVISIONAL", "UNKNOWN", "CONFLICT", "PRE_CAPTURE_ORIGIN"}


@dataclass(frozen=True)
class RegisterVersion:
    id: str
    trace: str
    epoch: int
    register: str
    bit_offset: int
    bit_width: int
    value: int | None
    version_no: int
    temporal_seq: int
    execution_instance: str | None
    operation_id: str | None
    status: str
    previous_version_id: str | None
    dependencies: tuple = ()
    origin: str = "OPERATION"


@dataclass(frozen=True)
class RegisterOperation:
    id: str
    trace: str
    epoch: int
    temporal_seq: int
    execution_instance: str
    pc: int
    rule_id: str
    destination: str
    bit_offset: int
    bit_width: int
    inputs: tuple
    dependency_roles: tuple
    status: str
    witness: tuple = ()


def _slice(register, bit_offset, bit_width):
    if register not in REGISTERS or bit_width not in WIDTHS:
        raise ValueError("unsupported register slice")
    limit = 32
    if type(bit_offset) is not int or bit_offset < 0 or bit_offset + bit_width > limit:
        raise ValueError("register slice exceeds 32 bits")
    if register.startswith("A") and bit_width != 32:
        raise ValueError("address registers require long slices")
    return bit_offset, bit_width


class RegisterVersionEngine:
    """Slice-aware temporal register store; unknown rules never guess values."""

    def __init__(self, trace, ram_engine=None, execution_model=None):
        self.trace = require_hash(trace)
        self.ram_engine = ram_engine
        self.execution_model = execution_model
        self.epochs = {}
        self.ram_edges = []

    def _version_by_id(self, ident):
        for state in self.epochs.values():
            for version in state["versions"]:
                if version.id == ident:
                    return version
        return None

    def begin_epoch(self, epoch, start_seq=0, initial=None):
        if type(epoch) is not int or epoch < 1 or epoch in self.epochs:
            raise ValueError("epoch must be a new positive integer")
        if type(start_seq) is not int or start_seq < 0:
            raise ValueError("invalid epoch start")
        state = {"start": start_seq, "last": start_seq, "counter": 0,
                 "slices": {}, "versions": [], "operations": []}
        self.epochs[epoch] = state
        for register, value in (initial or {}).items():
            self.seed(epoch, register, value, start_seq)

    def _state(self, epoch):
        if epoch not in self.epochs:
            raise ValueError("unknown register epoch")
        return self.epochs[epoch]

    def seed(self, epoch, register, value, temporal_seq=None, status="PRE_CAPTURE_ORIGIN"):
        state = self._state(epoch)
        _slice(register, 0, 32)
        if type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("register seed must be a 32-bit integer")
        seq = state["start"] if temporal_seq is None else temporal_seq
        return self._version(epoch, seq, None, 0, register, 0, 32, value,
                             None, status, None, (), "ROOT")

    def read(self, epoch, register, bit_offset=0, bit_width=32):
        state = self._state(epoch)
        _slice(register, bit_offset, bit_width)
        mask = (1 << bit_width) - 1
        value = 0
        ids = []
        for offset in range(bit_offset, bit_offset + bit_width, 8):
            part = self._latest_covering(state, register, offset, min(8, bit_offset + bit_width - offset))
            if part is None or part.value is None:
                return None, tuple(ids)
            width = part.bit_width
            shift = offset - part.bit_offset
            value |= ((part.value >> shift) & ((1 << min(8, width - shift)) - 1)) << (offset - bit_offset)
            ids.append(part.id)
        return value & mask, tuple(dict.fromkeys(ids))

    def current(self, epoch, register):
        return self.read(epoch, register, 0, 32)[0]

    def _latest_covering(self, state, register, offset, width):
        choices = [v for v in state["versions"] if v.register == register and
                   v.bit_offset <= offset and v.bit_offset + v.bit_width >= offset + width]
        return max(choices, key=lambda v: (v.temporal_seq, v.version_no), default=None)

    def _version(self, epoch, seq, execution_instance, pc, register, bit_offset, bit_width,
                 value, operation_id, status, previous, dependencies, origin="OPERATION"):
        state = self._state(epoch)
        if seq < state["last"]:
            raise ValueError("register time moved backwards")
        if value is not None and (type(value) is not int or not 0 <= value < 1 << bit_width):
            raise ValueError("register value does not fit slice")
        state["counter"] += 1
        payload = {"trace": self.trace, "epoch": epoch, "register": register,
                   "bit_offset": bit_offset, "bit_width": bit_width, "value": value,
                   "version_no": state["counter"], "temporal_seq": seq,
                   "execution_instance": execution_instance, "operation_id": operation_id,
                   "status": status, "previous_version_id": previous, "dependencies": list(dependencies),
                   "origin": origin}
        ident = digest({"kind": "thor-v3-register-version", "value": payload})
        version = RegisterVersion(ident, self.trace, epoch, register, bit_offset, bit_width,
                                 value, state["counter"], seq, execution_instance, operation_id,
                                 status, previous, tuple(dependencies), origin)
        state["versions"].append(version)
        state["slices"].setdefault((register, bit_offset, bit_width), []).append(version)
        state["last"] = max(state["last"], seq)
        return version

    def apply(self, epoch, temporal_seq, execution_instance, pc, rule_id, destination,
              bit_width, value=None, inputs=(), roles=(), status="OBSERVED", witness=()):
        bit_offset = 0
        if destination.startswith("A"):
            bit_width = 32
        _slice(destination, bit_offset, bit_width)
        state = self._state(epoch)
        prior = self._latest_covering(state, destination, bit_offset, bit_width)
        if status not in STATUSES or len(inputs) != len(roles) or any(r not in ROLES for r in roles):
            raise ValueError("invalid operation status or dependency roles")
        for source in inputs:
            version = self._version_by_id(source)
            if version and (version.epoch != epoch or version.temporal_seq > temporal_seq):
                raise ValueError("temporal DAG cannot point backwards")
        payload = {"trace": self.trace, "epoch": epoch, "temporal_seq": temporal_seq,
                   "execution_instance": execution_instance, "pc": pc, "rule_id": rule_id,
                   "destination": destination, "bit_offset": bit_offset, "bit_width": bit_width,
                   "inputs": list(inputs), "dependency_roles": list(roles), "status": status,
                   "witness": list(witness)}
        op_id = digest({"kind": "thor-v3-register-operation", "value": payload})
        operation = RegisterOperation(op_id, self.trace, epoch, temporal_seq, execution_instance,
                                      pc, rule_id, destination, bit_offset, bit_width,
                                      tuple(inputs), tuple(roles), status, tuple(witness))
        state["operations"].append(operation)
        return self._version(epoch, temporal_seq, execution_instance, pc, destination, bit_offset,
                             bit_width, value, op_id, status, prior.id if prior else None,
                             tuple(inputs), "OPERATION")

    def move(self, epoch, seq, execution_instance, pc, destination, source, bit_width):
        value, inputs = self.read(epoch, source, 0, bit_width)
        return self.apply(epoch, seq, execution_instance, pc, f"MOVE.{bit_width}", destination,
                          bit_width, value, inputs, ("VALUE",) * len(inputs),
                          "OBSERVED" if value is not None else "UNKNOWN")

    def movea_w(self, epoch, seq, execution_instance, pc, destination, value, inputs=()):
        if value is None:
            result, status = None, "UNKNOWN"
        else:
            raw = value & 0xFFFF
            result, status = (raw | 0xFFFF0000 if raw & 0x8000 else raw), "OBSERVED"
        return self.apply(epoch, seq, execution_instance, pc, "MOVEA.W", destination, 32,
                          result, inputs, ("VALUE",) * len(inputs), status)

    def movea_l(self, epoch, seq, execution_instance, pc, destination, value, inputs=()):
        status = "OBSERVED" if value is not None else "UNKNOWN"
        return self.apply(epoch, seq, execution_instance, pc, "MOVEA.L", destination, 32,
                          None if value is None else value & 0xFFFFFFFF,
                          inputs, ("VALUE",) * len(inputs), status)

    def moveq(self, epoch, seq, execution_instance, pc, destination, value):
        raw = value & 0xFF
        result = raw | 0xFFFFFF00 if raw & 0x80 else raw
        return self.apply(epoch, seq, execution_instance, pc, "MOVEQ", destination, 32,
                          result, (), ())

    def _binary(self, epoch, seq, execution_instance, pc, rule, destination, amount,
                operation, bit_width=32):
        current, inputs = self.read(epoch, destination, 0, bit_width)
        value = None if current is None else operation(current, amount) & ((1 << bit_width) - 1)
        return self.apply(epoch, seq, execution_instance, pc, rule, destination, bit_width,
                          value, inputs, ("VALUE",) * len(inputs),
                          "OBSERVED" if value is not None else "UNKNOWN")

    def add(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "ADD", destination, amount,
                            lambda left, right: left + right, bit_width)

    def addi(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "ADDI", destination, amount,
                            lambda left, right: left + right, bit_width)

    def sub(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "SUB", destination, amount,
                            lambda left, right: left - right, bit_width)

    def subi(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "SUBI", destination, amount,
                            lambda left, right: left - right, bit_width)

    def addq(self, epoch, seq, execution_instance, pc, destination, amount=1, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "ADDQ", destination, amount,
                            lambda left, right: left + right, bit_width)

    def adda(self, epoch, seq, execution_instance, pc, destination, amount):
        return self._binary(epoch, seq, execution_instance, pc, "ADDA", destination, amount,
                            lambda left, right: left + right, 32)

    def subq(self, epoch, seq, execution_instance, pc, destination, amount=1, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, "SUBQ", destination, amount,
                            lambda left, right: left - right, bit_width)

    def suba(self, epoch, seq, execution_instance, pc, destination, amount):
        return self._binary(epoch, seq, execution_instance, pc, "SUBA", destination, amount,
                            lambda left, right: left - right, 32)

    def effective_address(self, epoch, seq, execution_instance, pc, destination, mode,
                          base=0, displacement=0, index=0, absolute=None, inputs=()):
        modes = {"Dn", "An", "(An)", "(An)+", "-(An)", "d16(An)", "d8(An,Xn)",
                 "absolute_short", "absolute_long", "PC-relative", "immediate"}
        if mode not in modes:
            return self.unsupported(epoch, seq, execution_instance, pc, destination, 32)
        value = (absolute if absolute is not None else base + displacement + index) & 0xFFFFFFFF
        return self.lea(epoch, seq, execution_instance, pc, destination, value, inputs)

    def logical(self, epoch, seq, execution_instance, pc, rule, destination, amount,
                operation, bit_width=32):
        return self._binary(epoch, seq, execution_instance, pc, rule, destination, amount,
                            operation, bit_width)

    def and_(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self.logical(epoch, seq, execution_instance, pc, "AND", destination, amount,
                            lambda left, right: left & right, bit_width)

    def or_(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self.logical(epoch, seq, execution_instance, pc, "OR", destination, amount,
                            lambda left, right: left | right, bit_width)

    def eor(self, epoch, seq, execution_instance, pc, destination, amount, bit_width=32):
        return self.logical(epoch, seq, execution_instance, pc, "EOR", destination, amount,
                            lambda left, right: left ^ right, bit_width)

    def ext(self, epoch, seq, execution_instance, pc, destination, from_width=8):
        current, inputs = self.read(epoch, destination, 0, from_width)
        if current is None:
            value = None
        else:
            sign = 1 << (from_width - 1)
            value = current | ((0xFFFFFFFF ^ ((1 << from_width) - 1)) if current & sign else 0)
        return self.apply(epoch, seq, execution_instance, pc, "EXT", destination, 32, value,
                          inputs, ("VALUE",) * len(inputs), "OBSERVED" if value is not None else "UNKNOWN")

    def swap(self, epoch, seq, execution_instance, pc, destination):
        current, inputs = self.read(epoch, destination, 0, 32)
        value = None if current is None else ((current & 0xFFFF) << 16) | (current >> 16)
        return self.apply(epoch, seq, execution_instance, pc, "SWAP", destination, 32, value,
                          inputs, ("VALUE",) * len(inputs), "OBSERVED" if value is not None else "UNKNOWN")

    def shift(self, epoch, seq, execution_instance, pc, destination, amount, left=True,
              arithmetic=False, bit_width=32):
        rule = ("ASL" if arithmetic else "LSL") if left else ("ASR" if arithmetic else "LSR")
        if left:
            operation = lambda value, count: value << count
        elif arithmetic:
            operation = lambda value, count: (value if value < (1 << (bit_width - 1)) else
                                               value - (1 << bit_width)) >> count
        else:
            operation = lambda value, count: value >> count
        return self._binary(epoch, seq, execution_instance, pc, rule, destination, amount,
                            operation, bit_width)

    def compare(self, epoch, seq, execution_instance, pc, rule, destination, amount,
                bit_width=32):
        value, inputs = self.read(epoch, destination, 0, bit_width)
        status = "OBSERVED" if value is not None else "UNKNOWN"
        return self.apply(epoch, seq, execution_instance, pc, rule, destination, bit_width,
                          value, inputs, ("CONTROL",) * len(inputs), status)

    def addq_b(self, epoch, seq, execution_instance, pc, destination, amount=1):
        current, inputs = self.read(epoch, destination, 0, 8)
        value = None if current is None else (current + amount) & 0xFF
        return self.apply(epoch, seq, execution_instance, pc, "ADDQ.B", destination, 8,
                          value, inputs, ("VALUE",) * len(inputs),
                          "OBSERVED" if value is not None else "UNKNOWN")

    def clear(self, epoch, seq, execution_instance, pc, destination, bit_width=32):
        return self.apply(epoch, seq, execution_instance, pc, "CLR", destination, bit_width,
                          0, (), ())

    def lea(self, epoch, seq, execution_instance, pc, destination, address, inputs=()):
        return self.apply(epoch, seq, execution_instance, pc, "LEA", destination, 32,
                          address & 0xFFFFFFFF, inputs, ("ADDRESS",) * len(inputs))

    def write_ram(self, epoch, seq, execution_instance, pc, rule_id, address,
                  source_register, width=4, address_inputs=()):
        if self.ram_engine is None:
            raise ValueError("RAM engine is required for register to RAM writes")
        value, inputs = self.read(epoch, source_register, 0, width * 8)
        if value is None:
            raise ValueError("RAM write source is unresolved")
        ram_operation = self.ram_engine.write(epoch, seq, execution_instance, pc, rule_id,
                                               width, address, value)
        source_status = "OBSERVED"
        for ident in inputs:
            version = self._version_by_id(ident)
            if version and version.status not in {"OBSERVED", "PRE_CAPTURE_ORIGIN"}:
                source_status = "PROVISIONAL"
        self.ram_edges.append({"ram_operation_id": ram_operation["id"],
                               "source_versions": list(inputs),
                               "address_sources": list(address_inputs),
                               "roles": ["VALUE"] * len(inputs) + ["ADDRESS"] * len(address_inputs),
                               "status": source_status})
        return ram_operation

    def unsupported(self, epoch, seq, execution_instance, pc, destination, bit_width=32):
        return self.apply(epoch, seq, execution_instance, pc, "UNKNOWN_TRANSFORM", destination,
                          bit_width, None, (), (), "UNKNOWN")

    def read_ram(self, epoch, seq, execution_instance, pc, destination, value, source_id,
                 bit_width=32, source_status="OBSERVED"):
        status = "OBSERVED" if source_status in {"OBSERVED", "PROVEN"} else "PROVISIONAL"
        return self.apply(epoch, seq, execution_instance, pc, "RAM_READ", destination, bit_width,
                          value, (source_id,), ("VALUE",), status)

    def export(self):
        return {"schema": "thor.evidence.v3.register", "trace": self.trace,
                "epochs": {str(epoch): {"versions": [_plain(asdict(v)) for v in state["versions"]],
                                         "operations": [_plain(asdict(v)) for v in state["operations"]]}
                           for epoch, state in sorted(self.epochs.items())}}

    def versions(self, epoch):
        return list(self._state(epoch)["versions"])

    def operations(self, epoch):
        return list(self._state(epoch)["operations"])
