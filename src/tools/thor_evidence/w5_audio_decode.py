"""M12 W5 Deterministic Audio Decoder.

Decodes compressed Beyond Oasis delta-PCM audio streams (Format A)
into canonical Intermediate Representation (IR) and PCM sample sequences.
Directly implements reverse-engineered Z80 driver routines at 0x080D..0x0880 (Mode 0)
and 0x0899..0x0960 (Mode 1).
"""

from __future__ import annotations

from typing import Sequence

from .w5_audio_format import (
    DELTA_TABLE,
    INITIAL_ACCUMULATOR,
    AudioDescriptor,
    AudioResourceIR,
    AudioToken,
)


def decode_format_a_stream(
    compressed_bytes: bytes,
    mode: int = 0,
    initial_acc: int = INITIAL_ACCUMULATOR,
) -> tuple[list[AudioToken], list[int]]:
    """Decodes a stream of compressed bytes into tokens and PCM sample values.

    Args:
        compressed_bytes: Raw compressed data bytes from ROM.
        mode: Playback mode (0 = standard, 1 = interleaved hold).
        initial_acc: Starting accumulator value in register D (default 0x80).

    Returns:
        tuple of (tokens list, pcm_samples list).
    """
    tokens: list[AudioToken] = []
    pcm_samples: list[int] = []

    d = initial_acc & 0xFF
    e = 0  # Initial delta
    token_idx = 0

    for byte_idx, byte_val in enumerate(compressed_bytes):
        hi_nibble = (byte_val >> 4) & 0x0F
        lo_nibble = byte_val & 0x0F

        for pos, nibble in (("HIGH", hi_nibble), ("LOW", lo_nibble)):
            if nibble == 0:
                kind = "REPEAT"
                repeat_count = 3
                step_samples: list[int] = []
                for _ in range(repeat_count):
                    d = (d + e) & 0xFF
                    pcm_samples.append(d)
                    step_samples.append(d)
                    if mode == 1:
                        # Mode 1 routine calls 0x0999 which re-emits D
                        pcm_samples.append(d)
                        step_samples.append(d)
                delta_val = e
            else:
                kind = "DELTA"
                repeat_count = 1
                delta = DELTA_TABLE[nibble]
                assert delta is not None
                e = delta
                d = (d + e) & 0xFF
                pcm_samples.append(d)
                step_samples = [d]
                if mode == 1:
                    pcm_samples.append(d)
                    step_samples.append(d)
                delta_val = e

            token = AudioToken(
                token_index=token_idx,
                byte_index=byte_idx,
                nibble_pos=pos,
                kind=kind,
                delta_value=delta_val,
                repeat_count=repeat_count,
                pcm_samples=tuple(step_samples),
                source_nibble=nibble,
            )
            tokens.append(token)
            token_idx += 1

    return tokens, pcm_samples


def decode_audio_resource(
    rom_bytes: bytes,
    descriptor: AudioDescriptor,
    resource_id: str,
) -> AudioResourceIR:
    """Decodes an entire audio resource identified by an AudioDescriptor."""
    start = descriptor.physical_address
    end = descriptor.physical_end_exclusive
    if end > len(rom_bytes):
        raise ValueError(
            f"STOP_AUDIO_RESOURCE_EXCEEDS_ROM: {end} > {len(rom_bytes)}"
        )

    raw_bytes = rom_bytes[start:end]
    if len(raw_bytes) != descriptor.byte_length:
        raise ValueError(
            f"STOP_AUDIO_RESOURCE_SIZE_MISMATCH: expected {descriptor.byte_length}, got {len(raw_bytes)}"
        )

    tokens, pcm = decode_format_a_stream(
        raw_bytes,
        mode=descriptor.mode,
        initial_acc=INITIAL_ACCUMULATOR,
    )

    fmt_id = "AUDIO_FORMAT_A_MODE0" if descriptor.mode == 0 else "AUDIO_FORMAT_A_MODE1"
    return AudioResourceIR(
        format_id=fmt_id,
        resource_id=resource_id,
        descriptor=descriptor,
        initial_accumulator=INITIAL_ACCUMULATOR,
        tokens=tuple(tokens),
        pcm_samples=tuple(pcm),
    )
