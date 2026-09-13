# THOR Evidence Engine V8 — held-out unknown frontier

BASELINE: `bef6c32087694a18918f2a77fd41ed559fc40fe2`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Held-out evaluation

The machine evaluated the real canonical frontier `0x03BDA6–0x03BF86`, whose
preferred unresolved stream begins at `0x03BDD8`. It invoked the existing
`m12_relative_table_analysis.py` against the local canonical ROM and consumed
the existing natural runtime receipt with matching ROM identity.

Static enumeration returned the bounded pointer root, consumer grammar and 11
consumed pointer entries. The stream at `0x03BDD8` remains
`BLOCKED_BY_SELECTOR_0_INDEX_1_STREAM` because no bit-15 terminator was found
before the independent code boundary. Runtime discovery was accepted as an
observation, but its target hit does not close that static termination frontier.
The joined relation therefore remains `UNKNOWN` and `causal: false`.

## Architecture added

`heldout.py` binds the frontier to the canonical ROM and range, validates a
runtime receipt, runs the real static analyzer, and emits a non-owning combined
evaluation. It preserves unresolved capabilities and rejects wrong-ROM or
incomplete runtime inputs.

## Tests

`thor_evidence_v8_test.py` covers runtime receipt identity/shape guards, the
real local static/runtime evaluation when developer-only artifacts are present,
the preserved UNKNOWN join, and zero ownership delta. V0–V7 helpers remain the
carried regression set.

## Known defects carried

All V2.1–V7 entries remain carried. V8 does not close the unterminated stream,
prove complete consumer coverage, infer access width, or turn runtime reachability
into causal provenance.

## New defects

No blocker discovered. The held-out frontier produced useful new bounded
structure but correctly left the central termination/join capability UNKNOWN.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: V9 operational M12 integration.

TRANSITION DECISION: V8 PASS. Continue sequentially to V9 after publication and
exact CI.
