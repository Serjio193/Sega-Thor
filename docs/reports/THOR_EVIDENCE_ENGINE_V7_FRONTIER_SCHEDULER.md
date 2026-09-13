# THOR Evidence Engine V7 — automatic frontier scheduler

BASELINE: `4b5b08ecbc5646a0d1d483687cc82ff51f2ab70f`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Architecture added

`frontier.py` adds an unresolved-frontier inventory with deterministic scoring
by information gain, confidence, cost and risk. Every request is bounded by a
configured range and carries an explicit evidence class. Whole-ROM tracing is
rejected and exported manifests state `whole_rom_trace: false`.

The scheduler permits at most two non-progress attempts per frontier. A second
non-progress result exhausts that frontier; the scheduler then selects the next
ranked item. Results, requests, exhaustion and fixed-point state are exported
without ownership or promotion operations.

## Tests

`thor_evidence_v7_test.py` covers ranking, bounded request generation, two-pass
exhaustion, whole-ROM rejection and fixed-point/ownership invariants. V0–V6
helpers remain the carried regression set.

## Known defects carried

All V2.1–V6 entries remain carried. Ranking is a deterministic heuristic and
does not measure real information gain; evidence-class selection does not prove
that a request will close its frontier.

## New defects

No blocker discovered. Scheduler output remains request metadata and does not
execute tracing, static analysis or causal inference.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: V8 held-out unknown frontier.

TRANSITION DECISION: V7 PASS. Continue sequentially to V8 after publication and
exact CI.
