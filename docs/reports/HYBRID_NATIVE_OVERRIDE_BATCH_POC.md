# M11.30 — Hybrid native override small batch

STATUS: `HYBRID_OVERRIDE_NOT_YET_REPEATABLE`.

The bounded batch used the existing cold-reset, neutral-input 600-frame
scenario and developer-only hybrid runner. Canonical USA ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Instrumented GPGX source commit: `d60d079934977aa6973e220d123533387159f66e`.
The rebuilt bridge used for the run had SHA-256
`c269f4852b069340eef203396ce72a2e0272a0241a4a746e32afe00dc1933e71`.

Candidate discovery selected `0x604BC` and `0x61032`. Both are fully decoded,
short direct-RTS leaf routines with bounded RAM/register effects, no VDP/Z80/I/O,
no self-modifying writes and no observed interrupt. `0x6121A` was rejected for
a VDP write, `0x611F4` for YM/Z80 I/O, `0x611EA` for nested calls, and `0x60004`
for a non-RTS branch continuation.

The shared registry observed 14 natural calls: `0x2D66` one, `0x604BC` four and
`0x61032` nine. SHADOW_NATIVE produced 14/14 exact bounded comparisons with
zero divergences and zero interrupts. The EMULATED checkpoint sequence was
`1220aa86c3efbc913b86ce0a5e45b651053ebd513cb512442c5ecd0e060dd1e7`; the video
sequence was `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

NATIVE_OVERRIDE invoked all three targets 14 times, with zero original-body
instruction starts and zero fallback calls (native share 14/14 = 1.0 for the
registered set). The scenario and video completed,
but serialized emulator state diverged from EMULATED. The first 120-frame
differences were four bytes at offsets `3060`, `144468`, `144482` and `144558`;
later checkpoints also diverged. This is an emulator-visible VDP/sound timing
state difference despite matching ordinary register/RAM effects.

The native checkpoint sequence was
`f002ff0a703e1826e461182650acb49567f69e6bcf81e81bdbd192b41c5389b8` while the
EMULATED sequence was `1220aa86c3efbc913b86ce0a5e45b651053ebd513cb512442c5ecd0e060dd1e7`.

Exact blocker: the minimal boundary lacks the per-instruction GPGX bus-refresh
and hardware phase contract needed when a body is skipped. A fixed cycle delta
and GPGX's internal return/prefetch transition do not reproduce that state. No
independent prefetch or timing engine was added.

Decision: `HYBRID_OVERRIDE_NOT_YET_REPEATABLE`. Keep the registry and clean
shadow evidence as developer-only migration infrastructure; stop timing work,
0x3820 repair, ID3 investigation, manual gameplay and production changes.
