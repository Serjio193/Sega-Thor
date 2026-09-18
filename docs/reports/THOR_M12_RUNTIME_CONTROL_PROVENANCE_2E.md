# M12 runtime control provenance 2E

**Capability result:** `PASS_RUNTIME_CONTROL_PROVENANCE_CAPABILITY_V1`

**Canonical Beyond Oasis witness:** `NONE`

**Parent checkpoint:** `f06256bb9e72ecc7fa14e9836ea7e007fc77a0ec`

2E adds bounded, post-run provenance over audited FLOW_V1 instruction
occurrences. It extends the existing AUTO67 predecessor path with continuous
interval validation and shared register-writer semantics, then reconstructs
only the registers needed by executed indirect consumers. It does not add a
CPU memory/instruction hook, a new transport, or CPU hot-path work. FLOW_V1,
Worker lifecycle/scaling behavior, production AUTO67, the accepted 2D map, and
ownership are unchanged.

The capability proof is from an isolated BizHawk/GPGX synthetic 68000 ROM. The
program executes a ROM longword pointer load followed by `JSR (A0)`, then reads
a signed ROM word, sign-extends it, adds the base register, and executes
`JMP (A0)`. Across 100 Worker cycles, each with a fresh capture ID and
generation, the run produced 1,000 executed indirect consumers (500 JSR and
500 JMP). The independent audit accepted 945 factual chains: 482 unmodified
ROM pointers and 463 transformed offsets. Thirteen unsupported-transform
cases and 42 unresolved-predecessor cases remained non-facts. No jump-table
entry was present in this micro-program; indexed selected-entry behavior is
covered by deterministic tests. The 945 evidence occurrences deduplicate to
two synthetic relations and two exact target ranges. Synthetic-ROM results
are capability evidence only and were not imported into the canonical map.

The two deduplicated synthetic facts are `ROM[$500,$504)` pointer to the exact
RTS instruction `ROM[$300,$302)`, and `ROM[$510,$512)` signed offset to the
exact JMP absolute-long instruction `ROM[$580,$586)`. These are synthetic ROM
ranges, not Beyond Oasis objects.

The separate natural Beyond Oasis capture used the accepted canonical ROM
(SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`),
the isolated `gpgx.wbx` (SHA-256
`4ac692a115cb5543bb3c2260fc04fdacd2cf5d963df59c17d87a12a30add3cd4`), one
Worker and 100 cycles. All 100 captures completed with fresh identities and
all four lifecycle counters at 100. The CPU stream advanced in every audited
round. The bounded replay contained **zero** indirect JMP/JSR consumers, so the
canonical runtime witness is explicitly `NONE`; it yields zero canonical
relations, not a claim that the entire game has no indirect transfers. The
accepted 2B run likewise had zero consumers in its bounded 199,630-instruction
trace. Static census results are only 671 `STATIC_CANDIDATE` encodings (113
JSR, 558 JMP), not runtime or code claims.

The independent auditor reconciled all 1,000 synthetic consumers and every
emitted fact against exact occurrence identity, instruction bytes/ranges,
source effective address and ROM bytes, register/transform chain, actual FLOW
`next_pc`, and exact target instruction bytes/range. It reported zero false
provenance detections, FLOW mismatches, and identity conflicts. Exact target
range decoding in this capability fixture is intentionally limited to its
known RTS and JMP-absolute-long targets. A future canonical consumer whose
target cannot be mapped by exact accepted decoder/map evidence must stop with
`STOP_TARGET_ROM_MAPPING_UNRESOLVED`; it will not create a guessed map object.

| Evidence set | Indirect consumers | Pointer facts | Offset facts | Used table entries | Independent facts |
| --- | ---: | ---: | ---: | ---: | ---: |
| Synthetic capability run | 1,000 | 482 | 463 | 0 | 945 |
| Canonical Beyond Oasis run | 0 | 0 | 0 | 0 | 0 |

The feature is post-run host analysis over the existing bounded segment. The
BizHawk Lua benchmark measured recorder-disabled vs recorder-enabled p50 at
17 ms vs 17 ms for both runs. During Worker execution p50 was 16 ms. Maximum
frame times were 17/19 ms with the recorder disabled and 19/19 ms with the
Worker active (micro/canonical respectively), so this 120-frame sample shows
no p50 slowdown but does not rule out tail-time variation. Each run completed
100/100 captures with zero invalid, dropped,
or ring-retention-failed captures. The existing CPU ring wrapped as expected
(1,039 times in the micro run and 996 in the canonical run); no new side ring
or side events were added, so side-ring overwrite count is not applicable.

The accepted 2D map was not modified because the canonical witness set is
empty. `SOURCE_OWNED` remains `1,475,600` bytes (`delta = 0`). No prediction,
unused table entry, or ownership promotion was made.

**Validation:** 22 focused 2E tests pass; 17 existing AUTO67.6 predecessor
tests pass; Python compilation, Debug/Release builds and full CTest, file-limit,
and diff checks are recorded in `docs/WORKLOG.md` after completion.

**Evidence retention:** raw FLOW files, micro ROM, campaign receipts, and
per-segment analysis/audit JSONL remain under ignored
`build/thor-evidence/runtime-control-provenance-2e-final-checked/`. They are
local evidence and are not part of this checkpoint's Git publication.
