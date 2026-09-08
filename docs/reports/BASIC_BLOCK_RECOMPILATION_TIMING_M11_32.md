# M11.32 — Basic-Block Recompilation Timing Proof

Status: `BASIC_BLOCK_RECOMP_TIMING_PROVEN`

Baseline commit: `c1e28b2f7db6b4d756bf29df4b20dbaffa3ba78d`.
Canonical ROM: local USA `Beyond Oasis (USA).md`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
GPGX source/build identity: instrumented checkout commit
`d60d079934977aa6973e220d123533387159f66e`; DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`.
Scenario: cold reset, neutral input, 600 frames.

The developer-only block boundary is installed immediately before GPGX's
opcode fetch/dispatch. Native execution uses GPGX's own immediate fetch,
memory bus, instruction cycle table, prefetch fields and refresh mechanism.
The production Sega-Thor targets do not link this bridge, GPGX, M68KContext,
ROM PCs or emulated RAM layout.

The bounded batch contains three mechanically decoded blocks:

| PC | translated body | exit | natural calls |
| --- | --- | --- | ---: |
| `0x2D66` | `MOVEM.L D7/A3,-(A7)`; `CLR.W D7`; `MOVE.B (A6)+,D7`; `LEA $FF134C,A3`; `ADDA.W D7,A3`; `MOVE.B (A6)+,D7`; `MOVE.W (A6)+,(A3)+` | `0x2D7A` | 1 |
| `0x604BC` | `LEA $FF0628,A6` | `0x604C2` | 4 |
| `0x61032` | `ADD.L D1,D2` | `0x61034` | 9 |

All are direct bounded entries selected from the existing deterministic
scenario. No VDP, Z80, I/O or interrupt occurred inside a replacement. The
long MOVEM block includes its GPGX dynamic per-register cycle addition and bus
refresh skip; no independent prefetch or timing implementation was added.

## Shadow result

The shadow run observed 14 natural entries and completed all 600 frames.
It performed 14/14 exact comparisons with zero divergences. Per-block results
were `0x2D66: 1/1`, `0x604BC: 4/4`, and `0x61032: 9/9`. Comparisons covered
all D0-D7, A0-A7, PC, SR/CCR, IR, prefetch address/data, cycles, refresh,
ordered RAM reads and ordered RAM writes. The original interpreter remained
authoritative; original body instruction starts inside shadow blocks were 20.

Shadow state checkpoint aggregate:
`fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`.
Shadow video sequence:
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

## Native override result

All three blocks were enabled simultaneously. There were 14 native override
calls, with per-block counts `1`, `4`, and `9`; fallback/emulated calls for
the registered set were zero, and native call share was `1.0`. The original
target body instruction-start count during overridden calls was zero, which
is the explicit proof that the CPU loop skipped original dispatch after the
native block hook returned success. Twenty translated guest instructions ran:
seven in `0x2D66` and one for each of the other calls.

The native run completed 600 frames with zero divergences, zero unexpected
hardware accesses and zero interrupts inside replacements. Its state
checkpoint aggregate and video sequence exactly matched the emulated and
shadow runs: `fffe59fcdbed7fdac8ef22badb4f7236b8619459fed27c9931e0f93906549052`
and `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

Generated summaries, checkpoints, call logs and video buffers remain ignored
local evidence. No ROM, asset, emulator binary or generated evidence blob is
tracked by Git.
