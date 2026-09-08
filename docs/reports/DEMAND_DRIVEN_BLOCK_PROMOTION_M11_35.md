# M11.35 — Demand-Driven Block Promotion Pilot

Status: `DEMAND_DRIVEN_PROMOTION_RUNTIME_BLOCKED`

Baseline: M11.34 commit `32708bdcec086c6e954acdbb53715e4e3293fd2e`.
Scenario: cold reset, neutral input, 600 frames.
Canonical ROM: local USA `Beyond Oasis (USA).md`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
GPGX source/build identity: instrumented checkout commit
`d60d079934977aa6973e220d123533387159f66e`; DLL SHA-256
`ba6c10fdb1fe421e1e38c88b70ce18b0d2a122f4472f477b8a1a7aba14fce274`.

## Boundaries

This was one bounded developer-only promotion pilot. It did not add a general
block-ranking framework, runtime JIT, automatic trust, production emulator
dependency, whole-game coverage, M12 work, ID3 work, `0x3820` work, timing
optimization or toolchain archaeology. The M11.33 generated bodies remained
separate from handwritten hybrid glue and remain the only previously proven
native-promoted bodies.

## Phase results

### Natural discovery

`DISCOVER_BLOCKS` recorded 2,188 unique guest PCs during the existing 600-frame
scenario. The final bounded shortlist contained three entries outside the
M11.33 blocks:

| entry | exact bytes and form | direct exit |
| --- | --- | --- |
| `0x3A85E` | `4A79 00FF 1654` — `TST.W ($00FF1654).L` | `0x3A864` |
| `0x3A8BA` | `4A79 00FF 1654` — `TST.W ($00FF1654).L` | `0x3A8C0` |
| `0x3A88C` | `4A39 00FF 0BFD` — `TST.B ($00FF0BFD).L` | `0x3A892` |

Bcc and DBF alternatives were inspected and rejected at the bounded
timing/interrupt boundary. `0x3A7AE` was rejected after an IR/prefetch
mismatch. Wider candidates with indirect control flow or unsupported exact IR
were not promoted or expanded.

### Exact IR and independent semantics

The selected forms were classified as `NEW_VERIFICATION_REQUIRED`. The
test-only independent model added deterministic TST edge vectors for byte and
word width, zero, negative and positive results, while the bounded Bcc/DBcc
vectors recorded the branch/loop timing cases considered during selection.
The semantic executable passed. The generator emitted only exact supported
TST helper calls with guest PC/opcode/assembly provenance and direct successors;
unsupported forms remain fail-closed.

The independent semantic reference was transcribed from the Motorola/NXP
68000 Programmer's Reference Manual:
<https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf>.

### Runtime shadow gate

The current code was rebuilt and the candidate set was run in GPGX
`BASIC_BLOCK_SHADOW` mode. The first comparison stopped at:

```text
FIRST_DIVERGENCE block=0x3a85e timing actual_cycles=74 expected_cycles=896114
actual_refresh=228 expected_refresh=896268
```

The divergence is in the GPGX cycle/refresh and interrupt-boundary contract, so
the required exact CPU/RAM/PC/SR, prefetch/cycle/refresh and hardware equivalence
was not established. Later candidates were not treated as proven.
`BASIC_BLOCK_NATIVE` was intentionally not run after the failed shadow gate.

## Final gate

No M11.35 candidate was promoted. The failed candidates remain interpreter
fallback, while the M11.33 native-promoted blocks remain preserved. The result
is a runtime blocker, not a semantic-core failure and not a claim of whole-game
coverage.

`game.srm` remains untouched and untracked. ROMs, extracted assets, emulator
binaries, generated run evidence and other local artifacts are not repository
content.

Exact next step: stop and resolve the cycle/refresh and interrupt-boundary
bridge contract in a separately bounded milestone before another promotion
attempt.
