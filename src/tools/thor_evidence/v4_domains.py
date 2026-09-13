"""V4 ROM/resource/hardware domain bridge for bounded developer analysis."""
from dataclasses import asdict, dataclass

from .identity import ROM_SHA, ROM_SIZE, canonical, digest, require_hash

STATUSES = {"OBSERVED", "PROVISIONAL", "UNKNOWN", "CONFLICT"}
ROLES = {"VALUE", "ADDRESS", "CONTROL", "EXECUTION"}
ROOT_KINDS = {"ROM_DATA", "ROM_CODE_CONSTANT", "IMMEDIATE_CONSTANT", "EXTERNAL_STATE",
              "RAM", "REGISTER", "HARDWARE"}
HARDWARE_RANGES = {
    "VRAM": (0x0000, 0x10000),
    "CRAM": (0x0000, 0x80),
    "VSRAM": (0x0000, 0x50),
    "SAT": (0xD000, 0xD400),
}


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value


def _status(value):
    if value not in STATUSES:
        raise ValueError("invalid V4 evidence status")
    return value


@dataclass(frozen=True)
class RootNode:
    id: str
    trace: str
    epoch: int
    kind: str
    key: str
    value: int | str | None
    status: str
    lineage: tuple = ()


@dataclass(frozen=True)
class ResourceTransform:
    id: str
    trace: str
    epoch: int
    routine_pc: int
    decoder_id: str
    input_ids: tuple
    input_sha256: str | None
    output_sha256: str | None
    output_size: int | None
    interop: tuple
    status: str
    witness: tuple = ()


@dataclass(frozen=True)
class HardwareVersion:
    id: str
    trace: str
    epoch: int
    domain: str
    address: int
    width: int
    value: int | None
    execution_instance: str | None
    operation_id: str
    status: str


@dataclass(frozen=True)
class DmaTransfer:
    id: str
    trace: str
    epoch: int
    source_ids: tuple
    destination_domain: str
    destination_address: int
    length: int
    execution_instance: str | None
    status: str
    witness: tuple = ()


class V4DomainEngine:
    """Cross-domain graph with explicit uncertainty and no ownership writes."""

    def __init__(self, trace, rom_hash=ROM_SHA, rom_size=ROM_SIZE):
        self.trace = require_hash(trace)
        if rom_hash != ROM_SHA or rom_size != ROM_SIZE:
            raise ValueError("V4 requires the canonical ROM identity")
        self.rom_hash = rom_hash
        self.rom_size = rom_size
        self.epochs = set()
        self.roots = {}
        self.transforms = {}
        self.hardware = {}
        self.dma = {}
        self.dependencies = {}
        self.external_nodes = set()

    def begin_epoch(self, epoch):
        if type(epoch) is not int or epoch < 1 or epoch in self.epochs:
            raise ValueError("epoch must be a new positive integer")
        self.epochs.add(epoch)

    def _epoch(self, epoch):
        if epoch not in self.epochs:
            raise ValueError("unknown V4 epoch")

    def _node_id(self, kind, payload):
        return digest({"kind": kind, "value": payload})

    def add_root(self, epoch, kind, key, value=None, status="OBSERVED", lineage=()):
        self._epoch(epoch)
        if kind not in ROOT_KINDS or not isinstance(key, str) or not key:
            raise ValueError("invalid V4 root")
        _status(status)
        if kind == "ROM_DATA":
            offset = int(key, 0) if key.lower().startswith("0x") else int(key)
            if not 0 <= offset < self.rom_size:
                raise ValueError("ROM root is outside canonical ROM")
        payload = {"trace": self.trace, "epoch": epoch, "kind": kind, "key": key,
                   "value": value, "status": status, "lineage": list(lineage)}
        ident = self._node_id("thor-v4-root", payload)
        root = RootNode(ident, self.trace, epoch, kind, key, value, status, tuple(lineage))
        prior = self.roots.get(ident)
        if prior and prior != root:
            raise ValueError("V4 root identity collision")
        self.roots[ident] = root
        return root

    def link(self, source_id, target_id, role="VALUE", status="OBSERVED", rule_id="V4_LINK",
             witness=()):
        if role not in ROLES:
            raise ValueError("invalid V4 dependency role")
        _status(status)
        if source_id not in self.roots and source_id not in self.transforms and source_id not in self.hardware:
            self.external_nodes.add(source_id)
        if target_id not in self.roots and target_id not in self.transforms and target_id not in self.hardware:
            self.external_nodes.add(target_id)
        payload = {"trace": self.trace, "source": source_id, "target": target_id,
                   "role": role, "status": status, "rule_id": rule_id,
                   "witness": list(witness)}
        ident = self._node_id("thor-v4-dependency", payload)
        self.dependencies[ident] = {"id": ident, **payload}
        return self.dependencies[ident]

    def add_resource_transform(self, epoch, routine_pc, decoder_id, input_ids=(),
                               input_sha256=None, output_sha256=None, output_size=None,
                               interop=None, status="PROVISIONAL", witness=()):
        self._epoch(epoch)
        if type(routine_pc) is not int or not 0 <= routine_pc <= 0xFFFFFF or not decoder_id:
            raise ValueError("invalid resource transform contract")
        if input_sha256 is not None:
            require_hash(input_sha256)
        if output_sha256 is not None:
            require_hash(output_sha256)
        if output_size is not None and (type(output_size) is not int or output_size < 0):
            raise ValueError("invalid resource output size")
        _status(status)
        payload = {"trace": self.trace, "epoch": epoch, "routine_pc": routine_pc,
                   "decoder_id": decoder_id, "input_ids": list(input_ids),
                   "input_sha256": input_sha256, "output_sha256": output_sha256,
                   "output_size": output_size, "interop": dict(interop or {}),
                   "status": status, "witness": list(witness)}
        ident = self._node_id("thor-v4-resource-transform", payload)
        transform = ResourceTransform(ident, self.trace, epoch, routine_pc, decoder_id,
                                      tuple(input_ids), input_sha256, output_sha256, output_size,
                                      tuple(sorted((interop or {}).items())), status, tuple(witness))
        self.transforms[ident] = transform
        for source in input_ids:
            self.link(source, ident, "VALUE", status, decoder_id, witness)
        return transform

    def add_3820_transform(self, epoch, input_ids=(), input_sha256=None, output_sha256=None,
                           output_size=None, status="PROVISIONAL", witness=()):
        return self.add_resource_transform(
            epoch, 0x3820, "ancient-0x3820-checked-decoder", input_ids,
            input_sha256, output_sha256, output_size,
            {"decoder_family": "Ancient", "contract": "external-checked-decoder"},
            status, witness)

    def add_hardware_write(self, epoch, domain, address, width, value=None,
                           execution_instance=None, operation_id="HW_WRITE",
                           source_ids=(), status="OBSERVED"):
        self._epoch(epoch)
        if domain not in HARDWARE_RANGES or type(address) is not int or type(width) is not int:
            raise ValueError("invalid hardware domain write")
        lower, upper = HARDWARE_RANGES[domain]
        if width < 1 or address < lower or address + width > upper:
            raise ValueError("hardware write exceeds domain")
        _status(status)
        payload = {"trace": self.trace, "epoch": epoch, "domain": domain,
                   "address": address, "width": width, "value": value,
                   "execution_instance": execution_instance, "operation_id": operation_id,
                   "status": status}
        ident = self._node_id("thor-v4-hardware-version", payload)
        version = HardwareVersion(ident, self.trace, epoch, domain, address, width, value,
                                  execution_instance, operation_id, status)
        self.hardware[ident] = version
        for source in source_ids:
            self.link(source, ident, "VALUE", status, operation_id)
        if execution_instance:
            self.link(execution_instance, ident, "EXECUTION", status, operation_id)
        return version

    def add_dma_transfer(self, epoch, source_ids, destination_domain, destination_address,
                         length, execution_instance=None, status="PROVISIONAL", witness=()):
        self._epoch(epoch)
        if destination_domain not in HARDWARE_RANGES or type(destination_address) is not int:
            raise ValueError("invalid DMA destination")
        lower, upper = HARDWARE_RANGES[destination_domain]
        if type(length) is not int or length < 1 or destination_address < lower or destination_address + length > upper:
            raise ValueError("DMA range exceeds hardware domain")
        _status(status)
        payload = {"trace": self.trace, "epoch": epoch, "source_ids": list(source_ids),
                   "destination_domain": destination_domain, "destination_address": destination_address,
                   "length": length, "execution_instance": execution_instance,
                   "status": status, "witness": list(witness)}
        ident = self._node_id("thor-v4-dma-transfer", payload)
        transfer = DmaTransfer(ident, self.trace, epoch, tuple(source_ids), destination_domain,
                               destination_address, length, execution_instance, status, tuple(witness))
        self.dma[ident] = transfer
        hardware = self.add_hardware_write(epoch, destination_domain, destination_address,
                                           length, None, execution_instance, ident,
                                           source_ids, status)
        self.link(ident, hardware.id, "EXECUTION", status, "DMA", witness)
        return transfer, hardware

    def import_v3_graph(self, graph):
        if graph.get("schema") != "thor.evidence.v3.graph" or graph.get("trace") != self.trace:
            raise ValueError("V3 graph identity mismatch")
        for edge in graph.get("dependencies", []):
            self.external_nodes.update((edge["source"], edge["target"]))

    def export(self):
        return {"schema": "thor.evidence.v4.graph", "trace": self.trace,
                "rom": {"sha256": self.rom_hash, "size": self.rom_size},
                "epochs": sorted(self.epochs),
                "roots": [_plain(asdict(v)) for v in sorted(self.roots.values(), key=lambda x: x.id)],
                "transforms": [_plain(asdict(v)) for v in sorted(self.transforms.values(), key=lambda x: x.id)],
                "hardware": [_plain(asdict(v)) for v in sorted(self.hardware.values(), key=lambda x: x.id)],
                "dma": [_plain(asdict(v)) for v in sorted(self.dma.values(), key=lambda x: x.id)],
                "dependencies": [self.dependencies[key] for key in sorted(self.dependencies)],
                "external_nodes": sorted(self.external_nodes)}

    def explain(self, target_id):
        data = self.export()
        targets = {item["id"]: item for key in ("roots", "transforms", "hardware", "dma")
                   for item in data[key]}
        if target_id not in targets:
            raise KeyError(target_id)
        return {"target": targets[target_id],
                "dependencies": [edge for edge in data["dependencies"]
                                  if edge["target"] == target_id],
                "unresolved_frontier": [edge for edge in data["dependencies"]
                                         if edge["target"] == target_id and
                                         edge["status"] in {"UNKNOWN", "PROVISIONAL", "CONFLICT"}]}
