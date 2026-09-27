"""M12 W5 Audio Resource Format Specification and Data Model.

Defines neutral, non-promotional data structures for Beyond Oasis
non-linear delta-PCM audio resources (Format A).
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Sequence

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3145728

# Non-linear delta lookup table stored at Z80 RAM 0x0008..0x0016
# Nibble 0 is special: repeats previous delta for 3 sample ticks.
# Nibbles 1..15 index directly into the delta table.
DELTA_TABLE: tuple[int | None, ...] = (
    None,   # Nibble 0: repeat previous delta (count=3)
    0,      # Nibble 1: delta = 0
    1,      # Nibble 2: delta = +1
    2,      # Nibble 3: delta = +2
    6,      # Nibble 4: delta = +6
    12,     # Nibble 5: delta = +12
    24,     # Nibble 6: delta = +24
    48,     # Nibble 7: delta = +48
    96,     # Nibble 8: delta = +96
    -96,    # Nibble 9: delta = -96 (0xA0)
    -48,    # Nibble 10: delta = -48 (0xD0)
    -24,    # Nibble 11: delta = -24 (0xE8)
    -12,    # Nibble 12: delta = -12 (0xF4)
    -6,     # Nibble 13: delta = -6 (0xFA)
    -2,     # Nibble 14: delta = -2 (0xFE)
    -1,     # Nibble 15: delta = -1 (0xFF)
)

# 8 consecutive 32KB audio banks in Genesis ROM (0x080000..0x0B8000)
AUDIO_BANK_PHYSICAL_BASES: tuple[int, ...] = (
    0x080000, 0x088000, 0x090000, 0x098000,
    0x0A0000, 0x0A8000, 0x0B0000, 0x0B8000,
)

AUDIO_BANK_INDICES: tuple[int, ...] = (
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
)

DESCRIPTORS_PER_BANK: int = 16
DESCRIPTOR_ENTRY_SIZE: int = 5
DESCRIPTOR_TABLE_SIZE: int = DESCRIPTORS_PER_BANK * DESCRIPTOR_ENTRY_SIZE  # 80 bytes (0x50)
BANK_WINDOW_BASE: int = 0x8000
INITIAL_ACCUMULATOR: int = 0x80  # Midpoint unsigned 8-bit PCM (silence)


@dataclass(frozen=True)
class AudioDescriptor:
    """Exact descriptor table entry defining an audio resource in ROM."""

    bank_id: int
    bank_physical_base: int
    entry_index: int
    logical_address: int
    physical_address: int
    byte_length: int
    mode: int

    @property
    def physical_end_exclusive(self) -> int:
        return self.physical_address + self.byte_length

    def to_dict(self) -> dict[str, Any]:
        return {
            "bank_id": f"0x{self.bank_id:02X}",
            "bank_physical_base": f"0x{self.bank_physical_base:06X}",
            "entry_index": self.entry_index,
            "logical_address": f"0x{self.logical_address:04X}",
            "physical_address": f"0x{self.physical_address:06X}",
            "physical_end_exclusive": f"0x{self.physical_end_exclusive:06X}",
            "byte_length": self.byte_length,
            "mode": self.mode,
        }


# Bijective inverse mapping from delta value to nibble code (1..15)
INVERSE_DELTA_TABLE: dict[int, int] = {
    0: 1, 1: 2, 2: 3, 6: 4, 12: 5, 24: 6, 48: 7, 96: 8,
    -96: 9, -48: 10, -24: 11, -12: 12, -6: 13, -2: 14, -1: 15,
}


def semantic_token_to_nibble(kind: str, delta_value: int) -> int:
    """Computes exact 4-bit nibble from decoded semantic state only.

    Proves unique inverse mapping:
      kind == 'REPEAT' -> 0x0
      kind == 'DELTA' with delta_value -> exact unique nibble in 1..15
    """
    if kind == "REPEAT":
        return 0
    if kind == "DELTA":
        nibble = INVERSE_DELTA_TABLE.get(delta_value)
        if nibble is None:
            raise ValueError(f"STOP_INVALID_SEMANTIC_DELTA: {delta_value}")
        return nibble
    raise ValueError(f"STOP_UNKNOWN_SEMANTIC_KIND: {kind}")


@dataclass(frozen=True)
class AudioToken:
    """Exact intermediate representation of a single nibble audio event."""

    token_index: int
    byte_index: int
    nibble_pos: str  # "HIGH" or "LOW" (ENCODE_INPUT)
    kind: str  # "DELTA" or "REPEAT" (ENCODE_INPUT)
    delta_value: int  # Exact integer delta (ENCODE_INPUT)
    repeat_count: int  # 1 for DELTA, 3 for REPEAT (SEMANTIC_STATE)
    pcm_samples: tuple[int, ...]  # Emitted PCM samples (SEMANTIC_STATE)
    source_nibble: int | None = None  # PROVENANCE_ONLY (never read by encoder)

    def to_dict(self) -> dict[str, Any]:
        return {
            "token_index": self.token_index,
            "byte_index": self.byte_index,
            "nibble_pos": self.nibble_pos,
            "kind": self.kind,
            "delta_value": self.delta_value,
            "repeat_count": self.repeat_count,
            "pcm_samples": list(self.pcm_samples),
            "source_nibble": self.source_nibble,
        }


@dataclass(frozen=True)
class AudioResourceIR:
    """Canonical Intermediate Representation (IR) of an audio resource."""

    format_id: str
    resource_id: str
    descriptor: AudioDescriptor
    initial_accumulator: int
    tokens: tuple[AudioToken, ...]
    pcm_samples: tuple[int, ...]

    @property
    def byte_length(self) -> int:
        return self.descriptor.byte_length

    @property
    def total_pcm_samples(self) -> int:
        return len(self.pcm_samples)

    @property
    def total_tokens(self) -> int:
        return len(self.tokens)

    def to_dict(self, include_tokens: bool = False, max_pcm_preview: int = 64) -> dict[str, Any]:
        preview = list(self.pcm_samples[:max_pcm_preview])
        out: dict[str, Any] = {
            "format_id": self.format_id,
            "resource_id": self.resource_id,
            "descriptor": self.descriptor.to_dict(),
            "initial_accumulator": f"0x{self.initial_accumulator:02X}",
            "total_tokens": self.total_tokens,
            "total_pcm_samples": self.total_pcm_samples,
            "pcm_sample_preview": preview,
        }
        if include_tokens:
            out["tokens"] = [t.to_dict() for t in self.tokens]
        return out


def parse_bank_descriptors(rom_bytes: bytes, bank_phys_base: int, bank_id: int) -> list[AudioDescriptor]:
    """Parses 16 5-byte descriptor entries at the base of an audio bank."""
    descriptors: list[AudioDescriptor] = []
    for entry_idx in range(DESCRIPTORS_PER_BANK):
        offset = bank_phys_base + entry_idx * DESCRIPTOR_ENTRY_SIZE
        entry_raw = rom_bytes[offset : offset + DESCRIPTOR_ENTRY_SIZE]
        if len(entry_raw) < DESCRIPTOR_ENTRY_SIZE:
            break
        addr = entry_raw[0] | (entry_raw[1] << 8)
        length = entry_raw[2] | (entry_raw[3] << 8)
        mode = entry_raw[4]

        # Valid active entry has non-zero address and length
        if addr >= BANK_WINDOW_BASE and length > 0:
            phys_addr = bank_phys_base + (addr - BANK_WINDOW_BASE)
            desc = AudioDescriptor(
                bank_id=bank_id,
                bank_physical_base=bank_phys_base,
                entry_index=entry_idx,
                logical_address=addr,
                physical_address=phys_addr,
                byte_length=length,
                mode=mode,
            )
            descriptors.append(desc)
    return descriptors
