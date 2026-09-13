# THOR Evidence Engine V9 — operational M12 integration

BASELINE: `8a75a124c40eec0fe18f359f3605a54e37ff324b`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Machine assembled

`orchestrator.py` joins the existing sealed capture import and SQLite sidecar,
V7 frontier scheduling, V5 static request/response and non-owning Carver merge
into one bounded operational cycle. It rejects trace splicing, persists the
capture through the existing transactional `Store`, exports the persistent-store
digest, static bridge state, scheduler state and Carver interval state in one
deterministic manifest, and carries an explicit zero ownership delta.

`cli.py` exposes the seed/capture/query workflow for one bounded cycle. The CLI
does not execute an uncontrolled trace or promote any static result.

## Tests

`thor_evidence_v9_test.py` covers capture orchestration, scheduler request
generation, static evidence merge, SQLite persistence, deterministic manifest
bytes, Carver ownership invariance and the CLI workflow. V0–V8 helpers remain
the carried regression set.

## Known defects carried

All V2.1–V8 entries remain carried. The assembled cycle does not close causal
provenance, infer access width, prove IRQ/input paths, or authorize SOURCE_OWNED
promotion. V9 is an operational build gate, not stabilization.

## New defects

No blocker discovered. V9 currently executes one bounded cycle per invocation;
multi-cycle policy, retry policy and production-facing UX remain outside this
build phase.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: **STOP**. Post-V9 stabilization is intentionally not started.

TRANSITION DECISION: V9 PASS. The autonomous V4→V9 build is complete.
