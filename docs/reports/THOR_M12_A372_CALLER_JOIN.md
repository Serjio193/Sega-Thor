# M12 static scheduler-to-shadow-SAT caller join

Baseline: `origin/main = 199e92e`.

Canonical ROM: `0x300000` bytes, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

`SOURCE_OWNED` is unchanged at `1,475,368 / 3,145,728 = 46.9006856283%`.
This report and its analyzer emit no ROM payload, savestate, capture, decoded
asset, or sprite pixels.

## Result

The new static join closes the caller path from the main scheduler to the
already proven `0xA372` shadow-SAT writer:

`0x008B22` scheduler
→ `0x008B42 -> 0x008E90`
→ `0x008B86 -> 0x00A196`
→ `0x00A19C -> 0x00A342..0x00A438`
→ `0x00A372 -> 0x00FF13CC`
→ DMA `0x000027EC`
→ SAT VRAM `0x0000D000`.

The same `0x008B22` slice also calls player update `0x00557A` at `0x008B2E`
before the movement and scheduler calls. Inside `0x00A196`, the exact prefix
is `SF.B ($00FF1651).L` at `0x00A196`, `BSR 0x00A342` at `0x00A19C`, and
`TST.B ($00FF1996).L` at `0x00A1A0`. This proves that `A342` is an enclosing
producer in the scheduler caller path, without assigning the produced records
an object, animation, frame, or piece name.

## Root-selector cross-subsystem edge

The classified `0x03C5B6..0x03C75E` caller contains the exact sequence:

| PC | operation | target/state |
| --- | --- | --- |
| `0x03C6E4` | `ST.B` | `0x00FF1858` |
| `0x03C6EA` | `JSR abs.l` | `0x008E90` |
| `0x03C6F0` | `JSR abs.l` | `0x00A196` |

Because `A196` reaches `A342`, this is a static control edge from the local
boolean `FF1858` root selector into the movement/scheduler route that produces
shadow SAT records. The selector's boolean behavior remains the only proven
meaning: zero selects root `0xA438`, nonzero selects root `0xA480`. No higher
semantic label is inferred.

The neighboring classified range `0x03C262..0x03C454` writes the same byte at
`0x03C328`, but has no opcode-level direct call to `A196` in that bounded range.
This is useful negative evidence against merging the two caller paths. Local
indirect or nested effects remain unresolved, so this absence is not a global
reachability claim.

The separate `A6A0` family is also confirmed: direct callers `0x0428A` and
`0x04A5A` reach `A6A0`, and `A6A0` calls the same `A342` producer. The two
families share the producer entry but are not merged semantically.

## Investigation cycles and decisions

### Cycle 1 — caller closure from A372

Question: which exact callers feed the closed `A342..A438` producer?

Method: bounded static consumer backtrace using exact 68000 direct-call
contracts and the existing raw-ROM slice decoder.

Result: `A19C` and `A6A0` are exact direct callers; `A196` contains `A19C`,
and the main scheduler contains `A196`. `A6A0` has the separate direct callers
`0x0428A` and `0x04A5A`.

Negative result: no semantic record role follows from these calls alone.

Decision: continue with a materially different cross-subsystem selector pass.

### Cycle 2 — FF1858 writer context

Question: does a proven `FF1858` writer occur on the scheduler path?

Method: bounded static writer-to-consumer join over the two classified code
ranges containing `0x03C328` and `0x03C6E4`.

Result: `0x03C6E4` sets `FF1858` immediately before calls to `0x008E90` and
`0x00A196`, which reaches `A342/A372`.

Negative result: `0x03C328` has no direct `A196` call in its bounded range;
its indirect/local continuation is intentionally left open.

Decision: perform one independent main-loop order check, then stop at the
strategic runtime-versus-static fork.

### Cycle 3 — independent main-loop order check

Question: is the scheduler join ordered after movement in the exact main loop?

Method: independent decode of `0x008B22..0x008C42`.

Result: direct calls at `0x008B2E`, `0x008B42`, and `0x008B86` establish the
order `0x00557A -> 0x008E90 -> 0x00A196`.

Negative result: no frame cursor or resource pointer is exposed by this
caller-only slice.

Decision: stop this static join session; further progress requires a new
evidence class rather than another caller-only replay.

## Proven graph and limits

New proven structure: `FF1858` writer context → main movement/scheduler
sequence → `A196` → `A342` → finite root records → `A372` → shadow SAT DMA.

The frame/object/animation/piece grammar remains **NOT PROVEN**. The existing
selector/descriptor → child/relative → `0x03B448` → `0xB730` → SAT graph is
still a separate proven branch; this session does not claim that it shares the
same root or record grammar.

No SOURCE_OWNED promotion is justified. The analyzer is
`src/tools/m12_a372_caller_join.py`, with regression coverage in
`tests/m12_a372_caller_join_test.py` and CTest registration in
`cmake/m12_auto2.cmake`.

## Validation

The focused Python regression, payload-free analyzer generation, and
`py_compile` passed. Windows Debug and Release each passed the full CTest
suite at `162/162`, including the source-file line-limit check. The
GNU/Linux-equivalent Release configure/build/link passed, followed by the two
targeted M12 producer/join tests at `2/2`. `git diff --check` and canonical ROM
size/hash identity checks passed. No ROM bytes, assets, savestates, or decoded
payloads were added.

## Best next directions (superseded by controlled root capture)

1. Targeted runtime register/SAT capture at `A196 -> A342 -> A372`, recording
   `FF1858`, `FF188C`, root choice, and post-DMA publication. Highest expected
   information gain because it can connect the neutral root records to an
   observed scheduler transition without guessing semantics.
2. Static consumer closure for `FF188C`, `FF188A`, and `FF1858` outside the
   already classified ranges. Medium gain; may close producer-family state
   transitions but is likely to retain indirect callers.
3. A separate consumer-boundary investigation for the unresolved `0x03BDA6`
   stream. Lower immediate gain for the A372 branch, but it is the strongest
   remaining static edge in the selector/descriptor branch.

The first runtime direction was executed after the verified harness was
restored. Its Right-versus-neutral control reached the same A196/A342/A372
events and FF13CC bytes, with FF188A/FF188C ending at 0010/0080. Right
causality is therefore not proven. The updated ranking is recorded in
`THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md`: static FF1858/FF188A/FF188C
consumer closure is first, the unresolved 0x03BDA6 boundary is second, and
another input-causality replay is blocked until a separately validated
input-polling state exists.

AUTONOMOUS SESSION STOP REASON: the controlled runtime fork produced bounded
register values and a negative causal control. Continuing with another
equivalent replay would violate the two-pass rule; the next session should
take the first-ranked static consumer-closure fork.
