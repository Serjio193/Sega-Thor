"""M12 W5 Deterministic Audio Encoder.

Reconstructs byte-identical original ROM binary data from canonical Intermediate
Representation (IR) tokens without reading or storing original binary blobs.
"""

from __future__ import annotations

from typing import Sequence

from .w5_audio_format import (
    AudioResourceIR,
    AudioToken,
    semantic_token_to_nibble,
)


def encode_format_a_tokens(tokens: Sequence[AudioToken]) -> bytes:
    """Encodes a sequence of AudioTokens into packed delta-PCM bytes.

    Each byte is constructed from exactly two consecutive tokens:
    token 2*i (HIGH nibble) and token 2*i + 1 (LOW nibble).
    Reconstructs nibbles strictly from semantic state (kind, delta_value)
    without reading source_nibble or original ROM bytes.
    """
    if len(tokens) % 2 != 0:
        raise ValueError(
            f"STOP_AUDIO_TOKEN_COUNT_UNPAIRED: expected even token count, got {len(tokens)}"
        )

    out = bytearray(len(tokens) // 2)
    for byte_idx in range(len(out)):
        t_high = tokens[byte_idx * 2]
        t_low = tokens[byte_idx * 2 + 1]

        if t_high.nibble_pos != "HIGH":
            raise ValueError(
                f"STOP_AUDIO_TOKEN_ORDER_MISMATCH: expected HIGH at {byte_idx*2}, got {t_high.nibble_pos}"
            )
        if t_low.nibble_pos != "LOW":
            raise ValueError(
                f"STOP_AUDIO_TOKEN_ORDER_MISMATCH: expected LOW at {byte_idx*2+1}, got {t_low.nibble_pos}"
            )

        # Non-tautological reconstruction from semantic state only
        hi_val = semantic_token_to_nibble(t_high.kind, t_high.delta_value)
        lo_val = semantic_token_to_nibble(t_low.kind, t_low.delta_value)
        out[byte_idx] = (hi_val << 4) | lo_val

    return bytes(out)


def encode_audio_resource(ir: AudioResourceIR) -> bytes:
    """Encodes an AudioResourceIR back to its original binary byte sequence."""
    raw = encode_format_a_tokens(ir.tokens)
    if len(raw) != ir.byte_length:
        raise ValueError(
            f"STOP_ENCODED_BYTE_LENGTH_MISMATCH: expected {ir.byte_length}, got {len(raw)}"
        )
    return raw
