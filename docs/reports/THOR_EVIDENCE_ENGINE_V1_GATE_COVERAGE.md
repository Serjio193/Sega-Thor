# THOR Evidence Engine V1-GATE-COVERAGE — local dense closure

**BASELINE SHA:** `a5cc2d09dc2698127f9977b3bf942f3d8e845ee3`
**FINAL SHA:** recorded in the final publication delivery

This report closes only the local dense execution, checked memory-effect and
interruption obligations for the already selected FF13CC A372 instance. It
does not implement V1 provenance, a graph, a general last-writer engine,
register provenance, a scheduler, new ROM discovery or SOURCE_OWNED promotion.

## Dense interval

The narrow interval starts at the first proven pre-boundary `A370` with
`A5=FF13CC` and ends at the next `A374` fetch after the selected `A372` store.
`A374` is a post-fetch boundary witness; it is not silently treated as an
unobserved instruction body. This is sufficient because the selected store is
`A372`, the next ordinary fetch is `A374`, and `event.on_bus_exec_any` reports
every instruction fetch between them. An exception or handler before `A374`
would appear as an intervening EXEC event and fail the continuity check.

| field | value |
|---|---|
| pre-boundary | epoch 1, raw seq `1`, PC `0x00A370` |
| selected A372 execution | epoch 1, raw seq `2`, PC `0x00A372` |
| post-boundary | epoch 1, raw seq `4`, PC `0x00A374` |
| executed instructions in interval | `2` (`A370`, `A372`) |
| raw event stream | `12` events across restore epochs `1,2` |
| capture API | `event.on_bus_exec_any` in BizHawk 2.11.1 |

The raw capture is local and ignored:
`build/thor-evidence/v1-gate-coverage/v1dense-attempt2.raw.jsonl`.
Its SHA-256 is
`d4d1a3e622785bc0b6e87c2e7dd07ac0f4173c2e02eab1b0ab5e6f0845da23df`.
The launch receipt identity is capture
`372bf3a0-37c5-45d2-801f-1ea34052d588`, receipt SHA
`066b9e470ed7a49694c108609ba36047e529503f811e312b9cbbe4e535b42c63`.
The sealed JSONL normalization also completed successfully.

## Execution coverage

The raw envelope is complete, sequence-contiguous and has two explicit restore
epochs. Every event in the declared interval came from `event.on_bus_exec_any`;
equal PCs are retained as separate execution instances. The callback's opcode
argument is zero on this BizHawk build, so instruction bytes are supplied by a
canonical-ROM reference and the checked static decoder, rather than inferred
from that callback field.

Canonical ROM identity is 3,145,728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The static JSON range `A342..A438` is SHA-256
`33ab59edbb3d9391f834e4848fabb6003dd7e7181cdaa2182137d584de82e421` and is
bound to `re_slice_decoder.cpp` SHA-256
`00457dfb5f1cb3e91805ea0545f13035879cf961b7ebf9669a51abef90d25109`.

## Memory-effect classification

The selected interval is classified as follows:

| classification | count |
|---|---:|
| `NO_MEMORY_WRITE` | 1 |
| `WRITE_DISJOINT` | 0 |
| `WRITE_TARGET_OVERLAP` | 1 |
| `UNKNOWN_MEMORY_EFFECT` | 0 |

The complete writer list contains the expected target writer:

| exec instance | PC/form | width | effective range | target intersection | value/witness |
|---|---|---:|---|---|---|
| `epoch-1-seq-2` | `A372 MOVE.L D2,(A5)+` | 4 | `FF13CC..FF13CF` | `FF13CC..FF13CF` | `00880901`, raw WRITE seq `3` |

The concrete EA is calculated from the pre-execution A5 snapshot. A same-value
write or a one-byte overlap remains a writer in the validator and cannot be
reclassified as absence.

**WRITER COMPLETENESS: PROVEN for the declared local interval.**

## Interruption certificate

The checked next-PC edges are:

```text
A370 + 2 -> A372
A372 + 2 -> A374 post-fetch boundary
```

No vector entry, supervisor/stack discontinuity or unexplained control transfer
appears between the boundaries.

**INTERRUPTION: `NO_INTERRUPTION_IN_INTERVAL`.** This is a local boundary
certificate and makes no global IRQ/exception claim.

## A372 pairing recheck

The prior pairing remains valid: EXEC seq `2` has pre-A5 `FF13CC` and D2
`00880901`; WRITE seq `3` observes destination `FF13CC` and value `00880901`;
the next fetch boundary is seq `4` at `A374` with post-A5 `FF13D0`. No PC-2
heuristic is used.

## Adversarial coverage tests

The persisted fixture
`tests/fixtures/thor_evidence_v1_gate_coverage/negative_cases.json` rejects:

- removing one executed instruction or one loop iteration;
- reordering instructions;
- an unknown memory effect;
- a forged disjoint effective address;
- forgetting a one-byte partial overlap;
- omitting a same-value overwrite;
- an unexplained control-flow discontinuity;
- a vector/handler entry with an incomplete handler body;
- collapsing duplicate-PC execution instances.

The positive fixture keeps correctly represented partial and same-value overlap
writers as two explicit `WRITE_TARGET_OVERLAP` entries.

## Decision

**V1-GATE-COVERAGE: PASS.** The local dense coverage and interruption gates are
closed for the selected FF13CC interval.

**V1 READINESS: READY for a separately authorized V1 implementation.** This
does not authorize implementation in this task. Input-read causality and other
capabilities outside this local closure remain explicit frontiers.

`SOURCE_OWNED` before/after is `1,475,368 / 3,145,728`; delta `0`.

## Tests, CI and stop

The new dense capture, receipt finalization, normalization and certificate
passed locally. V0, V1-gate and dense-gate Python tests pass, and the dense
helper is CTest-integrated beside the earlier gates. Release and
Linux-equivalent focused CTest are the applicable build checks. The known full
Debug MSVC `std::to_string` failure in `src/core/ram_flag_routine.cpp` remains
pre-existing and outside this bounded developer-only task.

The final publication SHA and exact CI run are recorded in the final delivery.
After publication, STOP. Do not implement V1 in this task.
