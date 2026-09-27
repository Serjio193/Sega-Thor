"""W3 Cross-CPU Timeline and Causal Relation Validator.

Constructs an authoritative cross-CPU timeline using common master_time,
and validates exact causal relations (68K write -> Z80 read -> audio write)
without speculative inference.
"""

from __future__ import annotations

import enum
from dataclasses import dataclass
from typing import Iterable, Iterator, Sequence

from .w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_PSG,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    DOMAIN_Z80_WINDOW,
    W3Record,
)

class TieBreak(enum.IntEnum):
    STRICT_BEFORE = 0
    SAME_MASTER_TIME_UNORDERED = 1


MASTER_TIME_SEMANTICS: str = "FRAME_RELATIVE"
MASTER_TIME_UNIT: str = "Genesis master clock cycles"
CANONICAL_TIMELINE_KEY: tuple[str, str, str, str] = ("run_id", "epoch", "frame", "master_time")


@dataclass(frozen=True)
class CrossCpuEvent:
    run_id: int
    epoch: int
    frame: int
    record: W3Record
    tie_break: TieBreak

    @property
    def canonical_timeline_key(self) -> tuple[int, int, int, int]:
        return (self.run_id, self.epoch, self.frame, self.record.master_time)

    @property
    def sort_key(self) -> tuple[int, int, int, int, int]:
        return (self.run_id, self.epoch, self.frame, self.record.master_time, self.tie_break.value)

    def can_relate(self, other: CrossCpuEvent) -> bool:
        """Determines if two events share the same execution context (run_id, epoch)."""
        return self.run_id == other.run_id and self.epoch == other.epoch

    def strictly_precedes(self, other: CrossCpuEvent) -> bool:
        """Evaluates strict temporal precedence.

        Requires:
        - Exact match on run_id and epoch (cross-run or cross-epoch events cannot be compared).
        - Precedence in canonical time: frame < other.frame, or (same frame and master_time < other.master_time).

        If timestamps are identical (same frame, same master_time), returns False (SAME_MASTER_TIME_UNORDERED).
        """
        if self.run_id != other.run_id:
            raise ValueError(
                f"Cannot compare cross-run events: run_id {self.run_id} vs {other.run_id}"
            )
        if self.epoch != other.epoch:
            raise ValueError(
                f"Cannot compare cross-epoch events: epoch {self.epoch} vs {other.epoch}"
            )
        if self.frame < other.frame:
            return True
        if self.frame > other.frame:
            return False
        return self.record.master_time < other.record.master_time


@dataclass(frozen=True)
class CausalWitness:
    """Witness proving exact causal communication across CPUs."""

    m68k_write_event: CrossCpuEvent
    z80_read_event: CrossCpuEvent
    audio_write_event: CrossCpuEvent | None

    @property
    def z80_ram_offset(self) -> int:
        return self.z80_read_event.record.address & 0x1FFF

    @property
    def write_time(self) -> int:
        return self.m68k_write_event.record.master_time

    @property
    def read_time(self) -> int:
        return self.z80_read_event.record.master_time

    @property
    def audio_time(self) -> int | None:
        return self.audio_write_event.record.master_time if self.audio_write_event else None


class CrossCpuTimeline:
    """Maintains and validates cross-CPU emulated time ordering."""

    def __init__(self, records: Iterable[W3Record], run_id: int = 0, epoch: int = 0) -> None:
        self.run_id = run_id
        self.epoch = epoch
        self.timeline: list[CrossCpuEvent] = []

        current_frame = 0
        raw_events: list[tuple[int, W3Record]] = []
        for r in records:
            if r.is_frame_boundary:
                current_frame = r.frame_number
            raw_events.append((current_frame, r))

        time_counts: dict[tuple[int, int], int] = {}
        for f, r in raw_events:
            key = (f, r.master_time)
            time_counts[key] = time_counts.get(key, 0) + 1

        for f, r in raw_events:
            key = (f, r.master_time)
            tb = TieBreak.SAME_MASTER_TIME_UNORDERED if time_counts[key] > 1 else TieBreak.STRICT_BEFORE
            self.timeline.append(CrossCpuEvent(run_id, epoch, f, r, tb))

        self.timeline.sort(key=lambda e: e.sort_key)

    @classmethod
    def from_events(cls, events: Iterable[CrossCpuEvent]) -> CrossCpuTimeline:
        """Constructs a timeline from pre-existing CrossCpuEvents.

        Enforces that all events share the exact same run_id and epoch.
        """
        event_list = list(events)
        if not event_list:
            return cls([], run_id=0, epoch=0)
        first_run = event_list[0].run_id
        first_epoch = event_list[0].epoch
        for e in event_list:
            if e.run_id != first_run:
                raise ValueError(
                    f"Cannot construct timeline across different run_id: {first_run} vs {e.run_id}"
                )
            if e.epoch != first_epoch:
                raise ValueError(
                    f"Cannot construct timeline across different epoch: {first_epoch} vs {e.epoch}"
                )
        instance = cls([], run_id=first_run, epoch=first_epoch)
        instance.timeline = sorted(event_list, key=lambda ev: ev.sort_key)
        return instance

    def join(self, other: CrossCpuTimeline) -> CrossCpuTimeline:
        """Joins another timeline into a unified timeline.

        Requires matching run_id and epoch. Different run_id or epoch are never joined.
        """
        if self.run_id != other.run_id:
            raise ValueError(
                f"Cannot join timelines with different run_id: {self.run_id} vs {other.run_id}"
            )
        if self.epoch != other.epoch:
            raise ValueError(
                f"Cannot join timelines with different epoch: {self.epoch} vs {other.epoch}"
            )
        merged = CrossCpuTimeline([], run_id=self.run_id, epoch=self.epoch)
        merged.timeline = sorted(self.timeline + other.timeline, key=lambda e: e.sort_key)
        return merged

    def __len__(self) -> int:
        return len(self.timeline)

    def __iter__(self) -> Iterator[CrossCpuEvent]:
        return iter(self.timeline)

    def is_strictly_monotonic(self) -> bool:
        """Returns True if every event is >= predecessor in canonical timeline order."""
        for i in range(1, len(self.timeline)):
            prev = self.timeline[i - 1]
            curr = self.timeline[i]
            if curr.canonical_timeline_key < prev.canonical_timeline_key:
                return False
        return True

    def filter_by_cpu(self, cpu_id: int) -> list[CrossCpuEvent]:
        return [e for e in self.timeline if e.record.cpu_id == cpu_id]

    def find_causal_chains(self) -> list[CausalWitness]:
        """Identifies proven causal interactions:

        1. 68K writes to Z80 window (0xA00000..0xA01FFF) at T1
        2. Z80 reads matching byte from Z80 RAM at T2 > T1
        3. Optional subsequent Z80 write to YM2612/PSG at T3 >= T2

        Does NOT infer causality from equal values alone. Requires:
        - Exact address match (Z80 RAM 8K offset)
        - Strict temporal ordering (T1 strictly precedes T2)
        - Shared execution context (same run_id and epoch)
        - Value match at read time
        """
        witnesses: list[CausalWitness] = []
        ram_writes: dict[int, tuple[CrossCpuEvent, int]] = {}

        for event in self.timeline:
            record = event.record
            if (
                record.cpu_id == CPU_68K
                and record.is_bus_write
                and record.domain == DOMAIN_Z80_WINDOW
            ):
                offset = record.address & 0x1FFF
                ram_writes[offset] = (event, record.value & 0xFF)

            elif (
                record.cpu_id == CPU_Z80
                and record.is_bus_read
                and record.domain == DOMAIN_Z80_RAM
            ):
                offset = record.address & 0x1FFF
                if offset in ram_writes:
                    m68k_write, expected_val = ram_writes[offset]
                    actual_val = record.value & 0xFF
                    # Strict temporal precedence required: write happened strictly before read
                    if (
                        m68k_write.can_relate(event)
                        and m68k_write.strictly_precedes(event)
                        and actual_val == expected_val
                    ):
                        witnesses.append(
                            CausalWitness(
                                m68k_write_event=m68k_write,
                                z80_read_event=event,
                                audio_write_event=None,
                            )
                        )

        # Look forward for associated audio write
        result: list[CausalWitness] = []
        for w in witnesses:
            associated_audio: CrossCpuEvent | None = None
            for e in self.timeline:
                r = e.record
                if (
                    r.cpu_id == CPU_Z80
                    and r.is_bus_write
                    and r.domain in (DOMAIN_YM2612, DOMAIN_PSG)
                    and w.z80_read_event.can_relate(e)
                    and (
                        w.z80_read_event.strictly_precedes(e)
                        or (
                            w.z80_read_event.frame == e.frame
                            and w.z80_read_event.record.master_time == e.record.master_time
                            and w.z80_read_event.record.stream_sequence <= r.stream_sequence
                        )
                    )
                ):
                    associated_audio = e
                    break
            result.append(
                CausalWitness(
                    m68k_write_event=w.m68k_write_event,
                    z80_read_event=w.z80_read_event,
                    audio_write_event=associated_audio,
                )
            )

        return result
