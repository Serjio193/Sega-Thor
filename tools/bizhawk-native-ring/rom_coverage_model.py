"""Read-only ROM property checkpoint decoding and coverage-cell aggregation."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct


PROPERTIES = (
    (1 << 0, "M68K_EXECUTED_ENCODING"),
    (1 << 1, "Z80_EXECUTED_ENCODING"),
    (1 << 2, "M68K_DATA_READ"),
    (1 << 3, "Z80_DATA_READ"),
    (1 << 4, "VDP_VRAM_SOURCE"),
    (1 << 5, "VDP_CRAM_SOURCE"),
    (1 << 6, "VDP_VSRAM_SOURCE"),
    (1 << 7, "AUDIO_PAYLOAD_PROVEN"),
    (1 << 8, "COMPRESSED_GRAPHICS_SOURCE"),
)
OBSERVATION_ONLY_MASK = (1 << 2) | (1 << 3)
CLASSIFIED_MASK = sum(bit for bit, _ in PROPERTIES if bit & ~OBSERVATION_ONLY_MASK)
CONTRACT_SHA256 = "c88eb4dcc273b55681bc1d0fc04e483b4d200d292b054488f7c87348b2c07842"
COLORS = {
    "UNKNOWN": "#252b35",
    "M68K_EXECUTED_ENCODING": "#e34b4b",
    "Z80_EXECUTED_ENCODING": "#ff9f43",
    "M68K_DATA_READ": "#4f86f7",
    "Z80_DATA_READ": "#9b6bdb",
    "VDP_VRAM_SOURCE": "#35b779",
    "VDP_CRAM_SOURCE": "#22a6a1",
    "VDP_VSRAM_SOURCE": "#9bc53d",
    "AUDIO_PAYLOAD_PROVEN": "#e056c4",
    "COMPRESSED_GRAPHICS_SOURCE": "#d08a39",
    "MIXED": "#f2d34f",
    "OBSERVED_UNCLASSIFIED": "#477da8",
}


@dataclass(frozen=True)
class Checkpoint:
    identity: dict[str, object]
    masks: bytes  # one little-endian uint16 property mask per ROM byte
    checksum: str

    @property
    def rom_size(self) -> int:
        return len(self.masks) // 2


def _load_checkpoint(path: str | Path, expected_contract: str) -> Checkpoint:
    """Decode a checkpoint, requiring the caller's explicitly accepted contract."""
    raw = Path(path).read_bytes()
    if len(raw) < 8 + 4 + 64 or raw[:8] != b"OASROMP1":
        raise ValueError("Invalid ROM property checkpoint header")
    body, stored = raw[:-64], raw[-64:].decode("ascii", errors="strict")
    actual = hashlib.sha256(body).hexdigest()
    if actual != stored:
        raise ValueError("ROM property checkpoint checksum mismatch")
    offset = 8

    def u32() -> int:
        nonlocal offset
        if offset + 4 > len(body):
            raise ValueError("Truncated checkpoint")
        value = struct.unpack_from("<I", body, offset)[0]
        offset += 4
        return value

    def u64() -> int:
        nonlocal offset
        if offset + 8 > len(body):
            raise ValueError("Truncated checkpoint")
        value = struct.unpack_from("<Q", body, offset)[0]
        offset += 8
        return value

    def string() -> str:
        nonlocal offset
        count = u32()
        if offset + count > len(body):
            raise ValueError("Truncated checkpoint metadata")
        value = body[offset:offset + count].decode("utf-8")
        offset += count
        return value

    if u32() != 1:
        raise ValueError("Unsupported checkpoint version")
    identity = {
        "rom_sha256": string(), "rom_size": u64(), "schema_id": string(),
        "contract_sha256": string(), "core_build_id": string(), "run_id": string(),
        "generation": u64(), "capabilities": u64(), "validation_state": string(),
    }
    count = u64()
    if count != identity["rom_size"] or count > (len(body) - offset) // 2:
        raise ValueError("Checkpoint ROM size and bitmap disagree")
    masks = body[offset:offset + count * 2]
    if offset + len(masks) != len(body):
        raise ValueError("Checkpoint has trailing or missing bitmap bytes")
    if any(value & 0xFE00 for (value,) in struct.iter_unpack("<H", masks)):
        raise ValueError("Checkpoint contains unknown property bits")
    if identity["schema_id"] != "thor.rom-properties.v1":
        raise ValueError("Unsupported ROM property schema")
    if identity["contract_sha256"] != expected_contract:
        raise ValueError("Unsupported ROM property proof contract")
    if any(len(str(identity[key])) != 64 or any(
            char not in "0123456789abcdef" for char in str(identity[key]))
           for key in ("rom_sha256", "contract_sha256", "core_build_id")):
        raise ValueError("Checkpoint identity hash is malformed")
    run_id = str(identity["run_id"])
    valid_run_id = 0 < len(run_id) <= 128 and all(
        char.isascii() and (char.isalnum() or char in "-_") for char in run_id)
    if (identity["rom_size"] <= 0 or not valid_run_id or
            identity["validation_state"] not in ("VALIDATED", "PARTIAL")):
        raise ValueError("Checkpoint identity is incomplete")
    capabilities = int(identity["capabilities"])
    if capabilities & ~0x1FF:
        raise ValueError("Checkpoint declares unknown property capabilities")
    if any(value & ~capabilities for (value,) in struct.iter_unpack("<H", masks)):
        raise ValueError("Checkpoint bitmap exceeds declared capabilities")
    return Checkpoint(identity, masks, stored)


def load_checkpoint(path: str | Path) -> Checkpoint:
    """Load a checkpoint only under the current property proof contract."""
    return _load_checkpoint(path, CONTRACT_SHA256)


def load_overlay_checkpoint(receipt_path: str | Path,
                            checkpoint_path: str | Path) -> Checkpoint:
    """Use an overlay receipt only when its paired immutable bitmap matches."""
    receipt = json.loads(Path(receipt_path).read_text(encoding="utf-8"))
    raw = Path(checkpoint_path).read_bytes()
    if receipt.get("status") != "PASS_SEAL_REPLAY":
        raise ValueError("Canonical overlay receipt is not accepted")
    if receipt.get("checkpoint_sha256") != hashlib.sha256(raw).hexdigest():
        raise ValueError("Overlay receipt does not identify this checkpoint")
    proof_contract = receipt.get("proof_contract_sha256")
    if not isinstance(proof_contract, str) or len(proof_contract) != 64:
        raise ValueError("Overlay receipt has no valid proof contract identity")
    checkpoint = _load_checkpoint(checkpoint_path, proof_contract)
    expected = {
        "rom_sha256": checkpoint.identity["rom_sha256"],
        "rom_size": checkpoint.rom_size,
        "classifier_schema": checkpoint.identity["schema_id"],
        "proof_contract_sha256": checkpoint.identity["contract_sha256"],
        "runtime_build_id": checkpoint.identity["core_build_id"],
        "run_id": checkpoint.identity["run_id"],
        "capabilities": checkpoint.identity["capabilities"],
        "validation_state": checkpoint.identity["validation_state"],
    }
    if any(type(receipt.get(key)) is not type(value) or receipt.get(key) != value
           for key, value in expected.items()):
        raise ValueError("Overlay receipt and checkpoint identities disagree")
    return checkpoint


def read_mask(bitmap: bytes, offset: int) -> int:
    return struct.unpack_from("<H", bitmap, offset * 2)[0]


def coverage(bitmap: bytes) -> tuple[int, int, float]:
    """Return covered-byte union, total bytes, and percentage (mask != 0)."""
    if len(bitmap) % 2:
        raise ValueError("Malformed bitmap")
    total = len(bitmap) // 2
    covered = sum(value != 0 for (value,) in struct.iter_unpack("<H", bitmap))
    return covered, total, 100.0 * covered / total if total else 0.0


def classification_coverage(bitmap: bytes) -> tuple[int, int, int]:
    """Return classified bytes, observation-only bytes, and total bytes."""
    if len(bitmap) % 2:
        raise ValueError("Malformed bitmap")
    classified = observed_only = 0
    for (mask,) in struct.iter_unpack("<H", bitmap):
        if mask & CLASSIFIED_MASK:
            classified += 1
        elif mask & OBSERVATION_ONLY_MASK:
            observed_only += 1
    return classified, observed_only, len(bitmap) // 2


def cell_range(rom_size: int, index: int, cell_bytes: int) -> tuple[int, int]:
    if rom_size <= 0 or index < 0 or cell_bytes <= 0:
        raise ValueError("Invalid cell mapping input")
    start = index * cell_bytes
    if start >= rom_size:
        raise IndexError(index)
    return start, min(start + cell_bytes, rom_size)


def summarize_cell(bitmap: bytes, start: int, end: int) -> dict[str, object]:
    if start < 0 or end <= start or end > len(bitmap) // 2:
        raise ValueError("Cell range outside ROM")
    counts = {name: 0 for _, name in PROPERTIES}
    covered = unknown = 0
    class_bits = 0
    observation_bits = 0
    for offset in range(start, end):
        mask = read_mask(bitmap, offset)
        if not mask:
            unknown += 1
            continue
        covered += 1
        class_bits |= mask & CLASSIFIED_MASK
        observation_bits |= mask & OBSERVATION_ONLY_MASK
        for bit, name in PROPERTIES:
            if mask & bit:
                counts[name] += 1
    bits = [(bit, name) for bit, name in PROPERTIES if class_bits & bit]
    if not covered:
        state = "UNKNOWN"
    elif not bits:
        state = "OBSERVED_UNCLASSIFIED"
    else:
        state = bits[0][1] if len(bits) == 1 else "MIXED"
    if not covered:
        visual_state = "UNKNOWN"
    elif not class_bits and observation_bits in {(1 << 2), (1 << 3)}:
        visual_state = next(name for bit, name in PROPERTIES if bit == observation_bits)
    elif not class_bits:
        visual_state = "MIXED" if observation_bits else "OBSERVED_UNCLASSIFIED"
    else:
        visual_state = state
    return {"start": start, "end": end, "total": end - start, "covered": covered,
            "unknown": unknown, "percent": covered * 100.0 / (end - start),
            "classified": sum(bool(value & CLASSIFIED_MASK)
                               for (value,) in struct.iter_unpack(
                                   "<H", bitmap[start * 2:end * 2])),
            "observed_unclassified": sum(bool(value and not value & CLASSIFIED_MASK)
                                          for (value,) in struct.iter_unpack(
                                              "<H", bitmap[start * 2:end * 2])),
            "counts": counts, "state": state, "visual_state": visual_state}


def newly_covered(current: bytes, canonical: bytes) -> int:
    if len(current) != len(canonical):
        raise ValueError("Current and canonical maps have different ROM sizes")
    return sum(a != 0 and b == 0 for (a,), (b,) in zip(
        struct.iter_unpack("<H", current), struct.iter_unpack("<H", canonical)))


def newly_classified(current: bytes, canonical: bytes) -> int:
    """Count current class bytes absent from the accepted class union."""
    if len(current) != len(canonical) or len(current) % 2:
        raise ValueError("Current and canonical maps have different ROM sizes")
    return sum(bool(a & CLASSIFIED_MASK) and not bool(b & CLASSIFIED_MASK)
               for (a,), (b,) in zip(struct.iter_unpack("<H", current),
                                     struct.iter_unpack("<H", canonical)))


def newly_covered_cells(current: bytes, canonical: bytes,
                        cell_bytes: int = 1024) -> int:
    if len(current) != len(canonical) or len(current) % 2 or cell_bytes <= 0:
        raise ValueError("Invalid maps or cell size")
    cells = 0
    for start in range(0, len(current) // 2, cell_bytes):
        end = min(start + cell_bytes, len(current) // 2)
        if any(a != 0 and b == 0 for (a,), (b,) in zip(
                struct.iter_unpack("<H", current[start * 2:end * 2]),
                struct.iter_unpack("<H", canonical[start * 2:end * 2]))):
            cells += 1
    return cells


def matches_filter(summary: dict[str, object], filter_name: str) -> bool:
    state = str(summary["state"])
    if filter_name == "All properties":
        return True
    if filter_name == "Unknown only":
        return state == "UNKNOWN"
    if filter_name == "Mixed only":
        return state == "MIXED" or summary["visual_state"] == "MIXED"
    if filter_name == "Classified only":
        return bool(summary["classified"])
    if filter_name == "Observed only":
        return state == "OBSERVED_UNCLASSIFIED"
    names = {
        "M68K": {"M68K_EXECUTED_ENCODING", "M68K_DATA_READ"},
        "Z80": {"Z80_DATA_READ", "Z80_EXECUTED_ENCODING"},
        "VDP": {"VDP_VRAM_SOURCE", "VDP_CRAM_SOURCE", "VDP_VSRAM_SOURCE"},
        "Audio": {"AUDIO_PAYLOAD_PROVEN"},
        "Graphics": {"COMPRESSED_GRAPHICS_SOURCE"},
    }.get(filter_name, set())
    return any(summary["counts"][name] for name in names)
