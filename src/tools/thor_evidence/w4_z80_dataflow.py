"""W4 Z80 Dataflow Provenance Tracker.

Tracks exact provenance of bytes across Z80 registers, memory reads/writes,
stack operations, and register pairs through executed Z80 instructions.
Terminates provenance tracking conservatively on unsupported opcodes.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Iterator, Sequence

from .w3_z80_evidence import (
    CPU_Z80,
    DOMAIN_BANKED_ROM,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    W3Record,
)


class OriginType(str, Enum):
    M68K_HANDOFF = "M68K_HANDOFF"
    BANKED_ROM_READ = "BANKED_ROM_READ"
    IMMEDIATE = "IMMEDIATE"
    UNKNOWN = "UNKNOWN"


@dataclass(frozen=True)
class ProvenanceTag:
    origin_type: OriginType
    origin_stream_seq: int
    origin_master_time: int
    origin_address: int
    origin_value: int
    transform_path: tuple[str, ...] = ()
    dependencies: tuple[ProvenanceTag, ...] = ()

    def add_step(self, step: str) -> ProvenanceTag:
        return ProvenanceTag(
            origin_type=self.origin_type,
            origin_stream_seq=self.origin_stream_seq,
            origin_master_time=self.origin_master_time,
            origin_address=self.origin_address,
            origin_value=self.origin_value,
            transform_path=(*self.transform_path, step),
            dependencies=self.dependencies,
        )

    def all_roots(self) -> tuple[tuple[OriginType, int, int, int, int], ...]:
        seen = {(self.origin_type, self.origin_stream_seq, self.origin_master_time, self.origin_address, self.origin_value)}
        for dep in self.dependencies:
            seen.update(dep.all_roots())
        return tuple(sorted(seen, key=lambda r: (r[0].value, r[1], r[3])))

    def has_origin_type(self, origin_type: OriginType) -> bool:
        return any(r[0] == origin_type for r in self.all_roots())

    def to_dict(self) -> dict[str, object]:
        roots = self.all_roots()
        return {
            "origin_type": self.origin_type.value,
            "origin_stream_seq": self.origin_stream_seq,
            "origin_master_time": self.origin_master_time,
            "origin_address": f"0x{self.origin_address:06X}",
            "origin_value": f"0x{self.origin_value:02X}",
            "transform_path": list(self.transform_path),
            "is_multi_root": len(roots) > 1,
            "dependency_roots": [
                {
                    "origin_type": r[0].value,
                    "origin_stream_seq": r[1],
                    "origin_master_time": r[2],
                    "origin_address": f"0x{r[3]:06X}",
                    "origin_value": f"0x{r[4]:02X}",
                }
                for r in roots
            ],
        }


def merge_binary_provenance(
    op_name: str,
    pc: int,
    prov_a: ProvenanceTag | None,
    prov_b: ProvenanceTag | None,
) -> ProvenanceTag | None:
    """Merges provenance of two operands in a binary ALU/memory operation."""
    if prov_a is None and prov_b is None:
        return None
    if prov_a is None:
        assert prov_b is not None
        return prov_b.add_step(f"{op_name} [0x{pc:04X}]")
    if prov_b is None:
        return prov_a.add_step(f"{op_name} [0x{pc:04X}]")

    # Both operands have provenance. Prioritize BANKED_ROM_READ as primary trace root.
    if prov_a.origin_type == OriginType.BANKED_ROM_READ and prov_b.origin_type != OriginType.BANKED_ROM_READ:
        primary = prov_a
        secondary = prov_b
    elif prov_b.origin_type == OriginType.BANKED_ROM_READ and prov_a.origin_type != OriginType.BANKED_ROM_READ:
        primary = prov_b
        secondary = prov_a
    elif prov_a.origin_stream_seq <= prov_b.origin_stream_seq:
        primary = prov_a
        secondary = prov_b
    else:
        primary = prov_b
        secondary = prov_a

    deps = list(primary.dependencies)
    seen = {(d.origin_type, d.origin_stream_seq, d.origin_address) for d in deps}
    for cand in (secondary, *secondary.dependencies):
        key = (cand.origin_type, cand.origin_stream_seq, cand.origin_address)
        if key not in seen:
            seen.add(key)
            deps.append(cand)

    return ProvenanceTag(
        origin_type=primary.origin_type,
        origin_stream_seq=primary.origin_stream_seq,
        origin_master_time=primary.origin_master_time,
        origin_address=primary.origin_address,
        origin_value=primary.origin_value,
        transform_path=(*primary.transform_path, f"{op_name} [0x{pc:04X}]"),
        dependencies=tuple(deps),
    )


class Z80DataflowState:
    """Exact Z80 register, stack, and RAM provenance state."""

    def __init__(self) -> None:
        # Main registers: (value, ProvenanceTag | None)
        self.regs: dict[str, tuple[int, ProvenanceTag | None]] = {
            "A": (0, None),
            "B": (0, None),
            "C": (0, None),
            "D": (0, None),
            "E": (0, None),
            "H": (0, None),
            "L": (0, None),
            "IX_H": (0, None),
            "IX_L": (0, None),
            "IY_H": (0, None),
            "IY_L": (0, None),
        }
        # Shadow registers for EXX / EX AF, AF'
        self.shadow_regs: dict[str, tuple[int, ProvenanceTag | None]] = {
            "A": (0, None),
            "B": (0, None),
            "C": (0, None),
            "D": (0, None),
            "E": (0, None),
            "H": (0, None),
            "L": (0, None),
        }
        # Stack provenance: list of ProvenanceTag | None
        self.stack: list[ProvenanceTag | None] = []
        # RAM byte provenance: address (0..0x1FFF) -> ProvenanceTag | None
        self.ram_prov: dict[int, ProvenanceTag | None] = {}
        # Last read memory event to associate with instruction
        self.last_read_prov: ProvenanceTag | None = None
        self.last_read_addr: int | None = None
        self.last_read_val: int | None = None

    def get_reg(self, name: str) -> tuple[int, ProvenanceTag | None]:
        return self.regs.get(name, (0, None))

    def set_reg(self, name: str, val: int, prov: ProvenanceTag | None) -> None:
        self.regs[name] = (val & 0xFF, prov)

    def set_ram(self, addr: int, prov: ProvenanceTag | None) -> None:
        self.ram_prov[addr & 0x1FFF] = prov

    def get_ram(self, addr: int) -> ProvenanceTag | None:
        return self.ram_prov.get(addr & 0x1FFF, None)

    def exx(self) -> None:
        for r in ("B", "C", "D", "E", "H", "L"):
            self.regs[r], self.shadow_regs[r] = self.shadow_regs[r], self.regs[r]

    def ex_af(self) -> None:
        self.regs["A"], self.shadow_regs["A"] = self.shadow_regs["A"], self.regs["A"]

    def push_pair(self, high: str, low: str) -> None:
        self.stack.append(self.regs[high][1])
        self.stack.append(self.regs[low][1])

    def pop_pair(self, high: str, low: str) -> None:
        low_prov = self.stack.pop() if self.stack else None
        high_prov = self.stack.pop() if self.stack else None
        self.regs[low] = (self.regs[low][0], low_prov)
        self.regs[high] = (self.regs[high][0], high_prov)


class Z80DataflowTracker:
    """Executes Z80 instruction provenance stepping."""

    def __init__(self) -> None:
        self.state = Z80DataflowState()
        self.supported_opcodes: int = 0
        self.unsupported_opcodes: int = 0

    def register_m68k_write(self, z80_offset: int, val: int, record: W3Record) -> None:
        """Called when M68K writes to Z80 window ($A00000..$A01FFF)."""
        prov = ProvenanceTag(
            origin_type=OriginType.M68K_HANDOFF,
            origin_stream_seq=record.stream_sequence,
            origin_master_time=record.master_time,
            origin_address=record.address,
            origin_value=val & 0xFF,
            transform_path=(f"M68K_WRITE(0x{record.address:06X}=0x{val:02X})",),
        )
        self.state.set_ram(z80_offset, prov)

    def process_bus_read(self, record: W3Record) -> None:
        """Called when Z80 performs a bus read."""
        addr = record.address & 0xFFFF
        val = record.value & 0xFF

        if record.domain == DOMAIN_BANKED_ROM or addr >= 0x8000:
            # Physical address in auxiliary or resolved base
            phys_addr = record.auxiliary if record.auxiliary != 0 else addr
            prov = ProvenanceTag(
                origin_type=OriginType.BANKED_ROM_READ,
                origin_stream_seq=record.stream_sequence,
                origin_master_time=record.master_time,
                origin_address=phys_addr,
                origin_value=val,
                transform_path=(f"BANKED_ROM_READ(0x{phys_addr:06X}=0x{val:02X})",),
            )
            self.state.last_read_prov = prov
        elif record.domain == DOMAIN_Z80_RAM or addr < 0x2000:
            self.state.last_read_prov = self.state.get_ram(addr)
        else:
            self.state.last_read_prov = None

        self.state.last_read_addr = addr
        self.state.last_read_val = val

    def process_instruction(self, record: W3Record) -> ProvenanceTag | None:
        """Steps a single Z80 instruction and tracks register/memory transfers.

        Returns provenance tag if this instruction is an audio write (e.g. to YM2612).
        """
        raw = record.z80_instruction_bytes
        if not raw:
            return None

        op0 = raw[0]

        # EXX (0xD9)
        if op0 == 0xD9:
            self.state.exx()
            self.supported_opcodes += 1
            return None

        # EX AF, AF' (0x08)
        if op0 == 0x08:
            self.state.ex_af()
            self.supported_opcodes += 1
            return None

        # LD A, (HL) (0x7E)
        if op0 == 0x7E:
            prov = self.state.last_read_prov
            if prov is not None:
                prov = prov.add_step(f"LD A, (HL) [0x{record.pc:04X}]")
            val = self.state.last_read_val if self.state.last_read_val is not None else 0
            self.state.set_reg("A", val, prov)
            self.supported_opcodes += 1
            return None

        # LD A, (nn) (0x3A)
        if op0 == 0x3A and len(raw) >= 3:
            addr = raw[1] | (raw[2] << 8)
            op_prov = self.state.get_ram(record.pc + 1)
            ram_prov = self.state.get_ram(addr)
            read_prov = self.state.last_read_prov
            mem_prov = ram_prov or read_prov
            prov = merge_binary_provenance(f"LD A, (0x{addr:04X})", record.pc, op_prov, mem_prov)
            val = self.state.last_read_val if self.state.last_read_val is not None else 0
            self.state.set_reg("A", val, prov)
            self.supported_opcodes += 1
            return None

        # AND n (0xE6)
        if op0 == 0xE6 and len(raw) >= 2:
            n = raw[1]
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"AND A, 0x{n:02X} [0x{record.pc:04X}]")
            self.state.set_reg("A", (val & n) & 0xFF, prov)
            self.supported_opcodes += 1
            return None

        # ADD A, n (0xC6)
        if op0 == 0xC6 and len(raw) >= 2:
            n = raw[1]
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"ADD A, 0x{n:02X} [0x{record.pc:04X}]")
            self.state.set_reg("A", (val + n) & 0xFF, prov)
            self.supported_opcodes += 1
            return None

        # ADD A, r (0x80..0x85)
        if 0x80 <= op0 <= 0x85:
            src_name = ("B", "C", "D", "E", "H", "L")[op0 - 0x80]
            val_a, prov_a = self.state.get_reg("A")
            val_s, prov_s = self.state.get_reg(src_name)
            prov = merge_binary_provenance(f"ADD A, {src_name}", record.pc, prov_a, prov_s)
            self.state.set_reg("A", (val_a + val_s) & 0xFF, prov)
            self.supported_opcodes += 1
            return None

        # RRCA (0x0F)
        if op0 == 0x0F:
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"RRCA [0x{record.pc:04X}]")
            rotated = ((val >> 1) | ((val & 1) << 7)) & 0xFF
            self.state.set_reg("A", rotated, prov)
            self.supported_opcodes += 1
            return None

        # RLCA (0x07)
        if op0 == 0x07:
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"RLCA [0x{record.pc:04X}]")
            rotated = ((val << 1) | (val >> 7)) & 0xFF
            self.state.set_reg("A", rotated, prov)
            self.supported_opcodes += 1
            return None

        # LD (nn), A (0x32)
        if op0 == 0x32 and len(raw) >= 3:
            addr = raw[1] | (raw[2] << 8)
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"LD (0x{addr:04X}), A [0x{record.pc:04X}]")
            self.state.set_ram(addr, prov)
            self.supported_opcodes += 1
            return None

        # LD A, r (0x78..0x7D)
        if 0x78 <= op0 <= 0x7D:
            src_name = ("B", "C", "D", "E", "H", "L")[op0 - 0x78]
            val, prov = self.state.get_reg(src_name)
            if prov is not None:
                prov = prov.add_step(f"LD A, {src_name} [0x{record.pc:04X}]")
            self.state.set_reg("A", val, prov)
            self.supported_opcodes += 1
            return None

        # LD r, A (0x47, 0x4F, 0x57, 0x5F, 0x67, 0x6F)
        if op0 in (0x47, 0x4F, 0x57, 0x5F, 0x67, 0x6F):
            dst_map = {0x47: "B", 0x4F: "C", 0x57: "D", 0x5F: "E", 0x67: "H", 0x6F: "L"}
            dst_name = dst_map[op0]
            val, prov = self.state.get_reg("A")
            if prov is not None:
                prov = prov.add_step(f"LD {dst_name}, A [0x{record.pc:04X}]")
            self.state.set_reg(dst_name, val, prov)
            self.supported_opcodes += 1
            return None

        # PUSH / POP
        if op0 in (0xC5, 0xD5, 0xE5, 0xF5):  # PUSH BC, DE, HL, AF
            pairs = {0xC5: ("B", "C"), 0xD5: ("D", "E"), 0xE5: ("H", "L"), 0xF5: ("A", "A")}
            h, l = pairs[op0]
            self.state.push_pair(h, l)
            self.supported_opcodes += 1
            return None

        if op0 in (0xC1, 0xD1, 0xE1, 0xF1):  # POP BC, DE, HL, AF
            pairs = {0xC1: ("B", "C"), 0xD1: ("D", "E"), 0xE1: ("H", "L"), 0xF1: ("A", "A")}
            h, l = pairs[op0]
            self.state.pop_pair(h, l)
            self.supported_opcodes += 1
            return None

        # Prefixed IX instructions (0xDD)
        if op0 == 0xDD and len(raw) >= 2:
            op1 = raw[1]
            # LD (IX+d), A (0xDD 0x77 d)
            if op1 == 0x77:
                val, prov = self.state.get_reg("A")
                if prov is not None:
                    prov = prov.add_step(f"LD (IX+d), A [0x{record.pc:04X}]")
                self.supported_opcodes += 1
                return prov

            # LD (IX+d), r
            if 0x70 <= op1 <= 0x75:
                src_name = ("B", "C", "D", "E", "H", "L")[op1 - 0x70]
                val, prov = self.state.get_reg(src_name)
                if prov is not None:
                    prov = prov.add_step(f"LD (IX+d), {src_name} [0x{record.pc:04X}]")
                self.supported_opcodes += 1
                return prov

        # Prefixed IY instructions (0xFD)
        if op0 == 0xFD and len(raw) >= 2:
            op1 = raw[1]
            # LD (IY+d), A (0xFD 0x77 d)
            if op1 == 0x77:
                val, prov = self.state.get_reg("A")
                if prov is not None:
                    prov = prov.add_step(f"LD (IY+d), A [0x{record.pc:04X}]")
                self.supported_opcodes += 1
                return prov

        # Conservative clearing on opcodes that modify A without tracked provenance
        if op0 in (0xAF,):  # XOR A (clears A to 0)
            self.state.set_reg("A", 0, None)
            self.supported_opcodes += 1
            return None

        if op0 in (0x3E,):  # LD A, n
            n = raw[1] if len(raw) > 1 else 0
            self.state.set_reg("A", n, None)
            self.supported_opcodes += 1
            return None

        # Control flow / comparisons (JR, JP, CALL, RET, CP, BIT, INC, DEC)
        # do not transfer register data values to registers
        if op0 in (
            0x28, 0x20, 0xC9, 0xCD, 0xC3, 0xC2, 0xCA, 0xFE, 0xB7, 0xB0,
            0x23, 0x2B, 0x03, 0x0B, 0x13, 0x1B,
        ):
            self.supported_opcodes += 1
            return None

        # Unsupported instruction modifying state -> conservative termination
        self.unsupported_opcodes += 1
        return None
