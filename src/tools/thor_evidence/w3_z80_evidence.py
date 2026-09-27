"""W3 Z80 Evidence Stream and Record Decoder.

Lossless decoding of V2 48-byte native evidence records with explicit
fields for master_time, cpu_id, length_or_width, domain, and auxiliary.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
from typing import BinaryIO, Iterator, Sequence

# V2 Record layout (48 bytes, little-endian):
#   uint64_t stream_sequence
#   uint64_t instruction_sequence
#   uint64_t master_time
#   uint32_t pc
#   uint32_t address
#   uint32_t value
#   uint16_t kind_flags
#   uint8_t  cpu_id
#   uint8_t  length_or_width
#   uint16_t domain
#   uint16_t reserved
#   uint32_t auxiliary
RECORD_FORMAT = "<QQQIIIHBBHHI"
RECORD_STRUCT = struct.Struct(RECORD_FORMAT)
assert RECORD_STRUCT.size == 48

# Flags
FLAG_INSTRUCTION = 0x0001
FLAG_COMPLETE = 0x0002
FLAG_FAULTED = 0x0004
FLAG_CONTROL_FLOW = 0x0008
FLAG_BRANCH_TAKEN = 0x0010
FLAG_BRANCH_NOT_TAKEN = 0x0020
FLAG_EXCEPTION = 0x0040
FLAG_ASYNCHRONOUS = 0x0080
FLAG_EXCEPTION_EVENT = 0x0100
FLAG_FETCHED = 0x0200
FLAG_CPU_STOP_EVENT = 0x0400
FLAG_EVENT = 0x8000

EVENT_SUBTYPE_SHIFT = 11
EVENT_SUBTYPE_MASK = 0x3800

EVENT_NONE = 0
EVENT_BUS_READ = 1
EVENT_BUS_WRITE = 2
EVENT_FRAME_BOUNDARY = 3
EVENT_BANK_REGISTER_CHANGE = 4

# CPU IDs
CPU_68K = 0
CPU_Z80 = 1
CPU_NONE = 0xFF

# Bus Domains
DOMAIN_ROM = 0
DOMAIN_68K_RAM = 1
DOMAIN_Z80_WINDOW = 2
DOMAIN_VDP = 3
DOMAIN_YM2612 = 4
DOMAIN_PSG = 5
DOMAIN_OTHER = 6
DOMAIN_Z80_RAM = 7
DOMAIN_BANKED_ROM = 8

DOMAIN_NAMES = {
    DOMAIN_ROM: "ROM",
    DOMAIN_68K_RAM: "68K_RAM",
    DOMAIN_Z80_WINDOW: "Z80_WINDOW",
    DOMAIN_VDP: "VDP",
    DOMAIN_YM2612: "YM2612",
    DOMAIN_PSG: "PSG",
    DOMAIN_OTHER: "OTHER",
    DOMAIN_Z80_RAM: "Z80_RAM",
    DOMAIN_BANKED_ROM: "BANKED_ROM",
}


@dataclass(frozen=True)
class W3Record:
    stream_sequence: int
    instruction_sequence: int
    master_time: int
    pc: int
    address: int
    value: int
    kind_flags: int
    cpu_id: int
    length_or_width: int
    domain: int
    auxiliary: int

    @property
    def is_instruction(self) -> bool:
        return bool(self.kind_flags & FLAG_INSTRUCTION)

    @property
    def is_event(self) -> bool:
        return bool(self.kind_flags & FLAG_EVENT)

    @property
    def event_subtype(self) -> int:
        if not self.is_event:
            return EVENT_NONE
        return (self.kind_flags & EVENT_SUBTYPE_MASK) >> EVENT_SUBTYPE_SHIFT

    @property
    def is_bus_read(self) -> bool:
        return self.event_subtype == EVENT_BUS_READ

    @property
    def is_bus_write(self) -> bool:
        return self.event_subtype == EVENT_BUS_WRITE

    @property
    def is_frame_boundary(self) -> bool:
        return self.event_subtype == EVENT_FRAME_BOUNDARY

    @property
    def frame_number(self) -> int:
        if not self.is_frame_boundary:
            return 0
        if self.pc != 0 or self.address != 0:
            return (self.pc & 0xFFFFFFFF) | ((self.address & 0xFFFFFFFF) << 32)
        return self.value

    @property
    def is_bank_register_change(self) -> bool:
        return self.event_subtype == EVENT_BANK_REGISTER_CHANGE

    @property
    def domain_name(self) -> str:
        return DOMAIN_NAMES.get(self.domain, f"UNKNOWN_{self.domain}")

    @property
    def z80_instruction_bytes(self) -> bytes:
        if not (self.is_instruction and self.cpu_id == CPU_Z80):
            return b""
        length = min(self.length_or_width, 4)
        raw = self.value.to_bytes(4, byteorder="little")
        return raw[:length]


def unpack_record(data: bytes | bytearray | memoryview, offset: int = 0) -> W3Record:
    fields = RECORD_STRUCT.unpack_from(data, offset)
    return W3Record(
        stream_sequence=fields[0],
        instruction_sequence=fields[1],
        master_time=fields[2],
        pc=fields[3],
        address=fields[4],
        value=fields[5],
        kind_flags=fields[6],
        cpu_id=fields[7],
        length_or_width=fields[8],
        domain=fields[9],
        auxiliary=fields[11],
    )


def pack_record(record: W3Record) -> bytes:
    return RECORD_STRUCT.pack(
        record.stream_sequence,
        record.instruction_sequence,
        record.master_time,
        record.pc,
        record.address,
        record.value,
        record.kind_flags,
        record.cpu_id,
        record.length_or_width,
        record.domain,
        0,  # reserved
        record.auxiliary,
    )


def iter_records(source: Path | BinaryIO | bytes) -> Iterator[W3Record]:
    if isinstance(source, bytes):
        for i in range(0, len(source), RECORD_STRUCT.size):
            if i + RECORD_STRUCT.size <= len(source):
                yield unpack_record(source, i)
    elif isinstance(source, Path):
        with source.open("rb") as f:
            while True:
                chunk = f.read(RECORD_STRUCT.size)
                if len(chunk) < RECORD_STRUCT.size:
                    break
                yield unpack_record(chunk)
    else:
        while True:
            chunk = source.read(RECORD_STRUCT.size)
            if len(chunk) < RECORD_STRUCT.size:
                break
            yield unpack_record(chunk)


class Z80BankTracker:
    """Tracks the Genesis 9-bit Z80 ROM bank register.

    The bank register maps the 32 KB window at Z80 $8000-$FFFF into
    the 24-bit 68K / cartridge address space. Writing a single bit to
    Z80 $6000-$60FF shifts the bank register right by 1 bit and inserts
    the written bit at bit 23.
    """

    def __init__(self, initial_base: int = 0) -> None:
        self.bank_base = initial_base & 0xFF8000
        self.change_history: list[tuple[int, int, int]] = []  # (master_time, bit, base)

    def write_bit(self, bit: int, master_time: int = 0) -> int:
        self.bank_base = ((self.bank_base >> 1) | ((bit & 1) << 23)) & 0xFF8000
        self.change_history.append((master_time, bit & 1, self.bank_base))
        return self.bank_base

    def set_base(self, base: int, master_time: int = 0) -> None:
        self.bank_base = base & 0xFF8000
        self.change_history.append((master_time, -1, self.bank_base))

    def resolve_physical(self, z80_address: int) -> int:
        """Resolves a Z80 logical address (0x8000-0xFFFF) to Genesis physical ROM address."""
        if not (0x8000 <= z80_address <= 0xFFFF):
            raise ValueError(f"Z80 address {z80_address:#06x} is outside the 32K banked window ($8000-$FFFF)")
        return self.bank_base | (z80_address & 0x7FFF)
