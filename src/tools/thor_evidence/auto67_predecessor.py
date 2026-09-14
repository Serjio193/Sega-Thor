"""Bounded AUTO67 register-predecessor evidence and fail-closed resolver."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
from typing import Any


MAGIC = b"O67P"
VERSION = 2
V1_HEADER_BYTES = 56
V2_HEADER_BYTES = 72
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
    format_version: int = 1
    ring_capacity: int = 0
    ring_wrapped: bool = False
    consumer_pc: int | None = None
    join_status: str = "LEGACY"


def _read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def decode(path: Path) -> PredecessorCapture:
    path = Path(path).resolve()
    data = path.read_bytes()
    if len(data) < V1_HEADER_BYTES or data[:4] != MAGIC:
        raise PredecessorFormatError("unknown predecessor evidence format")
    version = _read_u32(data, 4)
    if version == 1:
        header_bytes = V1_HEADER_BYTES
        (version, epoch, target_pc, register_mask, first_sequence, last_sequence,
         record_count, complete, truncated, gap, consumer_sequence,
         consumer_frame, overwrites) = struct.unpack_from("<13I", data, 4)
        ring_capacity, ring_wrapped, consumer_pc, join_status = 0, 0, target_pc, "LEGACY"
    elif version == VERSION:
        header_bytes = V2_HEADER_BYTES
        (version, epoch, target_pc, register_mask, first_sequence, last_sequence,
         record_count, complete, truncated, gap, consumer_sequence,
         consumer_frame, overwrites, ring_capacity, ring_wrapped, consumer_pc,
         join_code) = struct.unpack_from("<17I", data, 4)
        join_status = "EXACT" if join_code == 1 else "MISSING"
    else:
        raise PredecessorFormatError(f"unsupported predecessor version {version}")
    if record_count > 4096:
        raise PredecessorFormatError("predecessor record count exceeds bound")
    expected = header_bytes + record_count * RECORD_BYTES
    if len(data) != expected:
        raise PredecessorFormatError(
            f"predecessor length mismatch: expected {expected}, got {len(data)}")
    requested = tuple(name for name, mask in REGISTER_MASKS.items()
                      if register_mask & mask)
    records = []
    for offset in range(header_bytes, expected, RECORD_BYTES):
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
    if consumer_pc == 0xFFFFFFFF:
        consumer_pc = None
    return PredecessorCapture(
        str(path), epoch, target_pc, requested, first_sequence, last_sequence,
        bool(complete), bool(truncated), bool(gap), consumer_sequence,
        consumer_frame, overwrites, tuple(records), version, ring_capacity,
        bool(ring_wrapped), consumer_pc, join_status)


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
    if mode == 6:
        if cursor + 2 > len(rom):
            return None
        extension = int.from_bytes(rom[cursor:cursor + 2], "big")
        index_kind = "A" if extension & 0x8000 else "D"
        index = (extension >> 12) & 7
        displacement = extension & 0xFF
        if displacement & 0x80:
            displacement -= 0x100
        return {"kind": "MEMORY_REGISTER_INDEXED", "register": f"A{register}",
                "mode": mode, "index_register": f"{index_kind}{index}",
                "displacement": displacement,
                "text": f"{displacement}(A{register},{index_kind}{index})"}, cursor + 2
    if mode == 7 and register == 1:
        if cursor + 4 > len(rom):
            return None
        address = int.from_bytes(rom[cursor:cursor + 4], "big")
        return {"kind": "ABSOLUTE_MEMORY", "address": address,
                "text": f"${address:08X}.L"}, cursor + 4
    if mode == 7 and register in {2, 3}:
        if cursor + 2 > len(rom):
            return None
        extension = int.from_bytes(rom[cursor:cursor + 2], "big")
        displacement = _signed16(extension) if register == 2 else extension & 0xFF
        if register == 3 and displacement & 0x80:
            displacement -= 0x100
        text = f"{displacement}(PC)" if register == 2 else f"d8(PC)"
        return {"kind": "PC_MEMORY", "register": "PC",
                "displacement": displacement, "text": text}, cursor + 2
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
    if (opcode & 0xFFC0) == 0x4840:
        return {"status": "PROVEN", "mnemonic": "PEA", "writes": [
            {"register": "A7", "semantics": "PEA stack update"}]}
    if (opcode & 0xF1C0) == 0x40C0:
        mode, dest_reg = (opcode >> 3) & 7, opcode & 7
        writes = ([{"register": f"A{dest_reg}",
                    "semantics": "MOVE status-register destination update"}]
                  if mode in {3, 4} else [])
        return {"status": "PROVEN", "mnemonic": "MOVE_STATUS", "writes": writes}
    if (opcode >> 12) == 0 and (opcode & 0x0100) == 0:
        mode, dest_reg = (opcode >> 3) & 7, opcode & 7
        writes = ([{"register": f"A{dest_reg}",
                    "semantics": "bit-operation address auto-update"}]
                  if mode in {3, 4} else [])
        return {"status": "PROVEN", "mnemonic": "BIT_IMMEDIATE", "writes": writes}
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
    if not capture.complete or capture.truncated or capture.gap:
        return {"steps": [], "unresolved": requested,
                "reason": "INCOMPLETE_PREDECESSOR_CAPTURE"}
    if capture.format_version >= 2 and capture.join_status != "EXACT":
        return {"steps": [], "unresolved": requested,
                "reason": "CONSUMER_JOIN_NOT_EXACT"}
    if capture.consumer_sequence is None:
        return {"steps": [], "unresolved": requested,
                "reason": "CONSUMER_OCCURRENCE_MISSING"}
    consumer = next((item for item in records
                     if item.sequence == capture.consumer_sequence and
                     item.pc == (capture.consumer_pc or capture.target_pc)), None)
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
            # BizHawk's bus-exec callback supplies a second bus value, not a
            # reliable instruction opcode.  R2 therefore keeps the runtime
            # PC/sequence as evidence and decodes the opcode from the
            # canonical ROM when that callback value is zero.  A non-zero
            # captured value remains available for legacy/test captures.
            opcode = item.opcode
            opcode_source = "RUNTIME_RECORD"
            if capture.format_version >= 2 and item.pc >= 0 and item.pc + 2 <= len(rom):
                opcode = int.from_bytes(rom[item.pc:item.pc + 2], "big")
                opcode_source = "STATIC_ROM_PC"
            decoded = register_writes(rom, item.pc, opcode)
            if decoded["status"] != "PROVEN":
                failed_reason = decoded["reason"]
                break
            if register in {write["register"] for write in decoded["writes"]}:
                candidate = (item, decoded, opcode, opcode_source)
                break
        if failed_reason or candidate is None:
            unresolved.append(register)
            continue
        producer, decoded, producer_opcode, opcode_source = candidate
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
            "producer_opcode": f"0x{producer_opcode:04X}",
            "producer_opcode_source": opcode_source,
            "producer_semantics": [write["semantics"] for write in decoded["writes"]
                                   if write["register"] == register],
            "instruction_distance": consumer.sequence - producer.sequence,
            "evidence": {"complete_interval": True,
                          "sequence_range": [producer.sequence, consumer.sequence],
                          "ring_capacity": capture.ring_capacity,
                          "ring_wrapped": capture.ring_wrapped,
                          "ring_overwrites_before_snapshot": capture.overwrites,
                          "intervening_register_write": False,
                          "consumer_register_value": consumer.registers[register],
                          "post_state_available": False,
                          "value_consistency": "CORROBORATION_ONLY"}
        })
    return {"steps": steps, "unresolved": unresolved,
            "reason": None if not unresolved else "REGISTER_PROVENANCE_PARTIAL"}
