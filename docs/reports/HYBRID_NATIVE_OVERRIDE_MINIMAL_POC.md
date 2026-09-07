# M11.29 Minimal Safe Native Override PoC

## Decision

`HYBRID_NATIVE_OVERRIDE_MINIMAL_PROVEN`

The target is `0x2D66`, range `[0x2D66,0x2D84)`. It is a ten-instruction
direct-entry leaf with `RTS`, one local `DBF` loop, no nested call, no indirect
control flow, no VDP/Z80/I/O access and no self-modifying write. The only
observable effects are two source-byte post-increments through `A6`, sequential
word output at `0xFF134C + first_byte`, the eight-byte `MOVEM` save/restore
window and the RTS transition. The final `MOVE.W` determines N/Z/V/C and CCR.X
is preserved, so full SR is bounded. The nearby natural `0x6121A` candidate was
rejected because it writes VDP address `0xC00011`. `0x62CC` and `0xA8DA` were
not present in the deterministic 600-frame PC bitmap.

## Identity and scenario

| Field | Value |
| --- | --- |
| Canonical ROM | USA `Beyond Oasis`, 3,145,728 bytes |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| GPGX source/build identity | `d60d079934977aa6973e220d123533387159f66e` |
| GPGX DLL SHA-256 | `4488ae775fd8252d8a8cb169000f20ac388e6d102fa6e1b1d4a4eccaa7f49799` |
| Scenario | cold-reset, neutral input, 600 frames |
| Target | `0x2D66` |

The existing 600-frame PC bitmap contains the target. No manual gameplay,
ID3 investigation or new coverage collection was used.

## Shadow and override evidence

The shadow adapter captures D0-D7, A0-A7, PC, full SR, canonical source bytes,
the output range and the exact 12-byte stack window. It compares register
deltas, every bounded RAM write and read footprint, MOVEM bus order, output
bytes, source consumption and the RTS return state. The single natural call
produced one comparison and zero divergences. No interrupt occurred while the
routine was active.

The observed call consumed 28 canonical ROM source bytes and produced 26 output
bytes at `0xFF134C + 2`; the exact saved stack window was 12 bytes including
the caller return address.

The native override applies the proven output and saved-register effects, sets
the post-RTS A6/A7/SR values and transfers PC using GPGX's own `m68k_set_reg`
state transition. The original body was skipped: native override calls = 1 and
original target-body instruction starts = 0. The run completed all 600 frames.

| Observable | EMULATED | SHADOW_NATIVE | NATIVE_OVERRIDE |
| --- | ---: | ---: | ---: |
| Natural calls | 1 | 1 | 1 |
| Shadow comparisons | 0 | 1 | 0 |
| Divergences | 0 | 0 | 0 |
| Native override calls | 0 | 0 | 1 |
| Original target-body starts | 0 | 33 | 0 |
| Scenario completed | yes | yes | yes |
| Interrupts inside target | 0 | 0 | 0 |

The EMULATED and NATIVE_OVERRIDE state checkpoint sequence SHA-256 is
`1690fd7d9bd1a26aeacaccfeff943f17f2bb30a5e130f28a88973916e8e9abca`.
The video sequence SHA-256 is
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
Both sequences match exactly. The explicit body-skip proof is the zero target
body instruction-start count in the override report while the callback records
one native override call and the scenario continues normally.

## Scope boundary

The adapter is under `src/tools/hybrid/` and is linked only into developer
tools/tests. Production Sega-Thor code has no dependency on GPGX, M68K context,
ROM PCs, emulated RAM layout or the CPU emulator. `0x3820` remains shadow-only;
its CCR.X, interrupt/timing and prefetch contract is unchanged and out of scope.
No ROM, capture, emulator binary or generated evidence blob is committed.
