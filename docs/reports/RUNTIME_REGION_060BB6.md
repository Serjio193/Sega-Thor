# Bounded Runtime Region Investigation: `0x060BB6-0x060BC4`

Decision: `RUNTIME_REGION_060BB6_STRUCTURALLY_UNDERSTOOD`

Recommended next action: **B. classify bounded region with static support**.
That action is not performed by this report.

## Target and evidence

Canonical USA ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The investigation used the retained M11.19 manual-realtime capture, the exact
decoder output in `build/m11-20-global.json`, the bounded decoder output in
`build/m11-21-window.json`, the M11.20 structural explorer output, and the
existing candidate map. No new runtime capture, replay or ranking change was
performed.

The requested initial window was `0x060B80-0x060BF0`. Two boundary extensions
were necessary and concrete:

- `0x060B7C` is an 8-byte instruction whose bytes cross the lower window edge;
- `0x060BEE` is a 6-byte instruction whose bytes cross the upper window edge.

The effective listing below therefore covers `0x060B7C-0x060BF4` only to
establish those boundaries. No wider exploration was needed.

### Runtime PCs in the primary region

Exactly eight instruction-start PCs were observed:

`0x060BB6`, `0x060BB8`, `0x060BBA`, `0x060BBC`, `0x060BBE`, `0x060BC0`,
`0x060BC2`, `0x060BC4`.

The target has 8/8 decoded instruction starts observed (100%). No target
instruction start remains unobserved. Per-PC hit counts are unavailable: the
retained bitmap records presence, not execution frequency.

## Decoded listing

The exact decoder reports all rows below as decoded. `0x060BEE` appeared as
unsupported in the truncated bounded-slice text only because its final bytes
were outside the requested end; the full exact report decodes the complete
instruction as shown here.

| Address | Raw bytes | Instruction | Length | Flow | Runtime observed |
|---|---|---|---:|---|---|
| `0x060B7C` | `13FC008300A00003` | `move.b #$83,($00A00003).L` | 8 | fallthrough | no |
| `0x060B84` | `33FC000000A11100` | `move.w #$0,($00A11100).L` | 8 | fallthrough | yes |
| `0x060B8C` | `6100068C` | `bsr.w loc_06121A` | 4 | direct call | yes |
| `0x060B90` | `33FC010000A11100` | `move.w #$100,($00A11100).L` | 8 | fallthrough | yes |
| `0x060B98` | `0839000000A11100` | `btst.b #$0,($00A11100).L` | 8 | fallthrough | yes |
| `0x060BA0` | `6600FFF6` | `bne.w loc_060B98` | 4 | direct branch | yes |
| `0x060BA4` | `4A3900A00003` | `tst.b ($00A00003).L` | 6 | fallthrough | yes |
| `0x060BAA` | `67000018` | `beq.w loc_060BC4` | 4 | direct branch | yes |
| `0x060BAE` | `33FC000000A11100` | `move.w #$0,($00A11100).L` | 8 | fallthrough | yes |
| `0x060BB6` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BB8` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BBA` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BBC` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BBE` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BC0` | `4E71` | `nop` | 2 | fallthrough | yes |
| `0x060BC2` | `60CC` | `bra.s loc_060B90` | 2 | direct branch | yes |
| `0x060BC4` | `33FC000000A11100` | `move.w #$0,($00A11100).L` | 8 | fallthrough | yes |
| `0x060BCC` | `6100F8EE` | `bsr.w loc_0604BC` | 4 | direct call | yes |
| `0x060BD0` | `205F` | `movea.l (A7)+,A0` | 2 | fallthrough | yes |
| `0x060BD2` | `51F900FF0012` | `sf.b ($00FF0012).L` | 6 | fallthrough | yes |
| `0x060BD8` | `51ED0000` | `sf.b 0(A5)` | 4 | fallthrough | yes |
| `0x060BDC` | `51F900FF0010` | `sf.b ($00FF0010).L` | 6 | fallthrough | yes |
| `0x060BE2` | `51F900FF0011` | `sf.b ($00FF0011).L` | 6 | fallthrough | yes |
| `0x060BE8` | `51F900FF0013` | `sf.b ($00FF0013).L` | 6 | fallthrough | yes |
| `0x060BEE` | `51F900FF0014` | `sf.b ($00FF0014).L` | 6 | fallthrough | yes |

No `RTS`, `RTE`, `JMP` or indirect control-flow instruction occurs in the
primary region. `BSR` is present in the surrounding code at `0x060B8C` and
`0x060BCC`; the local listing does not contain their callees' returns.

## Direct bounded CFG

Exact direct edges, deduplicated from the decoder/explorer evidence:

| Source | Target | Edge type | Target runtime observed |
|---|---|---|---|
| `0x060B8C` | `0x06121A` | direct call | yes in global capture |
| `0x060BA0` | `0x060B98` | conditional branch | yes |
| `0x060BAA` | `0x060BC4` | conditional branch | yes |
| `0x060BC2` | `0x060B90` | direct branch | yes |
| `0x060BCC` | `0x0604BC` | direct call | yes in global capture |

Relevant sequential edges are `0x060BAE -> 0x060BB6` (entry into the target)
and `0x060BC4 -> 0x060BCC` (continuation after the target). Inside the target,
`0x060BC2 -> 0x060B90` is a loop-back edge. The branch at `0x060BAA` enters
the target's final instruction at `0x060BC4` without executing the NOP run.

## Incoming provenance

| Source | Target | Type | Source evidence |
|---|---|---|---|
| `0x060BAE` | `0x060BB6` | fallthrough | exact decode and explorer `STATIC_PROVEN`; runtime address classification remains `UNKNOWN`; enclosing candidate `0x06042A` is `STATIC_SUPPORTED` but does not prove a boundary |
| `0x060BAA` | `0x060BC4` | conditional branch | exact decoder direct branch and explorer `STATIC_PROVEN`; runtime address classification remains `UNKNOWN`; same enclosing-candidate limitation |

No direct external call into `0x060BB6` was found in the bounded evidence. The
natural entry is a fallthrough from `0x060BAE`; the alternate entry reaches
`0x060BC4` from `0x060BAA`.

## Outgoing provenance

The primary region has one direct control-flow exit: `0x060BC2 -> 0x060B90`,
which is a local loop-back. Its final instruction `0x060BC4` falls through to
`0x060BCC`. The immediate successor then calls `0x0604BC`.

Existing project evidence for `0x0604BC` is stronger than the target's
address-level runtime classification: the candidate map marks it `CONFIRMED`,
with Ghidra range `0x0604BC-0x0604E6`, a known direct call target, and no
code/data conflict. The call site `0x060BCC` is independently marked
`STATIC_SUPPORTED`. This corroborates the local outgoing path but does not
promote the whole `0x060BB6-0x060BC4` range.

## Runtime/static reconciliation

| Target instruction | Decoded | Executed | Branch/callee evidence |
|---|---|---|---|
| `0x060BB6` | yes | yes | sequential entry from observed `0x060BAE` |
| `0x060BB8` | yes | yes | fallthrough |
| `0x060BBA` | yes | yes | fallthrough |
| `0x060BBC` | yes | yes | fallthrough |
| `0x060BBE` | yes | yes | fallthrough |
| `0x060BC0` | yes | yes | fallthrough |
| `0x060BC2` | yes | yes | branch target `0x060B90` observed |
| `0x060BC4` | yes | yes | reached by `0x060BAA` and by NOP-run fallthrough |

Thus the manual gameplay capture proved natural execution of every target
address, but it did not record branch condition values, register state, or
per-PC hit counts. The surrounding `0x060BEE` instruction was also observed
and is fully decoded only after extending the static read to its complete
length.

## Boundary assessment

`LIKELY_INTERNAL_BLOCK` for the bounded target fragment; a whole routine
boundary is not established. The label means “internal bounded fragment”, not
that `0x060BB6-0x060BC4` is itself a natural compiler/basic-block boundary.

Evidence:

- the NOP run is entered by fallthrough from `0x060BAE`;
- the primary range starts inside that straight-line sequence and ends at the
  next sequential instruction `0x060BC4`;
- `0x060BC2` loops back to `0x060B90`, so the target is embedded in a larger
  control-flow loop rather than terminated by a return;
- `0x060BAA` provides a second entry to the final instruction only;
- the next instruction `0x060BCC` performs a direct call to a separately
  corroborated target.

This is a structural label only. It is not a function name and does not claim
that the surrounding broad Ghidra range `0x06042A-0x0611E0` is one routine.

## Comparison with rank #1: `0x000374-0x0003A0`

The M11.20 rank-1 region has 23/23 observed and decoded PCs, score 99,
static support `NONE`, three incoming xrefs, five outgoing edges, Ghidra and
candidate overlap at `0x00020E`, and a `DBF` terminator. Its low-address
context and lack of gameplay-specific corroboration make it startup/system-
like evidence for this comparison, not proven gameplay code. This is a
context classification, not a semantic function name.

The target has lower raw density contribution (8 PCs, score 80) but stronger
bounded corroboration: exact local CFG, explorer `STATIC_PROVEN` edges,
candidate/Ghidra overlap through the existing `0x06042A` evidence, and the
known `0x060BCC -> 0x0604BC` path. The ranking therefore correctly reflects
its transparent scoring inputs; it is not a gameplay-value ranking.

## Trust assessment and unknowns

Sufficient evidence exists for eight independent
`CODE_EXECUTED_AT_ADDRESS` facts at the target PCs. It is not sufficient to
assign `CODE_STATIC_SUPPORTED` to the entire target range automatically.
The adjacent `0x060BCC` and callee `0x0604BC` retain their existing project
classifications; no classification was changed here.

Remaining unknowns:

- per-PC hit counts and branch-condition outcomes;
- register/flag state at `0x060BAA`, `0x060BC2` and both `BSR` sites;
- exact routine boundary around the broad `0x06042A` candidate;
- repository-wide incoming-edge enumeration was intentionally not performed;
  only the concrete bounded-window incoming edges are claimed here;
- semantics of the hardware/RAM writes in this control path;
- whether the unobserved `0x060B7C` is part of the same natural entry path in
  another state.

No new deterministic translation or classification logic was added, so no
new tests were required.

## Verification artifacts

- `build/gpgx_runtime_execution_evidence.json` — retained M11.19 capture
- `build/m11-20-global.json` — exact decoder/runtime reconciliation source
- `build/m11-20-explore.json` — bounded structural explorer evidence
- `build/candidate-map-a.json` — existing static/candidate provenance
- `build/m11-21-window.json` and `.txt` — bounded decoder invocation

No ROM, savestate, extracted asset or generated commercial data was added to
the repository.
