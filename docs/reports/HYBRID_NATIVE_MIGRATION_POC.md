# M11.28 — Hybrid native migration PoC

Decision: **`HYBRID_SHADOW_PROVEN_OVERRIDE_BLOCKED`**.

This proves a developer-only natural-call SHADOW path for `0x3820`, within the
explicit observable contract below. It does **not** prove full CPU equivalence
or enable native override. M11.27 manual ID3 hunting is stopped.

## Identity and reproducible inputs

- Sega-Thor starting commit: `bfcaf9b0a4a813686d055f7590d94f2f411adf4a`.
- Canonical USA ROM: 3,145,728 bytes; CRC32 `C4728225`;
  SHA-1 `2944910c07c02eace98c17d78d07bef7859d386a`;
  SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- External GPGX base commit: `d60d079934977aa6973e220d123533387159f66e`,
  `C:/Github/Genesis-Plus-GX-instrumented`.
- This is a **modified build**, not the pristine base commit. Changes are the
  read-only `gpgx_bridge.c` supplied here and one opt-out guard in the external
  coverage initializer. No CPU loop, instruction implementation or timing code
  was changed.
- Bridge SHA-256: `4968382cf8c6a9db7333218256d08051bf0d6cc3a1d66c7d09d6f550780ce735`.
- Modified external `core/debug/coverage.c` SHA-256:
  `4bb1f5532dd0a0c21dba754adf09ec19a40713a9950b30ac2190a0c3d60267a3`.
- GPGX DLL SHA-256: `8a227d8dcd357be26e17a8f05c6fcf9d0b6604a0f25426e32800f5c8dcc547f5`.
- Build: MSYS2 UCRT64 GCC 16.2.0,
  `make -f Makefile.libretro platform=win HOOK_CPU=1 -j4`.
- Scenario: existing neutral startup input schedule, cold reset, 600 calls to
  `retro_run`, both controller ports always zero. No savestate seed, injected
  PC/registers, manual input or gameplay search. The small frontend supplies
  deterministic libretro callbacks and hashes video; no GUI is required.

## Accepted evidence

| Run | Natural calls | Shadow comparisons | Divergences | Frames completed |
|---|---:|---:|---:|---:|
| EMULATED, Debug frontend | 6 | 0 | 0 | 600 |
| SHADOW_NATIVE, Debug frontend | 6 | 6 | 0 | 600 |
| SHADOW_NATIVE, repeated Debug cold reset | 6 | 6 | 0 | 600 |
| SHADOW_NATIVE, Release frontend | 6 | 6 | 0 | 600 |

The bounded corpus is **six distinct natural inputs**, not 18 distinct cases.
Across three shadow runs there are 18 clean comparisons. Each shadow run
observed 112,361 instruction-start hooks inside the original routine and six
external interrupts during calls. No instruction trace is retained.

| Call | Source | Format | Entry/return frame (zero-based) | Consumed bytes | Output bytes |
|---|---|---|---|---:|---:|
| 1 | `0x150000` | B | 115 / 116 | 979 | 1638 |
| 2 | `0x15457A` | B | 360 / 360 | 77 | 128 |
| 3 | `0x1545C8` | B | 361 / 361 | 527 | 1280 |
| 4 | `0x171A62` | A | 377 / 378 | 1798 | 6304 |
| 5 | `0x170000` | B | 379 / 382 | 2363 | 5958 |
| 6 | `0x16943C` | A | 383 / 384 | 1217 | 3072 |

All 10 serialized GPGX checkpoints (every 60 frames) are identical between
EMULATED and every accepted shadow run. All 600 video callbacks are also
identical. Aggregate checkpoint hash:
`f6bf473bbd6b6ff039139c963504853ca079deef09c68f4515d8920ca6d137ab`.
Aggregate video-sequence hash:
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.

The three shadow call logs are byte-identical, SHA-256
`dfd6fa0e192259918f8d8950e950d30c8a5a865705e38b193ec0cdb92991fbb8`.
The checkpoint JSONL is also identical, SHA-256
`d7492f8b8d109c19de85505c9afcd7aaeb988f6b15b40d04396ec667510d1ece`.
Local evidence is under `build/m1128/{emulated-debug,shadow-debug-d,
shadow-debug-repeat,shadow-release}/`: `summary.json`, `calls.jsonl`,
`checkpoints.jsonl`. These contain hashes and register metadata, not extracted
graphics or full RAM dumps.

## Bounded observable contract

Entry capture includes D0–D7, A0–A7, full SR (including CCR.X), PC, exact
compressed source bytes, exact output destination footprint and the 16-byte
(A) or 28-byte (B) saved-register stack footprint plus the 4-byte return PC.
ROM identity is checked before loading GPGX; captured source bytes must also
match the canonical ROM. A read-only native preflight discovers the capture
lengths. Both existing C++ decompression implementations are independently run
again at return, against the preserved source copy.

The comparison independently checks:

- D0–D7 and all preserved address registers, including full upper halves;
- A0 advance equals consumed source bytes; A1 advance equals output size;
- A7 advance is 4 and return PC is the captured stack return address;
- full SR **except X**, with explicit comparison mask `0xFFEF`;
- byte-exact native/mechanical/original output, source consumption, sequential
  output writes and final destination memory;
- saved-register stack effects, final stack bytes and unchanged return slot.
  GPGX's `MOVEM` predecrement bus writes low word before high word, independently
  confirmed in `core/m68k/m68kops.h:m68k_op_movem_32_re_pd`;
- all routine data reads stay in the captured source, already written output
  or stack footprint; all routine writes stay in output or saved stack.

The wrapper's register preservation comes from exact canonical instructions:
`48E7 E020` at `0x3820`, the additional `48E7 1300` at `0x38D0`, matching
restores at `0x38CA` or `0x3A1C/0x3A20`, and RTS at `0x38CE` or `0x3A24`.
Final `MOVE.B` of the zero terminator sets Z and clears N/V/C; MOVEM/RTS do not
change those flags. X is captured and reported, **not predicted or compared**.
Every observed return had SR `0x2104`; this observation is not a general X
contract.

Interrupt entry is recognized at GPGX's autovector read, before its stack
frame writes. External ISR execution remains emulated. On resume, the
suspended D/A registers, PC, SP and full CCR must match; ISR writes may not
overlap the routine's required footprint. Other ISR effects are deliberately
not translated. This is isolation of the routine's observable effects, not an
implementation of interrupt semantics.

## Exact override blockers

1. **CCR.X:** neither existing decompressor returns the final X bit. Its value
   must be derived independently for both stream formats, including every
   relevant arithmetic/shift path; sampling zero at six returns is insufficient.
2. **Timing and interrupt ordering:** the first natural call receives the
   VBlank autovector read at `0x78` while executing the routine. Six interrupts
   occur across the six calls. Replacing the entire body atomically would
   change when those handlers and their device/RAM effects occur. A contract
   is missing for instruction-dependent cycle consumption, GPGX refresh
   adjustments (`MOVEM` also uses `SKIP_BUS_REFRESH`), partial progress across
   `m68k_run` budgets, and interrupt delivery/resumption. Total elapsed cycles
   learned from a prior run do not establish this contract.
3. **CPU continuation:** this GPGX build enables `M68K_EMULATE_PREFETCH`.
   No override adapter has proven the required prefetch/IR state and return
   transition at the pre-fetch hook boundary.

`NATIVE_OVERRIDE` is rejected before loading the library or creating an output
directory. The bridge exports no register/memory mutation API.

| Requested override evidence | Result |
|---|---|
| Native override calls | **0** |
| Original `0x3820` body skipped | **false; no skip attempted** |
| Scenario completed after override | **not run** |
| Full CPU/output state equivalence under override | **not proven** |

## Validation and scope

Windows GCC Debug and Release: relevant hybrid targets, native graphics
tests/reference and mechanical translation tests build and pass. Each focused
CTest selection passes 5/5, including source-file limits. Canonical-ROM format
A/B reference checks pass in both configurations. Negative synthetic tests
detect output/register corruption, out-of-footprint writes, ISR interference,
incomplete calls, and fail-closed override rejection. The real override CLI
negative check returns failure without creating a scenario directory.

GNU/Linux Release compilation/linking of the hybrid frontend and both hybrid
tests passes under WSL Ubuntu 24.04. Both hybrid tests and the existing graphics
and mechanical translation tests pass. The Linux emulator itself was not built
or executed; live ROM/GPGX comparisons above use the Windows DLL.
The separate Linux source-limit glob over the mounted Windows checkout was
stopped after more than six minutes without completing; it is not counted as
passing. The identical project checker passed on Windows. The final Linux
functional CTest selection is 4/4; Windows selections are 5/5.

Preliminary development runs stopped fail-closed on the initially unsupported
interrupt boundary and on the initially incorrect MOVEM bus-write order. Their
incomplete logs remain in `shadow-debug-a/b/c`; they are excluded from accepted
evidence. No decoder behavior was changed to obtain passing comparisons.

Production sources and production link dependencies are unchanged. All new
code lives under developer tooling/tests. No ROM, asset, CPU emulator, external
binary or execution capture is added to Git.

Publication closure requested before M11.29: fresh Windows Debug/Release
focused validation passes 5/5 each. A source-only copy on WSL's native
filesystem passes the full GNU/Linux Debug build and 47/47 CTests, including
the source-limit checker. This resolves the earlier NTFS scan limitation.
The focused source/tests/report change is ready for the requested commit/push;
remote main and exact-commit CI are checked after publication.

## Reproduction

From Sega-Thor, run `python src/tools/hybrid/prepare_gpgx.py
C:/Github/Genesis-Plus-GX-instrumented`, then build that external tree with the
GPGX make command above. The preparation script installs only the bridge and
the coverage opt-out guard; the latter takes effect only when the hybrid
frontend sets `GPGX_HYBRID_ONLY`.

Configure CMake with `-DOASIS_HYBRID_GPGX_SOURCE=C:/Github/Genesis-Plus-GX-instrumented`
and build `oasis_hybrid_poc`, `oasis_hybrid_contract_test`, and
`oasis_hybrid_dispatch_test`. The executable is in `src/tools/hybrid` under
the build directory. Supply a compiler-runtime PATH as for existing MinGW tools.

```text
oasis_hybrid_poc <GPGX DLL> <canonical ROM> EMULATED 600 <baseline-directory>
oasis_hybrid_poc <GPGX DLL> <canonical ROM> SHADOW_NATIVE 600 <shadow-directory>
oasis_hybrid_poc <GPGX DLL> <canonical ROM> SHADOW_NATIVE 600 <repeat-directory>
```

Compare identities, positive call/comparison counts, zero divergences,
completed frames, both aggregate hashes and exact call/checkpoint JSONL across
runs. Do not promote an incomplete run or a matching output hash to full CPU
equivalence. The bounded experiment ends here. **STOP.**
