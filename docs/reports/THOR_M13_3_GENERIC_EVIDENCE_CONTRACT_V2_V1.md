# M13.3 — Generic Evidence Contract V2 — Partial Validation

**Status:** `PARTIAL_EVIDENCE_CONTRACT_V2`; M13.3 acceptance remains open.

## Scope and guardrails

The developer-only post-run adapter now emits
`oasis.m13.normalized-generic-corpus.v2`. It preserves native stream and
instruction identities, captured CPU IDs, bus width/domain/value/causal link,
M68K opcode comparison, next-PC/control-flow facts, ROM reads, calls, returns,
and indirect JMP targets where the encoded effective address supports that
classification. Mismatch and unresolved instructions remain in the corpus.
Generic closure consumes normalized memory, ROM-read, call and return facts;
the M12 A6 selector path remains only a generic regression fixture.

The host-audited Worker result contains exact entry/exit D0-D7, A0-A7, SR and
PC state. V2 stores these as bounded segment-boundary snapshots and links them
to the first/last instruction identities. They do not stand in for
per-instruction state. Missing values remain null and are counted as gaps.
No entity-specific semantics, `SOURCE_OWNED` mutation, production runtime
change, or raw-evidence reclamation is permitted by this checkpoint.

## Live validation

The retained run is under
`build/thor-evidence/m13.3-validation-v2-20260923/`. The canonical ROM identity
matched. The emulator executed 2,059 total frames; the existing natural-input
schedule switched to movement/gameplay inputs at frame 1,200 and injected them
for 859 frames. Active in-game state was not independently screen-verified.
The bounded Worker run ended with `STOPPED_FRAME_LIMIT` after 24 audited segments
and 27,609 FLOW records. Raw FLOW (1,326,384 bytes; SHA-256
`5b1ae942a54d99b488430b6de1ed73fc657d3f62b24e9c73f4ec9b353eaa4122`) and its
JSONL index (SHA-256
`dbf518916f87a850dffab946dc95a9d1e5ad2571e032e74cd0a5c5cb6d00dd69`) remain
present and hash-verified.

The first continuous attempt exposed that `--max-frames` only bounded per-round
waits, not total capture time; it reached 5,094 frames. Added explicit
`--max-total-frames`/`LF_MAX_TOTAL_FRAMES` termination, with a regression test,
then repeated the validation under the intended short bound. Both attempts'
raw evidence remain retained; only the bounded rerun is used for the metrics
below.

## Normalized coverage

| Evidence | Count | Coverage |
|---|---:|---:|
| Instruction events | 16,388 | — |
| M68K / Z80 instructions | 9,936 / 6,452 | — |
| Opcode identity classes | 9,936 `ROM_OPCODE_EXACT`; 6,452 `NON_ROM_DOMAIN` (Z80) | 100% classified |
| CPU ID / opcode / next-PC | 16,388 each | 100% each |
| Instruction frame identity | 0 | 0% |
| Instruction register reference | 48 | 0.293% |
| Memory events | 11,221 | — |
| Memory width / domain / instruction link | 11,221 each | 100% each |
| ROM reads | 24 | — |
| Bounded register snapshots | 48 | first/last boundaries only |

`FRAME_BOUNDARY_EVENTS = 0`: despite 2,059 emulated frames, this installed
native capture emitted no frame markers and reported zero entry/exit frame
counters. V2 therefore marks event frame identity unavailable instead of
assigning a fabricated frame. This blocks the full identity contract.

## Generic closure dry run

Two fresh closure executions were byte-identical (`deterministic_replay_status
= PASS`). Observed metrics:

- `GENERIC_FACTS = 1,086`
- `CFG_EDGES = 445`
- `REGISTER_DEF_EDGES = 0`
- `MEMORY_CAUSAL_EDGES = 573`
- `ROM_READ_EDGES = 6`
- `CALL_EDGES = 21`
- `RETURN_EDGES = 68`
- `INDIRECT_TARGETS = 4`
- `UNRESOLVED = 16,388`
- `SOURCE_OWNED_DELTA = 0`

Closure status is `NO_DELTA`; these are live generic facts, not ownership
promotion. Zero register-definition edges and the unresolved instruction count
reflect the missing per-instruction register state and decoder/operand
normalization. Zero indirect targets is a corpus-local result, not a global
unreachability claim.

## Verification and remaining gate

Focused normalizer, FLOW audit, launcher, generic-stage, generic-closure and
cleanup suites passed (35/35); Python compilation passed. Full CMake Debug and
Release builds passed with MinGW 15.2; six native Worker/sideband/scaling/Z80
tests passed in each configuration. Final source-file limit and diff checks
are recorded in `docs/WORKLOG.md`.

Acceptance is **not** passed. Remaining work: wire exact frame boundaries into
the installed native capture, provide bounded per-instruction register
snapshots/references and decoded M68K instruction widths/forms, then repeat a
short capture, provenance closure and deterministic replay. Keep both FLOW
files until that complete acceptance is recorded.
