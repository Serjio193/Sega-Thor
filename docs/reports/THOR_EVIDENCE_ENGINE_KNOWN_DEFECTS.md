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

## V9 frontier entries

| ID | Severity | Module | Reproduced symptom | Temporary containment | Blocks V9 | Stabilization |
|---|---|---|---|---|---|---|
| V9-001 | Medium | `orchestrator.py` | One CLI invocation runs one bounded cycle and does not implement multi-cycle retry policy. | Cycle state is deterministic and explicit; callers must schedule subsequent cycles. | No | Post-V9 stabilization |

## Stabilization lifecycle

| ID | Status | Root cause | Repair / regression |
|---|---|---|---|
| V2.1-002 | VERIFIED | Coverage basis could omit a complete epoch boundary while retaining checked tags. | Attested coverage now requires a COMPLETE EPOCH_END and rejects NOTE/unknown-effect substitutions; truncation regression added. |`r`n| V2.1-001 | VERIFIED | Coverage accepted arbitrary tagged event dictionaries as a verified basis. | Content-attested event/basis hashes; `test_unattested_historical_tags_cannot_create_verified_coverage`. |
| V2.1-003 | VERIFIED | SQLite import checked identities but not producer, address, value or predecessor semantics. | Semantic output/predecessor checks and rollback regression in `test_sqlite_rejects_wrong_producer_output_association`. |
| V2.1-005 | VERIFIED | `ram_engine` branch returned before the legacy proof obligations. | Dedicated V2 certificate validator and `test_ram_branch_cannot_bypass_certificate_contract`. |
| V2.1-006 | VERIFIED | Forged RAM coverage could bypass the FF188A negative oracle. | V2 validator now requires trace-bound coverage over the target and rejects missing coverage. |

| V2.1-004 | VERIFIED | Legacy V1 target was not required to bind to concrete V2 byte outputs. | Identity-checked causal_bridge and adversarial detachment regression. |

P0 checkpoint: commit 066a5d9c43062369984877ef48ec19adb924e7c3; CI run 34748750039 (success). Windows Release 176/176, GNU/Linux evidence 12/12, direct V0-V9 24/24, SQLite integrity_check=ok. SOURCE_OWNED unchanged (1,475,368 / 3,145,728; delta 0).
