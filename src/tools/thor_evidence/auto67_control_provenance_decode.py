"""Exact 68000 effective-address decoding used by bounded control provenance."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class RegisterValue:
    value: int | None = None
    low_value: int | None = None
    known_width: int = 0
    source: dict[str, Any] | None = None
    transforms: list[dict[str, Any]] = field(default_factory=list)
    unsupported: str | None = None
    producers: list[dict[str, Any]] = field(default_factory=list)
    register_chain: list[dict[str, Any]] = field(default_factory=list)

    def copy(self) -> "RegisterValue":
        return RegisterValue(self.value, self.low_value, self.known_width,
                             dict(self.source) if self.source else None,
                             list(self.transforms), self.unsupported,
                             list(self.producers), list(self.register_chain))


def signed8(value: int) -> int:
    return value - 0x100 if value & 0x80 else value


def signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def register_name(kind: int, number: int) -> str:
    return f"{'A' if kind else 'D'}{number}"


def read_ea(rom: bytes, pc: int, mode: int, register: int,
            width: int) -> tuple[dict[str, Any], int] | None:
    cursor = pc + 2
    if mode in (0, 1):
        return {"kind": "REGISTER", "register": register_name(mode, register)}, cursor
    if mode in (2, 3, 4):
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "mode": mode}, cursor
    if mode == 5:
        if cursor + 2 > len(rom):
            return None
        displacement = signed16(int.from_bytes(rom[cursor:cursor + 2], "big"))
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "mode": mode, "displacement": displacement}, cursor + 2
    if mode == 6:
        if cursor + 2 > len(rom):
            return None
        ext = int.from_bytes(rom[cursor:cursor + 2], "big")
        return {"kind": "INDEXED_REGISTER", "register": f"A{register}",
                "index_register": register_name((ext >> 15) & 1, (ext >> 12) & 7),
                "index_long": bool(ext & 0x0800),
                "displacement": signed8(ext & 0xFF), "extension": ext}, cursor + 2
    if mode != 7:
        return None
    if register == 0:
        if cursor + 2 > len(rom):
            return None
        address = signed16(int.from_bytes(rom[cursor:cursor + 2], "big")) & 0xFFFFFF
        return {"kind": "ABSOLUTE_MEMORY", "address": address}, cursor + 2
    if register == 1:
        if cursor + 4 > len(rom):
            return None
        address = int.from_bytes(rom[cursor:cursor + 4], "big") & 0xFFFFFF
        return {"kind": "ABSOLUTE_MEMORY", "address": address}, cursor + 4
    if register == 2:
        if cursor + 2 > len(rom):
            return None
        displacement = signed16(int.from_bytes(rom[cursor:cursor + 2], "big"))
        return {"kind": "PC_MEMORY", "base": pc + 2,
                "displacement": displacement}, cursor + 2
    if register == 3:
        if cursor + 2 > len(rom):
            return None
        ext = int.from_bytes(rom[cursor:cursor + 2], "big")
        return {"kind": "PC_INDEXED", "base": pc + 2,
                "index_register": register_name((ext >> 15) & 1, (ext >> 12) & 7),
                "index_long": bool(ext & 0x0800),
                "displacement": signed8(ext & 0xFF), "extension": ext}, cursor + 2
    if register == 4:
        size = 4 if width == 4 else 2
        if cursor + size > len(rom):
            return None
        return {"kind": "IMMEDIATE", "value": int.from_bytes(
            rom[cursor:cursor + size], "big"), "width": size}, cursor + size
    return None


def effective_address(ea: dict[str, Any], regs: dict[str, RegisterValue]) -> int | None:
    kind = ea["kind"]
    if kind == "ABSOLUTE_MEMORY":
        return int(ea["address"])
    if kind == "MEMORY_REGISTER":
        base = regs.get(ea["register"], RegisterValue()).value
        if base is None:
            return None
        return (base + int(ea.get("displacement", 0))) & 0xFFFFFF
    if kind == "INDEXED_REGISTER":
        base = regs.get(ea["register"], RegisterValue()).value
        index = regs.get(ea["index_register"], RegisterValue()).value
        if base is None or index is None:
            return None
        if not ea["index_long"]:
            index = signed16(index & 0xFFFF)
        return (base + index + int(ea["displacement"])) & 0xFFFFFF
    if kind == "PC_MEMORY":
        return (int(ea["base"]) + int(ea["displacement"])) & 0xFFFFFF
    if kind == "PC_INDEXED":
        index = regs.get(ea["index_register"], RegisterValue()).value
        if index is None:
            return None
        if not ea["index_long"]:
            index = signed16(index & 0xFFFF)
        return (int(ea["base"]) + index + int(ea["displacement"])) & 0xFFFFFF
    return None


def memory_source(ea: dict[str, Any], width: int,
                  regs: dict[str, RegisterValue], rom: bytes) -> RegisterValue:
    address = effective_address(ea, regs)
    if address is None:
        return RegisterValue(unsupported="SOURCE_EFFECTIVE_ADDRESS_UNRESOLVED")
    if width > 1 and address & 1:
        return RegisterValue(unsupported="SOURCE_UNALIGNED_READ")
    if address >= len(rom) or address + width > len(rom):
        reason = "RAM_SOURCE_VALUE_UNRESOLVED" if 0xFF0000 <= address <= 0xFFFFFF \
            else "SOURCE_REGION_NOT_CANONICAL_ROM"
        return RegisterValue(unsupported=reason)
    raw = rom[address:address + width]
    value = int.from_bytes(raw, "big")
    source = {"kind": "ROM_MEMORY_READ", "start": address,
              "end": address + width, "width": width,
              "bytes_hex": raw.hex().upper(), "value": value,
              "cpu_address": address, "effective_address_rule": ea["kind"]}
    if ea["kind"] in {"INDEXED_REGISTER", "PC_INDEXED"}:
        source["selected_index_register"] = ea.get("index_register")
        index_value = regs.get(str(ea.get("index_register")), RegisterValue()).value
        source["selected_index_value"] = index_value
        table_base = ea.get("base", regs.get(
            str(ea.get("register")), RegisterValue()).value)
        source["table_base"] = int(table_base) if table_base is not None else None
        if index_value is not None:
            if not ea.get("index_long", True):
                index_value = signed16(index_value & 0xFFFF)
            entry_offset = index_value + int(ea.get("displacement", 0))
            source["selected_entry_offset"] = entry_offset
            if entry_offset >= 0 and entry_offset % width == 0 and table_base is not None and \
                    int(table_base) + entry_offset == address:
                source["selected_index"] = entry_offset // width
        source["entry_width"] = width
        source["table_entry_candidate"] = True
        source["table_entry"] = "selected_index" in source
    return RegisterValue(value=value if width == 4 else None,
                         low_value=value, known_width=width, source=source)


def indirect_consumer(rom: bytes, pc: int, opcode: int) -> dict[str, Any] | None:
    form = opcode & 0xFFC0
    if form not in (0x4E80, 0x4EC0):
        return None
    mode, register = (opcode >> 3) & 7, opcode & 7
    if mode in (2, 5, 6) or (mode == 7 and register in (2, 3)):
        decoded = read_ea(rom, pc, mode, register, 4)
    else:
        return None
    if decoded is None:
        return {"kind": "INDIRECT", "status": "CONSUMER_DECODE_UNRESOLVED"}
    return {"kind": "JSR" if form == 0x4E80 else "JMP",
            "opcode": opcode, "pc": pc, "ea": decoded[0],
            "instruction_end": decoded[1]}


def exact_target_range(rom: bytes, pc: int) -> dict[str, Any] | None:
    """Decode the full target range for the bounded executable proof subset."""
    if pc < 0 or pc + 2 > len(rom):
        return None
    opcode = int.from_bytes(rom[pc:pc + 2], "big")
    # The micro-ROM deliberately targets RTS and JMP abs.L. Unknown target
    # forms remain unmapped until an authoritative project decoder is wired in.
    if opcode == 0x4E75:
        end, kind = pc + 2, "M68K_RTS"
    elif opcode == 0x4EF9 and pc + 6 <= len(rom):
        end, kind = pc + 6, "M68K_JMP_ABSOLUTE_LONG"
    else:
        return None
    return {"start": pc, "end": end, "type": kind,
            "bytes_hex": rom[pc:end].hex().upper()}
