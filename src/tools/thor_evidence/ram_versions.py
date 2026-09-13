"""Reusable, coverage-aware RAM byte versions for the V2 evidence sidecar.

This module models observed writes only.  A writer becomes PROVEN through an
explicit complete coverage certificate; the latest observed event is never a
proof by itself.
"""
from dataclasses import asdict, dataclass
from bisect import bisect_right
import hashlib
import re

from .identity import canonical, digest

STATUSES = {"PROVEN", "EXTERNAL_STATE", "RESET_INITIALIZATION", "PRE_CAPTURE_ORIGIN", "INCOMPLETE_CAPTURE",
            "UNKNOWN_TRANSFORM", "CONFLICT"}
WIDTHS = {1, 2, 4}


def _hash(value):
    return hashlib.sha256(canonical(value).encode("utf-8")).hexdigest()


def _check_hash(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


@dataclass(frozen=True)
class CoverageCertificate:
    certificate_id: str
    start_seq: int
    end_seq: int
    addresses: tuple
    evidence_hash: str
    status: str = "PROVEN"
    complete: bool = True

    def validate(self):
        if not self.certificate_id or self.start_seq < 0 or self.end_seq < self.start_seq:
            raise ValueError("invalid coverage interval")
        if not self.addresses or any(type(a) is not int or not 0 <= a <= 0xFFFFFF for a in self.addresses):
            raise ValueError("invalid coverage address scope")
        if self.status != "PROVEN" or not self.complete or not _check_hash(self.evidence_hash):
            raise ValueError("coverage certificate is not proven")


@dataclass(frozen=True)
class LastWriterResult:
    status: str
    trace: str
    epoch: int
    address: int
    temporal_point: int
    version_id: str | None
    operation_id: str | None
    explanation: str

    def as_dict(self):
        return asdict(self)


class RamVersionEngine:
    """Byte-level temporal RAM SSA with explicit epoch and coverage scopes."""

    def __init__(self, trace):
        if not trace:
            raise ValueError("trace identity is required")
        self.trace = trace
        self._epochs = {}
        self._coverage = {}

    def begin_epoch(self, epoch, start_seq=0, initial=None, origin="PRE_CAPTURE_ORIGIN"):
        if type(epoch) is not int or epoch < 1 or epoch in self._epochs:
            raise ValueError("epoch must be a new positive integer")
        if origin not in {"PRE_CAPTURE_ORIGIN", "EXTERNAL_STATE", "RESET_INITIALIZATION"}:
            raise ValueError("invalid initial origin")
        self._epochs[epoch] = {"start_seq": start_seq, "next_write": 0, "current": {},
                               "versions": [], "versions_by_id": {}, "operations": [],
                               "writes_by_address": {}, "write_seqs": {}, "last_seq": start_seq}
        if initial:
            for address, value in initial.items():
                self._ensure_initial(epoch, address, value, origin)
        self._coverage[epoch] = []

    def _state(self, epoch):
        if epoch not in self._epochs:
            raise ValueError("unknown restore epoch")
        return self._epochs[epoch]

    def _ensure_initial(self, epoch, address, value=0, origin="PRE_CAPTURE_ORIGIN"):
        state = self._state(epoch)
        if type(address) is not int or not 0 <= address <= 0xFFFFFF:
            raise ValueError("invalid RAM address")
        if type(value) is not int or not 0 <= value <= 0xFF:
            raise ValueError("RAM byte value required")
        if address in state["current"]:
            return state["current"][address]
        payload = {"trace": self.trace, "epoch": epoch, "address": address,
                   "version": 0, "value": value, "origin": origin}
        item = {"id": digest({"kind": "ram-byte-version-v2", "value": payload}), **payload,
                "status": "EXTERNAL_STATE" if origin == "EXTERNAL_STATE" else origin,
                "operation_id": None, "previous_version_id": None, "temporal_seq": state["start_seq"]}
        state["current"][address] = item
        state["versions"].append(item)
        state["versions_by_id"][item["id"]] = item
        return item

    def add_coverage(self, certificate, epoch=None):
        if not isinstance(certificate, CoverageCertificate):
            raise TypeError("CoverageCertificate required")
        certificate.validate()
        if epoch is None:
            epochs = [number for number, state in self._epochs.items()
                      if certificate.start_seq >= state["start_seq"]]
            if len(epochs) != 1:
                raise ValueError("coverage certificate must identify one epoch")
            epoch = epochs[0]
        self._state(epoch)
        if certificate.evidence_hash != self.trace:
            raise ValueError("coverage evidence is not bound to trace identity")
        self._coverage[epoch].append(certificate)

    def write(self, epoch, temporal_seq, execution_instance, pc, rule_id, width, address,
              value, raw_witnesses=None, decoded_instruction=None):
        state = self._state(epoch)
        if width not in WIDTHS or type(address) is not int or not 0 <= address <= 0xFFFFFF:
            raise ValueError("unsupported RAM write shape")
        if not 0 <= value < (1 << (8 * width)):
            raise ValueError("write value does not fit width")
        if type(temporal_seq) is not int or temporal_seq < state["start_seq"]:
            raise ValueError("invalid temporal sequence")
        if temporal_seq < state["last_seq"]:
            raise ValueError("reordered write event")
        if not execution_instance or not rule_id:
            raise ValueError("operation identity is incomplete")
        operation_payload = {"trace": self.trace, "epoch": epoch,
                             "temporal_seq": temporal_seq, "execution_instance": execution_instance,
                             "pc": pc, "rule_id": rule_id, "width": width,
                             "effective_address": address, "value": value,
                             "raw_witnesses": raw_witnesses or [],
                             "decoded_instruction": decoded_instruction}
        operation_id = _hash({"kind": "ram-write-operation-v2", "value": operation_payload})
        operation = {"id": operation_id, **operation_payload, "byte_range": [address, address + width - 1],
                     "resulting_versions": [], "previous_versions": []}
        state["next_write"] += 1
        for offset in range(width):
            byte_address = address + offset
            byte_value = (value >> (8 * (width - offset - 1))) & 0xFF
            previous = self._ensure_initial(epoch, byte_address)
            payload = {"trace": self.trace, "epoch": epoch, "address": byte_address,
                       "version": state["next_write"], "value": byte_value,
                       "origin": "WRITE_OPERATION", "operation_id": operation_id,
                       "temporal_seq": temporal_seq}
            version = {"id": digest({"kind": "ram-byte-version-v2", "value": payload}), **payload,
                       "status": "OBSERVED", "previous_version_id": previous["id"]}
            state["current"][byte_address] = version
            state["versions"].append(version)
            state["versions_by_id"][version["id"]] = version
            state["writes_by_address"].setdefault(byte_address, []).append((temporal_seq, operation))
            state["write_seqs"].setdefault(byte_address, []).append(temporal_seq)
            operation["previous_versions"].append(previous["id"])
            operation["resulting_versions"].append(version["id"])
        state["operations"].append(operation)
        state["last_seq"] = max(state["last_seq"], temporal_seq)
        return operation

    def _covered(self, epoch, address, start, end):
        intervals = [(c.start_seq, c.end_seq) for c in self._coverage[epoch] if address in c.addresses]
        cursor = start
        for left, right in sorted(intervals):
            if right < cursor:
                continue
            if left > cursor:
                return False
            cursor = max(cursor, right)
            if cursor >= end:
                return True
        return cursor >= end

    def last_writer(self, epoch, address, temporal_point):
        state = self._state(epoch)
        if type(address) is not int or not 0 <= address <= 0xFFFFFF or type(temporal_point) is not int:
            raise ValueError("invalid last-writer query")
        if temporal_point < state["start_seq"]:
            return LastWriterResult("PRE_CAPTURE_ORIGIN", self.trace, epoch, address, temporal_point,
                                    None, None, "query precedes epoch capture boundary")
        address_writes = state["writes_by_address"].get(address, [])
        index = bisect_right(state["write_seqs"].get(address, []), temporal_point)
        writes = [item[1] for item in address_writes[:index]]
        if not writes:
            current = self._ensure_initial(epoch, address)
            if self._covered(epoch, address, state["start_seq"], temporal_point):
                return LastWriterResult("EXTERNAL_STATE", self.trace, epoch, address, temporal_point,
                                        current["id"], None, "complete coverage shows no write before query")
            return LastWriterResult("INCOMPLETE_CAPTURE", self.trace, epoch, address, temporal_point,
                                    current["id"], None, "no observed writer and coverage has a gap")
        latest_seq = max(op["temporal_seq"] for op in writes)
        latest = [op for op in writes if op["temporal_seq"] == latest_seq]
        if len(latest) != 1:
            return LastWriterResult("CONFLICT", self.trace, epoch, address, temporal_point,
                                    None, None, "multiple overlapping writes share one temporal point")
        operation = latest[0]
        version = next(v for v in state["versions_by_id"].values() if v["id"] in operation["resulting_versions"] and
                       v["address"] == address)
        previous = state["versions_by_id"][version["previous_version_id"]]
        if operation["rule_id"] in {"UNKNOWN_TRANSFORM", "UNSUPPORTED"}:
            return LastWriterResult("UNKNOWN_TRANSFORM", self.trace, epoch, address, temporal_point,
                                    version["id"], operation["id"], "writer rule is explicitly unsupported")
        if not self._covered(epoch, address, state["start_seq"], temporal_point):
            return LastWriterResult("INCOMPLETE_CAPTURE", self.trace, epoch, address, temporal_point,
                                    version["id"], operation["id"],
                                    "latest observed writer is not proven because coverage has a gap")
        return LastWriterResult("PROVEN", self.trace, epoch, address, temporal_point,
                                version["id"], operation["id"],
                                f"complete coverage from epoch start through seq {temporal_point}; "
                                f"writer seq {operation['temporal_seq']} replaced {previous['id']}")

    def versions(self, epoch):
        return list(self._state(epoch)["versions"])

    def operations(self, epoch):
        return list(self._state(epoch)["operations"])

    def coverage(self, epoch):
        self._state(epoch)
        return [{**asdict(item), "addresses": list(item.addresses)} for item in self._coverage[epoch]]

    def export(self):
        return {"schema": "thor.evidence.ram-v2", "trace": self.trace,
                "epochs": {str(epoch): {"versions": self.versions(epoch),
                                         "operations": self.operations(epoch),
                                         "coverage": self.coverage(epoch)}
                            for epoch in sorted(self._epochs)}}
