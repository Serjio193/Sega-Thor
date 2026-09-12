# THOR Evidence Engine V0 — foundation and capability contract

Status: COMPLETE (V0 only). Mandatory STOP after V0 publication; V1 is not authorized.
BASELINE SHA: `b886d2507c9010caac2a75ccbb629b809399ffd0`.
FINAL SHA: recorded after the publication commit; CI receipt is recorded in the
final worklog entry.
SOURCE_OWNED baseline: 1,475,368 / 3,145,728 = 46.9006856283%; expected delta 0.

## Frozen implementation contract

This implements the minimal V0 subset of ADR-0044, not all proposed future tables.
Python sqlite3 is a non-owning sidecar. Carver's canonical JSON encoder is reused;
its evidence-list-dependent/truncated IDs and ownership methods are not reused.
No promoter, ROM classification write, V1 slicer, scheduler or campaign exists.

Identity schema `thor.evidence.identity.v0` uses full SHA256 with kind separation.
Environment includes canonical ROM, core/executable/config/collector/watch/map/
harness hashes. Scenario adds exact savestate and a complete input/window spec.
Trace is the ordered canonical header+events hash. Epoch increments on every
restore, independently of identical memory values. Event identity includes
trace, epoch and sequence; EXEC observation can identify an execution occurrence,
but does not assert completed instruction retirement.

Location is ROM+map+space+address/register, without value or time. Value identity
includes event, sample slot, location and bit slice, allowing D2[31:8] and D2[7:0]
and overlapping memory samples. These are observed value versions, not inferred
last-writer definitions. Byte decomposition/operations/derived dependencies are
deferred. Candidate temporal links are UNKNOWN and require strictly ordered
events in the same trace/epoch; restore relations are deliberately unsupported.

Structural relation identity excludes witnesses and confidence. Witnesses reference
exact events which sampled both endpoints; repeated import adds no duplicate
relation. UNKNOWN cannot be silently upgraded. Raw-source receipts retain physical
hash/path and trace identity outside canonical logical export. Copies/repeated
capture files are not automatically independent proof. No confidence voting.

Capture format is sealed JSONL with schema/environment/scenario header, contiguous
sequence, explicit EPOCH_BEGIN/END and count+ordered logical hash footer. Import
rejects malformed envelopes, wrong ROM, rewinds without epoch, unknown schema,
duplicate JSON keys, unclosed epochs and incorrect/missing seal. One SQLite
transaction publishes a capture; failure rolls back. Seal proves transport
integrity only; observations remain RAW/OBSERVED/UNKNOWN, never completeness.

Coverage certificates remain future typed claims over actor/address/window/hook
guarantees. A V0 raw read/write does not create a certificate. Frontier remains
a documented missing obligation with reason, scope and required capability;
no scheduling or automatic dependency expansion is implemented.

## Frozen experiments

Reuse the unchanged `build/bizhawk-controlled-harness/run.ps1 -LuaScript ...`.
New code has one bounded Lua collector with uninstrumented/minimal/probe modes.
All modes load exact QuickSave1, settle three neutral frames, set Right for one
frame and restore/repeat once to examine hook lifetime and epoch isolation.
No guest-memory/register modifications, ROM patches or new savestates.

Probe watches only selected instruction boundaries near already known A372/
A42A/A430/1FCA, eight bytes at FF13CC, four at FF188A and seven exact read
addresses. Duplicate-start and reversed-install controls test callback order.
At most 10,000 events/process; existing launcher timeout 30 seconds. Baseline
uses the historical original script. Two identical probe cold processes establish
logical determinism. Raw order is preserved, not sorted into agreement.

The uninstrumented mode means no execution/read/write/input-poll hooks; it still
loads inputs and samples boundary oracle values. All timing reports distinguish
the four-frame original baseline from eight-frame two-epoch probe modes.

## Results

The V0 implementation is bounded, sealed and fail-closed. The local machine
readable matrix is `build/thor-evidence/v0/capability_matrix.json` (ignored;
payload SHA256 `e57dce403ba54b8938c8f3c48b435e6f6c146c83a6bb6d105df4d897e9602daf`).
Its canonical ROM identity is
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` and the
QuickSave1 identity is
`7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970`.

| Contract | V0 result | Boundary |
|---|---|---|
| EXEC phase | PROVEN: BizHawk documents immediate-before-execution; watched PC matched the callback PC, including A372 | selected M68K sites only |
| WRITE phase | PROVEN as immediate-before-write API; A372-associated FF13CC callback observed PC A374 | no universal PC-2 rule |
| READ phase | PROVEN as immediate-before-read API; selected callback PCs captured | complete pairing is not generalized |
| Access width | UNKNOWN | live GPGX flags carried access bits only; no size bits |
| Overlap/range | UNKNOWN | exact-start hooks do not prove a long overlapping access |
| Same-value writer | UNKNOWN | repeated callbacks lack an independent pre-write same-value oracle |
| Hook order | PROVEN | duplicate FF13CC callbacks follow installation order; reversed control changed order |
| Restore/epoch | PROVEN in bounded setup | hooks survived same-process restore; epochs 1 and 2 are distinct |
| Peek re-entry | OBSERVED | zero matching re-entries for selected `read_bytes_as_array` peeks |
| IRQ/exception | UNKNOWN | outside the V0 budget |
| Input poll | PROVEN negative result | zero poll callbacks; Right installation is not causal game-read proof |

The minimal, probe and reversed-install captures all reached the non-zero oracle
`SAT=0088090187810088`, `fields=00100080`; instrumented modes observed six
execution hits and one FF13CC write per epoch. The uninstrumented control saw
the same oracle with zero execution/write callbacks. The two cold probe runs
were byte-identical at the raw and logical-event-stream levels. Their trace IDs
intentionally differ because each run has its own environment/config identity;
the reversed-install event stream differs as the control requires. All measured
input frames were lagged and had zero input-poll callbacks.

The sealed artifacts contain 68 events (minimal), 378 events (each probe) and
12 events (uninstrumented). Process wall times were 3.29–4.18 seconds, below
the 30-second bound. `SOURCE_OWNED` stayed at 1,475,368 bytes (delta 0).

## Validation and publication

`tests/thor_evidence_v0_test.py` passed all three synthetic storage/transport
tests, including duplicate import idempotence, cross-ROM rejection, epoch
rewind rejection, malformed seal rejection, UNKNOWN relation guards and float
identity rejection. A normalized BizHawk capture imported into SQLite with the
same transaction path. Debug, Release and GNU/Linux-equivalent CMake/CTest
checks, `git diff --check` and the repository source-size check are required
before publication; their exact results, commit SHA, remote SHA and CI run are
recorded in `docs/WORKLOG.md`.

## Files and exclusions

The tracked V0 surface is the small Python identity/event/normalization/report/
store package, its SQLite schema, one developer-only Lua collector, the focused
test and CTest registration. Runtime captures, ROMs, savestates, emulator
installations and BizHawk source snapshots remain ignored/local. No promoter,
ownership mutation, scheduler, V1 causal slicer, M13 work or production C++ was
added.

## V1 blockers and STOP

V1 remains blocked on a measured width/range contract, a complete writer-value
oracle, bounded IRQ/exception coverage and causal input-read evidence. These are
explicit UNKNOWN/OBSERVED entries, not inferred behavior. Architectural
deviations from ADR-0044: none. Decision quality: V0 proves transport,
identity, temporal ordering, callback phase and selected installation/restore
properties while preserving uncertainty and `SOURCE_OWNED`; it does not claim
causal provenance. STOP here until a new milestone is explicitly authorized.
