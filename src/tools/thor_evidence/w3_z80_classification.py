"""W3 Primitive Event Classification Layer.

Classifies low-level primitive Z80 and audio bus events without any
higher-level music, song, instrument, voice, sample, note, or sound semantics.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Iterator, Sequence

from .w3_z80_evidence import (
    CPU_68K,
    CPU_Z80,
    DOMAIN_BANKED_ROM,
    DOMAIN_PSG,
    DOMAIN_VDP,
    DOMAIN_YM2612,
    DOMAIN_Z80_RAM,
    DOMAIN_Z80_WINDOW,
    W3Record,
)


class W3PrimitiveKind(Enum):
    Z80_INSTRUCTION = "Z80_INSTRUCTION"
    Z80_RAM_READ = "Z80_RAM_READ"
    Z80_RAM_WRITE = "Z80_RAM_WRITE"
    Z80_BANKED_ROM_READ = "Z80_BANKED_ROM_READ"
    YM2612_ADDRESS_WRITE = "YM2612_ADDRESS_WRITE"
    YM2612_DATA_WRITE = "YM2612_DATA_WRITE"
    YM2612_REGISTER_WRITE = "YM2612_REGISTER_WRITE"
    PSG_WRITE = "PSG_WRITE"
    Z80_VDP_ACCESS = "Z80_VDP_ACCESS"
    OTHER_Z80_ACCESS = "OTHER_Z80_ACCESS"
    OBSERVED_68K_Z80_WINDOW_WRITE = "OBSERVED_68K_Z80_WINDOW_WRITE"
    BANK_REGISTER_CHANGE = "BANK_REGISTER_CHANGE"
    FRAME_BOUNDARY = "FRAME_BOUNDARY"


@dataclass(frozen=True)
class W3ClassifiedEvent:
    kind: W3PrimitiveKind
    record: W3Record
    details: dict[str, int | str]


def classify_record(record: W3Record) -> W3ClassifiedEvent | None:
    """Classifies an individual record into a primitive fact."""
    # Frame boundary
    if record.is_frame_boundary:
        return W3ClassifiedEvent(
            kind=W3PrimitiveKind.FRAME_BOUNDARY,
            record=record,
            details={"frame": record.pc},
        )

    # Bank register change
    if record.is_bank_register_change:
        return W3ClassifiedEvent(
            kind=W3PrimitiveKind.BANK_REGISTER_CHANGE,
            record=record,
            details={
                "bit": record.value & 1,
                "new_base_address": record.auxiliary,
            },
        )

    # 68K write to Z80 window
    if (
        record.cpu_id == CPU_68K
        and record.is_bus_write
        and record.domain == DOMAIN_Z80_WINDOW
    ):
        return W3ClassifiedEvent(
            kind=W3PrimitiveKind.OBSERVED_68K_Z80_WINDOW_WRITE,
            record=record,
            details={
                "logical_address": record.address,
                "z80_ram_offset": record.address & 0x1FFF,
                "value": record.value & 0xFF,
            },
        )

    # Z80 events
    if record.cpu_id == CPU_Z80:
        if record.is_instruction:
            return W3ClassifiedEvent(
                kind=W3PrimitiveKind.Z80_INSTRUCTION,
                record=record,
                details={
                    "pc": record.pc,
                    "next_pc": record.address,
                    "length": record.length_or_width,
                    "raw_bytes_hex": record.z80_instruction_bytes.hex(),
                },
            )

        if record.is_bus_read:
            if record.domain == DOMAIN_Z80_RAM:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.Z80_RAM_READ,
                    record=record,
                    details={
                        "logical_address": record.address,
                        "ram_offset": record.address & 0x1FFF,
                        "value": record.value & 0xFF,
                    },
                )
            if record.domain == DOMAIN_BANKED_ROM:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.Z80_BANKED_ROM_READ,
                    record=record,
                    details={
                        "logical_address": record.address,
                        "physical_address": record.auxiliary,
                        "value": record.value & 0xFF,
                    },
                )
            if record.domain == DOMAIN_VDP:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.Z80_VDP_ACCESS,
                    record=record,
                    details={"address": record.address, "value": record.value & 0xFF},
                )
            return W3ClassifiedEvent(
                kind=W3PrimitiveKind.OTHER_Z80_ACCESS,
                record=record,
                details={"address": record.address, "value": record.value & 0xFF},
            )

        if record.is_bus_write:
            if record.domain == DOMAIN_Z80_RAM:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.Z80_RAM_WRITE,
                    record=record,
                    details={
                        "logical_address": record.address,
                        "ram_offset": record.address & 0x1FFF,
                        "value": record.value & 0xFF,
                    },
                )
            if record.domain == DOMAIN_YM2612:
                port = record.address & 3
                val = record.value & 0xFF
                is_addr = (port in (0, 2))
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.YM2612_ADDRESS_WRITE if is_addr else W3PrimitiveKind.YM2612_DATA_WRITE,
                    record=record,
                    details={
                        "port": port,
                        "is_part2": port >= 2,
                        "value": val,
                    },
                )
            if record.domain == DOMAIN_PSG:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.PSG_WRITE,
                    record=record,
                    details={
                        "port": record.address & 0x1F,
                        "value": record.value & 0xFF,
                    },
                )
            if record.domain == DOMAIN_VDP:
                return W3ClassifiedEvent(
                    kind=W3PrimitiveKind.Z80_VDP_ACCESS,
                    record=record,
                    details={"address": record.address, "value": record.value & 0xFF},
                )
            return W3ClassifiedEvent(
                kind=W3PrimitiveKind.OTHER_Z80_ACCESS,
                record=record,
                details={"address": record.address, "value": record.value & 0xFF},
            )

    return None


def classify_ym2612_register_pairs(records: Sequence[W3Record]) -> list[W3ClassifiedEvent]:
    """Identifies exact YM2612_REGISTER_WRITE pairs: address write followed by data write."""
    results: list[W3ClassifiedEvent] = []
    last_addr: W3Record | None = None

    for r in records:
        if r.cpu_id == CPU_Z80 and r.is_bus_write and r.domain == DOMAIN_YM2612:
            port = r.address & 3
            if port in (0, 2):  # address port
                last_addr = r
            elif port in (1, 3) and last_addr is not None:  # data port
                # Check that port matches bank: port 1 matches port 0; port 3 matches port 2
                if port - 1 == (last_addr.address & 3):
                    results.append(
                        W3ClassifiedEvent(
                            kind=W3PrimitiveKind.YM2612_REGISTER_WRITE,
                            record=r,
                            details={
                                "part": 2 if port == 3 else 1,
                                "register": last_addr.value & 0xFF,
                                "value": r.value & 0xFF,
                                "addr_time": last_addr.master_time,
                                "data_time": r.master_time,
                            },
                        )
                    )
                last_addr = None

    return results
