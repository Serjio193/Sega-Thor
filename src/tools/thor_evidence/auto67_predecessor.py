"""Bounded AUTO67 register-predecessor evidence and fail-closed resolver."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
from typing import Any


MAGIC = b"O67P"
VERSION = 1
HEADER_BYTES = 56
RECORD_BYTES = 28
REGISTER_MASKS = {"A4": 1, "A5": 2}


class PredecessorFormatError(ValueError):
    """Focused predecessor evidence is absent, stale, or malformed."""


@dataclass(frozen=True)
class PredecessorRecord:
    epoch: int
    sequence: int
    frame: int
    pc: int
    opcode: int
    registers: dict[str, int]


@dataclass(frozen=True)
class PredecessorCapture:
    path: str
    epoch: int
    target_pc: int
    requested_registers: tuple[str, ...]
    first_sequence: int
    last_sequence: int
    complete: bool
    truncated: bool
    gap: bool
    consumer_sequence: int | None
    consumer_frame: int | None
    overwrites: int
    records: tuple[PredecessorRecord, ...]


def _read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def decode(path: Path) -> PredecessorCapture:
    path = Path(path).resolve()
    data = path.read_bytes()
    if len(data) < HEADER_BYTES or data[:4] != MAGIC:
        raise PredecessorFormatError("unknown predecessor evidence format")
    (version, epoch, target_pc, register_mask, first_sequence, last_sequence,
     record_count, complete, truncated, gap, consumer_sequence,
     consumer_frame, overwrites) = struct.unpack_from("<13I", data, 4)
    if version != VERSION:
        raise PredecessorFormatError(f"unsupported predecessor version {version}")
    if record_count > 4096:
        raise PredecessorFormatError("predecessor record count exceeds bound")
    expected = HEADER_BYTES + record_count * RECORD_BYTES
    if len(data) != expected:
        raise PredecessorFormatError(
            f"predecessor length mismatch: expected {expected}, got {len(data)}")
    requested = tuple(name for name, mask in REGISTER_MASKS.items()
                      if register_mask & mask)
    records = []
    for offset in range(HEADER_BYTES, expected, RECORD_BYTES):
        values = struct.unpack_from("<7I", data, offset)
        registers = {}
        if "A4" in requested:
            registers["A4"] = values[5]
        if "A5" in requested:
            registers["A5"] = values[6]
        records.append(PredecessorRecord(*values[:5], registers))
    if consumer_sequence == 0xFFFFFFFF:
        consumer_sequence = None
    if consumer_frame == 0xFFFFFFFF:
        consumer_frame = None
    return PredecessorCapture(
        str(path), epoch, target_pc, requested, first_sequence, last_sequence,
        bool(complete), bool(truncated), bool(gap), consumer_sequence,
        consumer_frame, overwrites, tuple(records))


def _signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def _operand(rom: bytes, cursor: int, mode: int, register: int,
             width: int) -> tuple[dict[str, Any], int] | None:
    if mode == 0:
        return {"kind": "DATA_REGISTER", "register": f"D{register}",
                "text": f"D{register}"}, cursor
    if mode == 1:
        return {"kind": "ADDRESS_REGISTER", "register": f"A{register}",
                "text": f"A{register}"}, cursor
    if mode in {2, 3, 4}:
        suffix = {2: "", 3: "+", 4: "-"}[mode]
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "mode": mode, "text": f"(A{register}){suffix}"}, cursor
    if mode == 5:
        if cursor + 2 > len(rom):
            return None
        displacement = _signed16(int.from_bytes(rom[cursor:cursor + 2], "big"))
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "mode": mode, "displacement": displacement,
                "text": f"{displacement}(A{register})"}, cursor + 2
    if mode == 7 and register == 1:
        if cursor + 4 > len(rom):
            return None
        address = int.from_bytes(rom[cursor:cursor + 4], "big")
        return {"kind": "ABSOLUTE_MEMORY", "address": address,
                "text": f"${address:08X}.L"}, cursor + 4
    if mode == 7 and register == 4:
        size = 4 if width == 4 else 2
        if cursor + size > len(rom):
            return None
        value = int.from_bytes(rom[cursor:cursor + size], "big")
        return {"kind": "IMMEDIATE", "value": value,
                "text": f"#${value:X}"}, cursor + size
    return None


def register_writes(rom: bytes, pc: int, opcode: int) -> dict[str, Any]:
    """Decode only register-definition semantics; unknown remains unknown."""
    top = opcode >> 12
    if top in {1, 2, 3}:
        width = {1: 1, 2: 4, 3: 2}[top]
        source_mode, source_reg = (opcode >> 3) & 7, opcode & 7
        dest_mode, dest_reg = (opcode >> 6) & 7, (opcode >> 9) & 7
        source = _operand(rom, pc + 2, source_mode, source_reg, width)
        if source is None:
            return {"status": "UNKNOWN", "reason": "truncated MOVE source"}
        writes = []
        if source_mode in {3, 4}:
            writes.append({"register": f"A{source_reg}",
                           "semantics": f"MOVE source auto-update {source[0]['text']}"})
        if dest_mode == 1:
            name = "MOVEA" if top in {2, 3} else "MOVE"
            writes.append({"register": f"A{dest_reg}",
                           "semantics": f"{name}.{('L' if width == 4 else 'W')} "
                                         f"{source[0]['text']},A{dest_reg}"})
        elif dest_mode in {3, 4}:
            writes.append({"register": f"A{dest_reg}",
                           "semantics": f"MOVE destination auto-update (A{dest_reg})"})
        return {"status": "PROVEN", "mnemonic": f"MOVE.{('L' if width == 4 else 'W')}",
                "writes": writes}
    if (opcode & 0xF1C0) == 0x41C0:
        dest_reg = (opcode >> 9) & 7
        mode, source_reg = (opcode >> 3) & 7, opcode & 7
        source = _operand(rom, pc + 2, mode, source_reg, 4)
        if source is None:
            return {"status": "UNKNOWN", "reason": "truncated LEA source"}
        return {"status": "PROVEN", "mnemonic": "LEA",
                "writes": [{"register": f"A{dest_reg}",
                             "semantics": f"LEA {source[0]['text']},A{dest_reg}"}]}
    if top == 5:
        mode, dest_reg = (opcode >> 3) & 7, opcode & 7
        if mode == 1:
            operation = "SUBQ" if opcode & 0x0100 else "ADDQ"
            return {"status": "PROVEN", "mnemonic": operation,
                    "writes": [{"register": f"A{dest_reg}",
                                 "semantics": f"{operation} address-register definition"}]}
        if mode in {0, 2, 3, 4, 5, 6, 7}:
            return {"status": "PROVEN", "mnemonic": "SCC_OR_QUICK",
                    "writes": ([{"register": f"A{dest_reg}",
                                   "semantics": "address auto-update"}]
                                  if mode in {3, 4} else [])}
    if top == 6 or opcode in {0x4E71, 0x4E75, 0x4E73}:
        return {"status": "PROVEN", "mnemonic": "CONTROL_OR_NOP", "writes": []}
    return {"status": "UNKNOWN", "reason": f"unsupported opcode 0x{opcode:04X}"}


def _contiguous(records: list[PredecessorRecord], start: int, end: int) -> bool:
    interval = [item for item in records if start <= item.sequence <= end]
    return bool(interval) and interval[0].sequence == start and interval[-1].sequence == end \
        and all(left.sequence + 1 == right.sequence
                for left, right in zip(interval, interval[1:]))


def resolve(capture: PredecessorCapture, rom: bytes,
            requested: list[str]) -> dict[str, Any]:
    records = list(capture.records)
    if (not capture.complete or capture.truncated or capture.gap or
            capture.overwrites):
        reason = "PREDECESSOR_RING_OVERWRITE" if capture.overwrites else \
            "INCOMPLETE_PREDECESSOR_CAPTURE"
        return {"steps": [], "unresolved": requested,
                "reason": reason}
    if capture.consumer_sequence is None:
        return {"steps": [], "unresolved": requested,
                "reason": "CONSUMER_OCCURRENCE_MISSING"}
    consumer = next((item for item in records
                     if item.sequence == capture.consumer_sequence and
                     item.pc == capture.target_pc), None)
    if consumer is None or consumer.epoch != capture.epoch:
        return {"steps": [], "unresolved": requested,
                "reason": "CONSUMER_OCCURRENCE_IDENTITY_MISMATCH"}
    steps = []
    unresolved = []
    for register in requested:
        if register not in REGISTER_MASKS or register not in consumer.registers:
            unresolved.append(register)
            continue
        candidate = None
        failed_reason = None
        prior = [item for item in records if item.epoch == consumer.epoch and
                 item.sequence < consumer.sequence]
        for item in reversed(prior):
            decoded = register_writes(rom, item.pc, item.opcode)
            if decoded["status"] != "PROVEN":
                failed_reason = decoded["reason"]
                break
            if register in {write["register"] for write in decoded["writes"]}:
                candidate = (item, decoded)
                break
        if failed_reason or candidate is None:
            unresolved.append(register)
            continue
        producer, decoded = candidate
        if not _contiguous(records, producer.sequence, consumer.sequence):
            unresolved.append(register)
            continue
        steps.append({
            "kind": "REGISTER_REACHING_DEFINITION",
            "register": register,
            "producer_pc": f"0x{producer.pc:06X}",
            "consumer_pc": f"0x{consumer.pc:06X}",
            "producer_occurrence": {"epoch": producer.epoch,
                                     "sequence": producer.sequence,
                                     "frame": producer.frame,
                                     "pc": f"0x{producer.pc:06X}"},
            "consumer_occurrence": {"epoch": consumer.epoch,
                                     "sequence": consumer.sequence,
                                     "frame": consumer.frame,
                                     "pc": f"0x{consumer.pc:06X}"},
            "producer_opcode": f"0x{producer.opcode:04X}",
            "producer_semantics": [write["semantics"] for write in decoded["writes"]
                                   if write["register"] == register],
            "evidence": {"complete_interval": True,
                          "sequence_range": [producer.sequence, consumer.sequence],
                          "intervening_register_write": False,
                          "consumer_register_value": consumer.registers[register],
                          "post_state_available": False,
                          "value_consistency": "CORROBORATION_ONLY"}
        })
    return {"steps": steps, "unresolved": unresolved,
            "reason": None if not unresolved else "REGISTER_PROVENANCE_PARTIAL"}
