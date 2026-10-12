# M68K `0x9D70` instruction-step replay — 2026-10-13

## Result

The scheduler-capable instruction-step primitive was applied to one natural
`0x009144 BSR.W → 0x009D70 → RTS → 0x009148` call and a full 3,600-frame
replay. The replacement was diagnostic-only and selected exactly one call.

```text
SCHEDULER_MICRO_FIXTURE = PASS (10/10; see THOR_SCHEDULER_FIXTURE_20261013)
NATURAL_CALL = PASS (frame 2655; one stepped call)
NATURAL_CALL_TIMING = MATCH (420 helper cycles; 546 from caller instruction)
FULL_REPLAY_OBSERVABLE_PARITY = PASS (3600 frames; defined surface below)
STRICT_SERIALIZED_STATE_LANDMARK = FAILS_NONDETERMINISTICALLY (60 checkpoints)
9D70_REPLAY_NATIVE_ROUTE_EQUIVALENT = YES (one selected path/call, captured surface)
CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES (unchanged)
CPP_REPLACEMENT_VERIFIED_GLOBAL = NO
RUNTIME_REPLACEMENT_ACTIVE = NO
AUTHORITY_DELTAS = 0
```

The native stepper replaced only the first accepted caller-path invocation in
this replay. It did not route other `0x9D70` calls or the unsupported caller at
`0x00979E`.

## Exact natural-call evidence

The paired runs used the same canonical ROM, fixture core, recorded effective
input stream, and captured initial-state file. The selected call occurred at
frame 2655. The ROM and stepped path both entered at cycle `690457`, returned
at `690877`, and charged 420 cycles from helper entry to return (546 cycles
from the caller transfer source). The line target at entry was `690840`; both
paths crossed it by 37 cycles and produced identical return and next-scheduler
boundary snapshots.

The selected call's entry, RTS, return, and next scheduler boundary records
match byte-for-byte. The 128-instruction post-return architectural and
scheduler windows also match byte-for-byte. The helper-timing census contains
184 exact `0x009144` calls; only sequence 1 is marked as instruction-stepped.
All timing/register/return fields match after disregarding that diagnostic
`native` label.

## Full replay comparison

The paired 3,600-frame runs have identical SHA-256 values for:

- full-frame CPU, scheduler, and combined observable-memory snapshots;
- selected call boundary records;
- 128-instruction architectural and scheduler sync windows;
- ordered external bus/VDP events;
- normalized helper-call events;
- sampled Z80 RAM sync data.

The helper-timing JSONL rows also match in every field except the intentional
`native` annotation on the selected first call. Input polls and ordered input
state matched; both sessions completed all 3,600 frames.

The runner's legacy strict landmark check still reports divergence at 60
checkpoints because `retro_serialize` blob hashes differ. This is not isolated
to substitution: two separate ROM-only restores from the same captured
session have the same 60-checkpoint `state_sha256` mismatch pattern, while
PC fingerprints, instruction counts, frame-end CPU state, WRAM hashes, and
globals match. The independent full-frame trace and the call/scheduler traces
are exactly equal between ROM and instruction-step runs. The result is
therefore scoped to those explicitly captured guest/scheduler observables and
the tested replay; it does not claim byte-identical serialized savestates or
equivalence beyond this session/path.

## Timing and status boundary

The 10-case synthetic scheduler fixture passed ROM-versus-step ordered-trace
comparison for its documented instruction subset, refresh, line-budget,
Z80/VDP, IRQ, and resume cases. The natural call then exercised the full
`0x9D70` instruction path under the actual core scheduler and crossed the same
line boundary as ROM. This establishes the instruction-step route for the
selected natural path only. It does not verify arbitrary replacement opcodes,
the other `0x9D70` caller, every helper branch under scheduler stress, or a
production dispatch route.

No aggregate cycle compensation was used. The block hook stepped one
instruction at a time, charging core opcode cycles and preserving the core's
refresh and scheduler checks. The selected helper verification status,
production routing status, canonical SQLite, and runtime/structural/
`SOURCE_OWNED` totals are unchanged.

## Reproduction identities

- ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- HOOK_CPU scheduler fixture core SHA-256: `95358845357104e31949310b7ea63156da8804e390ef4dccc924e1f7c7ed9a8b`
- Linux Debug replay runner SHA-256: `aa8a9e82d60db2091e5a4afa9e6a73deec1bbc2e3c76b9acdea6db5e1a590cb1`
- Linux Release replay runner SHA-256: `98a5e146b00c6995f7f7699b1d89d19e7d3ee9527b59dd00abe6fdb23ed0b562`
- Windows Debug replay runner SHA-256: `a1c9a36f3f7693764189dba603aef61f1044ffe00454086cb6831916711aa327`
- Windows Release replay runner SHA-256: `7c271af12c62519b9331f8030b032e4b98e709bc1427b37a27f8a23a932c6383`
- Machine comparison receipt: `THOR_9D70_INSTRUCTION_STEP_REPLAY_20261013.json`
- Raw paired traces and replay manifests: ignored `build/9d70-step-diagnostic-20261013/post-marker3/`

## Validation

The runner built in Windows Debug/Release and GNU/Linux Debug/Release. The
three focused gameplay-session tests passed in all four configurations. The
10-case scheduler fixture had already passed in GNU/Linux Debug/Release and
Windows runner builds. `gameplay_session_runner.cpp` is 499 lines. The ROM and
assets were not added to the repository. No authority or total changed.
