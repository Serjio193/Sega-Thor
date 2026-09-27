"""Fail-closed Genesis VDP protocol decoding for W2 sideband analysis."""

from __future__ import annotations

from typing import Any, Iterable, Mapping


VDP_BASE = 0xC00000
VDP_PORT_MASK = 0x1F
REGISTER_WRITES = "register_writes"


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def port_kind(address: int) -> str:
    port = (address - VDP_BASE) & VDP_PORT_MASK
    if port in (0x00, 0x02):
        return "data"
    if port in (0x04, 0x06):
        return "control"
    return "other"


def _target(code: int) -> str:
    return {1: "VRAM", 3: "CRAM", 5: "VSRAM"}.get(code & 0x0F, "OTHER")


def _dma_mode(reg23: int) -> str:
    return {2: "FILL", 3: "COPY"}.get(reg23 >> 6, "68K_BUS")


def _identity(event: Mapping[str, Any]) -> dict[str, Any]:
    return {"run_id": event.get("run_id"), "epoch": event.get("epoch"),
            "frame": event.get("frame", "UNRESOLVED")}


class VdpProtocolDecoder:
    """Decode only facts present in ordered CPU-visible VDP port writes."""

    def __init__(self) -> None:
        self.registers: dict[int, dict[str, Any]] = {}
        self.pending_word: dict[str, Any] | None = None
        self.commands: list[dict[str, Any]] = []
        self.register_writes: list[dict[str, Any]] = []
        self.data_writes: list[dict[str, Any]] = []
        self.incomplete_commands: list[dict[str, Any]] = []
        self.dma_operations: list[dict[str, Any]] = []

    def _register(self, event: Mapping[str, Any], value: int) -> dict[str, Any]:
        register = (value >> 8) & 0x1F
        item = {
            "stream_sequence": event["stream_sequence"],
            "instruction_sequence": event["instruction_sequence"],
            "pc": event["pc"],
            "address": event["address"],
            "value": value & 0xFF,
            "register": register,
            "raw_control_word": value & 0xFFFF,
            "truth": "OBSERVED",
        }
        item.update(_identity(event))
        self.registers[register] = item
        self.register_writes.append(item)
        return item

    def _command(self, first: Mapping[str, Any], second: Mapping[str, Any]) -> dict[str, Any]:
        first_word = int(first["value"]) & 0xFFFF
        second_word = int(second["value"]) & 0xFFFF
        address = (first_word & 0x3FFF) | ((second_word & 0x0003) << 14)
        code = ((first_word >> 14) & 0x03) | ((second_word >> 2) & 0x3C)
        target = _target(code)
        result: dict[str, Any] = {
            "first_stream_sequence": first["stream_sequence"],
            "second_stream_sequence": second["stream_sequence"],
            "first_instruction_sequence": first["instruction_sequence"],
            "second_instruction_sequence": second["instruction_sequence"],
            "pc": first["pc"],
            "first_word": first_word,
            "second_word": second_word,
            "address": address,
            "code": code,
            "target": target,
            "dma_requested": bool(code & 0x20),
            "truth": "DERIVED_EXACT",
        }
        first_identity = _identity(first)
        second_identity = _identity(second)
        result.update(first_identity if first_identity == second_identity else {
            "run_id": None, "epoch": None, "frame": "UNRESOLVED"})
        if code & 0x20:
            result["dma"] = self._dma(result)
        self.commands.append(result)
        return result

    def _dma(self, command: Mapping[str, Any]) -> dict[str, Any]:
        needed = (19, 20, 21, 22, 23)
        present = all(reg in self.registers for reg in needed)
        dma: dict[str, Any] = {
            "programming_complete": present,
            "transfer_observed": False,
            "transfer_observation": "CPU_PROGRAMMING_ONLY",
            "truth": "DERIVED_EXACT" if present else "OBSERVED",
            "target": command["target"],
            "destination_address": command["address"],
            "command_code": command["code"],
            "run_id": command.get("run_id"), "epoch": command.get("epoch"),
            "frame": command.get("frame", "UNRESOLVED"),
        }
        if not present:
            dma["missing_registers"] = [reg for reg in needed if reg not in self.registers]
            return dma
        reg = {number: self.registers[number]["value"] for number in needed}
        length_words = (reg[20] << 8) | reg[19]
        if length_words == 0:
            length_words = 0x10000
        source_word = (reg[23] << 16) | (reg[22] << 8) | reg[21]
        dma.update({
            "mode": _dma_mode(reg[23]),
            "source_word_address": source_word,
            "source_byte_address": (reg[23] << 17) | (reg[22] << 9) | (reg[21] << 1),
            "length_words": length_words,
            "length_bytes": length_words * 2,
            "registers": {str(number): reg[number] for number in needed},
        })
        return dma

    def consume(self, event: Mapping[str, Any]) -> None:
        if event.get("direction") != "WRITE" or port_kind(int(event["address"])) == "other":
            return
        kind = port_kind(int(event["address"]))
        value = int(event["value"])
        if kind == "data":
            self.data_writes.append({
                "stream_sequence": event["stream_sequence"],
                "instruction_sequence": event["instruction_sequence"],
                "pc": event["pc"],
                "address": event["address"],
                "value": value,
                "width": event["width"],
                "target": self.commands[-1]["target"] if self.commands else "UNKNOWN",
                "command_address": self.commands[-1]["address"] if self.commands else None,
                "truth": "OBSERVED",
            })
            self.data_writes[-1].update(_identity(event))
            return
        if event["width"] != 16:
            self.incomplete_commands.append({
                "stream_sequence": event["stream_sequence"],
                "reason": "CONTROL_WORD_WIDTH_NOT_16",
                "truth": "OBSERVED", **_identity(event),
            })
            return
        if value & 0xC000 == 0x8000:
            self._register(event, value)
            return
        if self.pending_word is None:
            self.pending_word = dict(event)
            return
        first = self.pending_word
        self.pending_word = None
        self._command(first, event)

    def consume_all(self, events: Iterable[Mapping[str, Any]]) -> None:
        for event in events:
            self.consume(event)
        if self.pending_word is not None:
            self.incomplete_commands.append({
                "stream_sequence": self.pending_word["stream_sequence"],
                "first_word": self.pending_word["value"],
                "reason": "CONTROL_COMMAND_MISSING_SECOND_WORD",
                "truth": "OBSERVED", **_identity(self.pending_word),
            })
            self.pending_word = None

    def report(self) -> dict[str, Any]:
        dma = [item["dma"] | {
            "first_stream_sequence": item["first_stream_sequence"],
            "second_stream_sequence": item["second_stream_sequence"],
        } for item in self.commands if "dma" in item]
        return {
            "register_writes": sorted(self.register_writes, key=lambda item: item["stream_sequence"]),
            "control_commands": sorted(self.commands,
                                        key=lambda item: item["first_stream_sequence"]),
            "data_port_writes": sorted(self.data_writes,
                                        key=lambda item: item["stream_sequence"]),
            "dma_operations": sorted(dma, key=lambda item: item["first_stream_sequence"]),
            "incomplete_commands": sorted(self.incomplete_commands,
                                            key=lambda item: item["stream_sequence"]),
            "limitations": [
                "CPU-visible VDP port writes only; no internal DMA bytes are attributed to M68K BUS_READ",
                "no rendering, sprite, music, song or instrument semantics are assigned",
            ],
        }


__all__ = ["VDP_BASE", "VdpProtocolDecoder", "port_kind"]
