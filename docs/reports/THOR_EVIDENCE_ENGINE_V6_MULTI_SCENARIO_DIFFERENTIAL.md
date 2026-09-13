# THOR Evidence Engine V6 — multi-scenario differential

BASELINE: `1206faa04917a111808be6bf16251ef8542bb455`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Architecture added

`differential.py` adds controlled scenario identities containing environment,
trace, arm, motivation and repeat lineage. Each scenario owns an independent
event/fact/edge graph. Fact keys compare normalized observations while retaining
the scenario event references; the comparison result labels shared facts as
`observation_only` and `causal: false`.

Neutral repeats and motivated alternate arms are validated at construction.
Edges must have endpoints in one scenario graph, so a differential manifest
cannot splice temporal or causal relations across captures. Export is sorted and
explicitly contains empty cross-scenario edge and causal-claim collections.

## Tests

`thor_evidence_v6_test.py` covers common observation classification across two
neutral repeats and an alternate arm, motivation validation, cross-scenario edge
rejection, deterministic graph ownership, and the zero ownership delta.
V0–V5 helpers remain the carried regression set.

## Known defects carried

All V2.1, V3, V4 and V5 ledger entries remain carried. Differential comparison
does not prove causal equivalence, complete same-value writers, interrupt/input
coverage or event completeness; it only reports repeated normalized facts.

## New defects

No blocker discovered. Scenario fact normalization is intentionally narrow and
requires the caller to supply a valid event reference; it does not infer facts
from raw captures or claim semantic equivalence for unlisted fields.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: V7 automatic frontier scheduler.

TRANSITION DECISION: V6 PASS. Continue sequentially to V7 after publication and
exact CI.
