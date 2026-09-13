"""Coverage-bound byte versions for the developer-only evidence sidecar."""
from dataclasses import asdict, dataclass
from bisect import bisect_right
import hashlib
import re

from .identity import canonical, digest

WIDTHS = {1, 2, 4}


def _hash(value):
    return hashlib.sha256(canonical(value).encode("utf-8")).hexdigest()


def _check_hash(value):
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


@dataclass(frozen=True)
class CoverageCertificate:
    """Untrusted metadata claim. It can never authorize a PROVEN result."""
    certificate_id: str
    start_seq: int
    end_seq: int
    addresses: tuple
    evidence_hash: str
    status: str = "PROVEN"
    complete: bool = True


@dataclass(frozen=True)
class VerifiedCoverageCertificate:
    certificate_id: str
    trace: str
    epoch: int
    start_seq: int
    end_seq: int
    addresses: tuple
    raw_artifact_hash: str
    receipt_sha256: str
    decoder_id: str
    rule_id: str
    execution_instances: tuple
    basis_hash: str
    completeness: str = "PROVEN"

    @classmethod
    def from_capture(cls, claim, *, trace, epoch, raw_artifact_hash,
                     receipt_sha256, decoder_id, rule_id, execution_instances,
                     source_events):
        _validate_claim(claim)
        if claim.evidence_hash != trace or claim.status != "PROVEN" or not claim.complete:
            raise ValueError("coverage claim is not bound to trace/completeness")
        if claim.start_seq < 0 or claim.end_seq <= claim.start_seq:
            raise ValueError("verified coverage requires a non-empty temporal interval")
        if raw_artifact_hash != trace or not _check_hash(trace) or not _check_hash(receipt_sha256):
            raise ValueError("coverage lineage hashes are required")
        if not decoder_id or not rule_id or not execution_instances:
            raise ValueError("coverage decoder and execution basis are required")
        bounded = [event for event in source_events
                   if claim.start_seq <= event.get("seq", -1) <= claim.end_seq]
        sequences = [event.get("seq") for event in bounded]
        if sequences != list(range(claim.start_seq, claim.end_seq + 1)):
            raise ValueError("coverage event basis has a gap or truncation")
        if any(event.get("epoch") != epoch for event in bounded):
            raise ValueError("coverage basis crosses epoch")
        receipts = {event.get("receipt_sha256") for event in bounded}
        decoders = {event.get("decoder_id") for event in bounded}
        rules = {event.get("rule_id") for event in bounded}
        if receipts != {receipt_sha256} or decoders != {decoder_id} or rules != {rule_id}:
            raise ValueError("coverage basis is missing checked lineage metadata")
        expected = tuple(execution_instances)
        available = {str(seq) for seq in sequences}
        available.update(str(event.get("execution_instance")) for event in bounded
                         if event.get("execution_instance") is not None)
        if any(str(item) not in available for item in expected):
            raise ValueError("coverage execution basis is outside interval")
        basis = {"trace": trace, "epoch": epoch, "start": claim.start_seq,
                 "end": claim.end_seq, "addresses": list(claim.addresses),
                 "raw_artifact_hash": raw_artifact_hash, "receipt_sha256": receipt_sha256,
                 "decoder_id": decoder_id, "rule_id": rule_id,
                 "execution_instances": list(expected), "events": bounded}
        return cls(claim.certificate_id, trace, epoch, claim.start_seq, claim.end_seq,
                   tuple(claim.addresses), raw_artifact_hash, receipt_sha256,
                   decoder_id, rule_id, expected, _hash(basis))


def _validate_claim(claim):
    if not isinstance(claim, CoverageCertificate) or not claim.certificate_id:
        raise TypeError("unverified coverage claim required")
    if type(claim.start_seq) is not int or type(claim.end_seq) is not int:
        raise ValueError("invalid coverage interval")
    if claim.start_seq < 0 or claim.end_seq < claim.start_seq:
        raise ValueError("invalid coverage interval")
    if not claim.addresses or any(type(a) is not int or not 0 <= a <= 0xFFFFFF for a in claim.addresses):
        raise ValueError("invalid coverage address scope")
    if claim.status != "PROVEN" or not claim.complete or not _check_hash(claim.evidence_hash):
        raise ValueError("coverage claim is not proven")


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
    """Byte-level temporal RAM SSA with verified coverage only."""

    def __init__(self, trace):
        if not _check_hash(trace):
            raise ValueError("trace identity must be a SHA256")
        self.trace = trace
        self._epochs = {}
        self._coverage = {}

    def begin_epoch(self, epoch, start_seq=0, initial=None, origin="PRE_CAPTURE_ORIGIN"):
        if type(epoch) is not int or epoch < 1 or epoch in self._epochs:
            raise ValueError("epoch must be a new positive integer")
        if type(start_seq) is not int or start_seq < 0:
            raise ValueError("invalid epoch start")
        if origin not in {"PRE_CAPTURE_ORIGIN", "EXTERNAL_STATE", "RESET_INITIALIZATION"}:
            raise ValueError("invalid initial origin")
        self._epochs[epoch] = {"start_seq": start_seq, "next_write": 0, "current": {},
                               "roots": {}, "versions": [], "versions_by_id": {},
                               "operations": [], "operations_by_id": {},
                               "writes_by_address": {}, "write_seqs": {}, "last_seq": start_seq}
        self._coverage[epoch] = []
        if initial:
            for address, value in initial.items():
                self._ensure_initial(epoch, address, value, origin)

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
        if address in state["roots"]:
            return state["roots"][address]
        payload = {"trace": self.trace, "epoch": epoch, "address": address,
                   "version": 0, "value": value, "origin": origin}
        item = {"id": digest({"kind": "ram-byte-version-v2", "value": payload}), **payload,
                "status": "EXTERNAL_STATE" if origin == "EXTERNAL_STATE" else origin,
                "operation_id": None, "previous_version_id": None,
                "temporal_seq": state["start_seq"]}
        state["roots"][address] = item
        state["current"][address] = item
        state["versions"].append(item)
        state["versions_by_id"][item["id"]] = item
        return item

    def add_coverage(self, certificate, epoch=None):
        if not isinstance(certificate, VerifiedCoverageCertificate):
            raise TypeError("verified coverage certificate required")
        if certificate.trace != self.trace or not _check_hash(certificate.basis_hash):
            raise ValueError("coverage certificate is not bound to engine trace")
        if epoch is not None and epoch != certificate.epoch:
            raise ValueError("coverage certificate epoch mismatch")
        self._state(certificate.epoch)
        self._coverage[certificate.epoch].append(certificate)

    def write(self, epoch, temporal_seq, execution_instance, pc, rule_id, width, address,
              value, raw_witnesses=None, decoded_instruction=None):
        state = self._state(epoch)
        if width not in WIDTHS or type(address) is not int or not 0 <= address <= 0xFFFFFF:
            raise ValueError("unsupported RAM write shape")
        if address + width - 1 > 0xFFFFFF:
            raise ValueError("RAM write range exceeds address space")
        if not 0 <= value < (1 << (8 * width)):
            raise ValueError("write value does not fit width")
        if type(temporal_seq) is not int or temporal_seq < state["start_seq"]:
            raise ValueError("invalid temporal sequence")
        if temporal_seq < state["last_seq"]:
            raise ValueError("reordered write event")
        if not execution_instance or not rule_id:
            raise ValueError("operation identity is incomplete")
        payload = {"trace": self.trace, "epoch": epoch, "temporal_seq": temporal_seq,
                   "execution_instance": execution_instance, "pc": pc, "rule_id": rule_id,
                   "width": width, "effective_address": address, "value": value,
                   "raw_witnesses": raw_witnesses or [], "decoded_instruction": decoded_instruction}
        operation_id = _hash({"kind": "ram-write-operation-v2", "value": payload})
        addresses = list(range(address, address + width))
        if operation_id in state["operations_by_id"]:
            existing = state["operations_by_id"][operation_id]
            if any(existing[key] != payload[key] for key in payload):
                raise ValueError("operation identity collision")
            return existing
        previous = [state["current"].get(item) or self._ensure_initial(epoch, item)
                    for item in addresses]
        state["next_write"] += 1
        operation = {"id": operation_id, **payload, "byte_range": [address, address + width - 1],
                     "resulting_versions": [], "previous_versions": [item["id"] for item in previous]}
        values = [(value >> (8 * (width - offset - 1))) & 0xFF for offset in range(width)]
        for byte_address, byte_value, prior in zip(addresses, values, previous):
            version_payload = {"trace": self.trace, "epoch": epoch, "address": byte_address,
                               "version": state["next_write"], "value": byte_value,
                               "origin": "WRITE_OPERATION", "operation_id": operation_id,
                               "temporal_seq": temporal_seq}
            version = {"id": digest({"kind": "ram-byte-version-v2", "value": version_payload}),
                       **version_payload, "status": "OBSERVED", "previous_version_id": prior["id"]}
            state["current"][byte_address] = version
            state["versions"].append(version)
            state["versions_by_id"][version["id"]] = version
            state["writes_by_address"].setdefault(byte_address, []).append((temporal_seq, operation))
            state["write_seqs"].setdefault(byte_address, []).append(temporal_seq)
            operation["resulting_versions"].append(version["id"])
        state["operations"].append(operation)
        state["operations_by_id"][operation_id] = operation
        state["last_seq"] = max(state["last_seq"], temporal_seq)
        return operation

    def _covered(self, epoch, address, start, end, operation=None):
        intervals = []
        for cert in self._coverage[epoch]:
            if address not in cert.addresses or cert.start_seq > start or cert.end_seq < end:
                continue
            if operation is not None and (cert.rule_id not in {"*", operation["rule_id"]} or
                                          operation["execution_instance"] not in cert.execution_instances):
                continue
            intervals.append((cert.start_seq, cert.end_seq))
        cursor = start
        for left, right in sorted(intervals):
            if left > cursor:
                return False
            cursor = max(cursor, right)
            if cursor >= end:
                return True
        return bool(intervals) and cursor >= end

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
            root = self._ensure_initial(epoch, address)
            covered = self._covered(epoch, address, state["start_seq"], temporal_point)
            status = "EXTERNAL_STATE" if covered else "INCOMPLETE_CAPTURE"
            return LastWriterResult(status, self.trace, epoch, address, temporal_point,
                                    root["id"], None, "no observed writer before query")
        latest_seq = max(op["temporal_seq"] for op in writes)
        latest = [op for op in writes if op["temporal_seq"] == latest_seq]
        if len(latest) != 1:
            return LastWriterResult("CONFLICT", self.trace, epoch, address, temporal_point,
                                    None, None, "multiple overlapping writes share one temporal point")
        operation = latest[0]
        version = next(v for v in state["versions_by_id"].values()
                       if v["id"] in operation["resulting_versions"] and v["address"] == address)
        previous = state["versions_by_id"][version["previous_version_id"]]
        if operation["rule_id"] in {"UNKNOWN_TRANSFORM", "UNSUPPORTED"}:
            return LastWriterResult("UNKNOWN_TRANSFORM", self.trace, epoch, address, temporal_point,
                                    version["id"], operation["id"], "writer rule is explicitly unsupported")
        if not self._covered(epoch, address, state["start_seq"], temporal_point, operation):
            return LastWriterResult("INCOMPLETE_CAPTURE", self.trace, epoch, address, temporal_point,
                                    version["id"], operation["id"],
                                    "verified coverage is absent or has a gap")
        return LastWriterResult("PROVEN", self.trace, epoch, address, temporal_point,
                                version["id"], operation["id"],
                                f"verified coverage through seq {temporal_point}; writer seq "
                                f"{operation['temporal_seq']} replaced {previous['id']}")

    def versions(self, epoch):
        return list(self._state(epoch)["versions"])

    def operations(self, epoch):
        return list(self._state(epoch)["operations"])

    def coverage(self, epoch):
        result = []
        for item in self._coverage[epoch]:
            row = asdict(item) | {"addresses": list(item.addresses),
                                  "execution_instances": list(item.execution_instances),
                                  "evidence_hash": item.trace, "status": "PROVEN",
                                  "complete": True}
            result.append(row)
        return result

    def export(self):
        return {"schema": "thor.evidence.ram-v2", "trace": self.trace,
                "epochs": {str(epoch): {"versions": self.versions(epoch),
                                         "operations": self.operations(epoch),
                                         "coverage": self.coverage(epoch)}
                            for epoch in sorted(self._epochs)}}
