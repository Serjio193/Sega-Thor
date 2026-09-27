"""W4 Audio Sink Tracker and Event Classifier.

Tracks hardware audio ports (YM2612 ports 0x4000-0x4003, PSG port 0x7F11),
enforces port-local latch pairing without cross-port leakage, and identifies
DAC data/enable registers as exact hardware facts.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Iterator, Sequence

from .w3_z80_evidence import (
    CPU_Z80,
    DOMAIN_PSG,
    DOMAIN_YM2612,
    W3Record,
)


class AudioSinkType(str, Enum):
    YM2612_REGISTER_WRITE = "YM2612_REGISTER_WRITE"
    YM2612_DAC_WRITE = "YM2612_DAC_WRITE"
    YM2612_UNLATCHED_DATA_WRITE = "YM2612_UNLATCHED_DATA_WRITE"
    PSG_WRITE = "PSG_WRITE"


class TruthClass(str, Enum):
    OBSERVED = "OBSERVED"
    DERIVED_EXACT = "DERIVED_EXACT"
    STATIC_VERIFIED = "STATIC_VERIFIED"
    HYPOTHESIS = "HYPOTHESIS"


@dataclass(frozen=True)
class AudioSinkEvent:
    sink_type: AudioSinkType
    truth_class: TruthClass
    port: int
    register: int | None
    value: int
    data_record: W3Record
    address_record: W3Record | None = None
    is_dac_register: bool = False
    is_psg: bool = False

    @property
    def master_time(self) -> int:
        return self.data_record.master_time

    @property
    def stream_sequence(self) -> int:
        return self.data_record.stream_sequence

    @property
    def pc(self) -> int:
        return self.data_record.pc

    def to_dict(self) -> dict[str, object]:
        return {
            "sink_type": self.sink_type.value,
            "truth_class": self.truth_class.value,
            "port": f"0x{self.port:04X}",
            "register": f"0x{self.register:02X}" if self.register is not None else None,
            "value": f"0x{self.value:02X}",
            "data_stream_seq": self.data_record.stream_sequence,
            "data_master_time": self.data_record.master_time,
            "address_stream_seq": (
                self.address_record.stream_sequence if self.address_record else None
            ),
            "address_master_time": (
                self.address_record.master_time if self.address_record else None
            ),
            "pc": f"0x{self.pc:04X}",
            "is_dac_register": self.is_dac_register,
            "is_psg": self.is_psg,
        }


class AudioSinkTracker:
    """Maintains independent port-local address latches for YM2612 and PSG.

    Rules:
    - Port 0x4000 (Part 1 Addr) latches register address for Port 0x4001 (Part 1 Data).
    - Port 0x4002 (Part 2 Addr) latches register address for Port 0x4003 (Part 2 Data).
    - New address write to a port replaces any previous unconsumed latch for that port.
    - Part 1 and Part 2 NEVER cross-pair (e.g. 0x4000 never pairs with 0x4003).
    - If 0x4001 or 0x4003 receives a data write without a prior address latch on that
      port, it is emitted as YM2612_UNLATCHED_DATA_WRITE.
    - YM2612 Register 0x2A is DAC Data; Register 0x2B is DAC Enable.
    - Port 0x7F11 is PSG Data Write.
    """

    def __init__(self) -> None:
        self.part1_latch: W3Record | None = None
        self.part2_latch: W3Record | None = None
        self.sink_events: list[AudioSinkEvent] = []
        self.unlatched_writes: list[W3Record] = []
        self.address_overwrites: int = 0

    def process_record(self, record: W3Record) -> AudioSinkEvent | None:
        """Processes a single record and returns an AudioSinkEvent if an audio sink is hit."""
        if not record.is_bus_write:
            return None

        # YM2612 Port writes
        if record.domain == DOMAIN_YM2612 or (0x4000 <= (record.address & 0xFFFF) <= 0x4003):
            addr = record.address & 0xFFFF
            val = record.value & 0xFF

            if addr == 0x4000:
                if self.part1_latch is not None:
                    self.address_overwrites += 1
                self.part1_latch = record
                return None

            if addr == 0x4002:
                if self.part2_latch is not None:
                    self.address_overwrites += 1
                self.part2_latch = record
                return None

            if addr == 0x4001:
                addr_rec = self.part1_latch
                reg = (addr_rec.value & 0xFF) if addr_rec is not None else None
                is_dac = reg in (0x2A, 0x2B)
                sink_type = (
                    AudioSinkType.YM2612_DAC_WRITE
                    if reg == 0x2A
                    else (
                        AudioSinkType.YM2612_REGISTER_WRITE
                        if addr_rec is not None
                        else AudioSinkType.YM2612_UNLATCHED_DATA_WRITE
                    )
                )
                if addr_rec is None:
                    self.unlatched_writes.append(record)

                event = AudioSinkEvent(
                    sink_type=sink_type,
                    truth_class=TruthClass.OBSERVED,
                    port=addr,
                    register=reg,
                    value=val,
                    data_record=record,
                    address_record=addr_rec,
                    is_dac_register=is_dac,
                    is_psg=False,
                )
                self.sink_events.append(event)
                return event

            if addr == 0x4003:
                addr_rec = self.part2_latch
                reg = (addr_rec.value & 0xFF) if addr_rec is not None else None
                sink_type = (
                    AudioSinkType.YM2612_REGISTER_WRITE
                    if addr_rec is not None
                    else AudioSinkType.YM2612_UNLATCHED_DATA_WRITE
                )
                if addr_rec is None:
                    self.unlatched_writes.append(record)

                event = AudioSinkEvent(
                    sink_type=sink_type,
                    truth_class=TruthClass.OBSERVED,
                    port=addr,
                    register=reg,
                    value=val,
                    data_record=record,
                    address_record=addr_rec,
                    is_dac_register=False,
                    is_psg=False,
                )
                self.sink_events.append(event)
                return event

        # PSG Port write
        if record.domain == DOMAIN_PSG or (record.address & 0xFFFF) == 0x7F11:
            addr = record.address & 0xFFFF
            val = record.value & 0xFF
            event = AudioSinkEvent(
                sink_type=AudioSinkType.PSG_WRITE,
                truth_class=TruthClass.OBSERVED,
                port=addr,
                register=None,
                value=val,
                data_record=record,
                address_record=None,
                is_dac_register=False,
                is_psg=True,
            )
            self.sink_events.append(event)
            return event

        return None

    def process_all(self, records: Sequence[W3Record]) -> list[AudioSinkEvent]:
        events: list[AudioSinkEvent] = []
        for r in records:
            ev = self.process_record(r)
            if ev is not None:
                events.append(ev)
        return events
