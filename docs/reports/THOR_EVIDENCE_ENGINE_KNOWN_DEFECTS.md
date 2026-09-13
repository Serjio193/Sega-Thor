# THOR Evidence Engine known defects

This ledger is carried into V3 BUILD from the independent V2.1 counterexample
audit. V3 is an assembly milestone; it does not promote any of these paths to
sound evidence.

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks current build | Stabilization |
|---|---|---|---|---|---|---|
| V2.1-001 | Critical | `ram_versions.py` | Public verified-certificate construction and fabricated tagged historical events can produce false `PROVEN`. | V3 marks inherited RAM inputs provisional unless narrow accepted proof exists. | No | V3 stabilization |
| V2.1-002 | High | `canary_engine.py` | Adapter coverage does not establish complete instruction effects; replacing an EXEC with NOTE/unknown PC can preserve `PROVEN`. | V3 never widens inherited RAM status. | No | V3 stabilization |
| V2.1-003 | High | `store.py` | Operation output ordering, previous-version semantics and root-vs-write meaning are not fully validated on import. | V3 imports use immutable V3 identities. | No | V3 stabilization |
| V2.1-004 | High | V1 graph | Legacy target can still terminate upstream dependencies instead of traversing concrete V2 output. | V3 graph is separate and does not reinterpret legacy edges. | No | V3 stabilization |
| V2.1-005 | High | `canary_engine.py` | `validate_certificate()` has an early V2 branch that bypasses old causal checks. | V3 does not use it as a soundness gate. | No | V3 stabilization |
| V2.1-006 | Critical | FF188A oracle | Fabricated verified-certificate paths can defeat the historical `INCOMPLETE_CAPTURE` oracle. | FF188A remains an explicit V3 frontier. | No | V3 stabilization |

No SOURCE_OWNED transaction is allowed during V4-V9 BUILD.

## V4 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V4 | Stabilization |
|---|---|---|---|---|---|---|
| V4-001 | Medium | `v4_domains.py` | DMA timing and same-frame VDP publication are not represented by the bounded contract. | DMA remains `PROVISIONAL`; no publication claim is emitted. | No | V4/V9 stabilization |
| V4-002 | Medium | `v4_domains.py` | Complete register-mapped hardware aliases and IRQ/exception effects are not enumerated. | Domains are explicit and unknown aliases fail closed. | No | V5–V9 evidence expansion |

## V5 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V5 | Stabilization |
|---|---|---|---|---|---|---|
| V5-001 | Medium | `static_bridge.py` | Static records do not by themselves close indirect CFG or exact parser boundaries. | Requests are bounded; certificates remain EVIDENCE_ONLY and unresolved frontiers are retained. | No | V5/V9 stabilization |

## V6 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V6 | Stabilization |
|---|---|---|---|---|---|---|
| V6-001 | Medium | `differential.py` | Shared normalized facts do not establish causal equivalence or complete same-value writer sets. | Common results are tagged observation-only with empty causal claims; each graph remains scenario-local. | No | V7/V9 stabilization |

## V7 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V7 | Stabilization |
|---|---|---|---|---|---|---|
| V7-001 | Medium | `frontier.py` | Ranking scores are deterministic heuristics and are not measured information gain. | Scores are explicit, bounded and never interpreted as proof; each frontier has a two-pass cap. | No | V9/stabilization |

## V8 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V8 | Stabilization |
|---|---|---|---|---|---|---|
| V8-001 | High | `heldout.py` / `03BDD8` | Existing runtime reachability does not close the static unterminated stream or prove its causal join. | Combined result remains UNKNOWN with `causal: false`; no ownership promotion. | No | V9/stabilization |
