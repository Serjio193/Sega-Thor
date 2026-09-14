# THOR M12 — AUTO67.6R targeted register predecessor checkpoint

Status: **NEGATIVE — FAIL CLOSED**
Baseline: `4ad091e1fe92f6682f1445a8e92b462609544c41`

## Scope

This checkpoint adds only targeted runtime evidence for register provenance.
It does not start AUTO68, merge graphs, change `SOURCE_OWNED`, mutate the old
AUTO67.4 database, or run long gameplay.

The implementation reuses the existing BizHawk mechanisms:

- `event.on_bus_exec_any` for bounded instruction occurrences;
- `emu.getregister("M68K A4")` and `emu.getregister("M68K A5")` only at a
  selected consumer occurrence;
- a per-investigation 256-record ring in
  `capture/predecessor_capture.lua`;
- worker-side `auto67_predecessor.py` decoding and static register-write
  resolution.

## O67P v1 evidence contract

The frozen predecessor sidecar starts with 56 bytes. After the four-byte magic,
the header has 13 little-endian uint32 values:

```text
version, epoch, target_pc, register_mask,
first_sequence, last_sequence, record_count,
complete, truncated, gap, consumer_sequence,
consumer_frame, overwrites
```

Each record is 28 bytes, seven little-endian uint32 values:

```text
epoch | sequence | frame | pc | opcode | A4 | A5
```

Only A4/A5 requested by the static obligation are semantically used. Values
for non-consumer occurrences are zero-filled; the selected consumer occurrence
gets the targeted register values. The resolver rejects any incomplete,
truncated, gapped or overwritten capture, missing consumer, epoch mismatch,
sequence gap, unsupported writer, or missing producer. It emits a step only
after a statically decoded producer is found before the exact consumer and the
whole producer-to-consumer sequence is contiguous.

## Canary

Static AUTO67.5 evidence for the primary seed is:

```text
BUS_WRITE_PC pc=0x0027EC address=0xC00004
ROM[0x0027EC] = 0x3955
decoded = MOVE.W (A5),-4(A4)
requested register provenance = A5, A4
```

The real 180-frame runs observed the seed as follows:

| run | occurrence 1 | occurrence 2 |
|---|---|---|
| `auto67-6r-finalproof.json` | frame 90, epoch 0, sequence 5552 | frame 120, epoch 0, sequence 8453 |
| `auto67-6r-focused1.json` | frame 90, epoch 0, sequence 1955 | frame 120, epoch 0, sequence 2876 |

The targeted diagnostic run wrote real O67P sidecars such as
`build/auto67-6r-optimized180.capsules/capsule-00-L00000070.bin.pred`.
The persisted launcher snapshot records `record_count=256`,
`predecessor_complete=false`, `predecessor_truncated=true`,
`predecessor_gap=false`, and no usable consumer occurrence. The bounded
resolver therefore returned `INCOMPLETE_PREDECESSOR_CAPTURE`; after the final
overwrite gate it also rejects any capture with `overwrites > 0` as
`PREDECESSOR_RING_OVERWRITE`.

Required result:

```text
producer instruction: UNKNOWN
register version: UNKNOWN
consumer occurrence: observed by discovery, not present in a complete O67P interval
chain_steps: []
unresolved frontier: REGISTER_PROVENANCE / INCOMPLETE_PREDECESSOR_CAPTURE
```

This is a real negative result. No producer PC was copied from prior research
and no causal step was fabricated.

## Genericity and adversarial coverage

The resolver is not address-specific. The same path requests registers from
decoded MOVE source/destination semantics; the test fixture uses an unrelated
supported writer and checks:

- two writes: only the latest reaching definition is selected;
- a write after the consumer cannot connect;
- sequence gaps, truncation, ring overwrite and epoch mismatch fail closed;
- equal register values without a static producer are not causal;
- unsupported static writers remain unresolved;
- exact same-PC occurrence identity is sequence-bound;
- a valid contiguous interval emits one step;
- the `MOVE.W (A5),-4(A4)` shape requests both A5 and A4.

The real production canary did not reach the positive branch because its
capture interval was incomplete. That limitation is retained as the next
explicit frontier rather than hidden by synthetic evidence.

## Real BizHawk evidence

### Accepted normal bounded run

Artifact: `build/auto67-6r-finalproof.json`

- 180 frames, 5.797 seconds, 16 workers;
- 427 worker leases and 427 returns;
- peak busy 16, peak working 7, all workers returned/free at close;
- focused predecessor captures installed: 0, because ordinary focused slots
  consumed the four configured live slots; 407 slot waits;
- duplicate active claims 0, active claims 0 at close;
- merges 137, known rejections 0;
- maximum frame 25 ms; 103 frames over 16 ms, 0 over 33 ms, 0 over 50 ms;
- rolling discovery overwrites 308;
- queue drops 0, database errors 0;
- raw event backlog: `NONEXISTENT`;
- worker pool prestarted: true.

### Targeted predecessor diagnostic

Artifact: `build/auto67-6r-optimized180.json`

- 180 frames, 16 workers;
- six predecessor captures installed;
- 256 predecessor records retained, maximum ring utilization 256;
- one truncation, zero reported gaps, two unresolved register-provenance
- results, zero resolved results and zero chain steps;
- materialized evidence totals: 182 observed facts, 2 static causal facts,
  0 register-provenance chain steps, and 0 unsupported causal facts;
- duplicate active claims 0, queue drops 0, database errors 0, raw backlog
  `NONEXISTENT`;
- frame maximum 778 ms, with 59 frames over 50 ms.

This diagnostic proves the targeted hook and worker handoff are live, but it is
not an accepted gameplay-performance configuration. The hitch is the measured
cost of six simultaneous targeted address hooks; it remains documented rather
than presented as a pass.

### Focused-slot performance confirmation

Artifact: `build/auto67-6r-focused1.json`

With one configured focused slot, the real run completed 489 leases and 489
returns in 180 frames, with 25 ms maximum frame time and zero frames over 33 or
50 ms. The slot was occupied by ordinary focused seeds, so no predecessor
capture was installed; 484 predecessor requests waited for a slot. This
confirms the normal bounded path remains responsive while preserving the
negative capture-capacity result.

## Validation

- `tests/thor_evidence_auto67_6_test.py -q`: 12 passed;
- `tests/thor_evidence_auto67_4_test.py -q`: 8 passed;
- full AUTO67 regression matrix: passed;
- Python compilation: passed;
- source file-limit check: passed;
- `git diff --check`: passed;
- Debug and Release build/CTest: passed;
- real BizHawk launcher return code: 0 for the bounded proof runs.

The authoritative machine-readable form is
`docs/reports/THOR_M12_AUTO67_6R_TARGETED_REGISTER_PREDECESSOR.json`.
