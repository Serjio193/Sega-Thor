# THOR Evidence Engine stabilization report

## Scope

This stabilization reviewed the assembled V0–V9 evidence chain at the V9
baseline and repaired bounded trust defects. It did not migrate to native C++,
reverse engineer a new ROM region, or promote SOURCE_OWNED bytes.

The chain under review is:

`sealed capture → identity/epoch → attested coverage → RAM byte versions →
V1/V2 target bridge → SQLite import → held-out frontier → operational manifest`.

## Repaired defects

| Area | Failure mode | Bounded repair |
|---|---|---|
| V2.1-001 | Fabricated historical tags reached verified coverage. | Content-attested event and basis hashes; accepted event kinds; unknown effects, alias and overlap markers rejected. |
| V2.1-002 | A coherent event fragment could omit completion. | Verified coverage requires an attested `EPOCH_END` with `reason=COMPLETE`; truncated bases fail closed. |
| V2.1-003 | SQLite accepted semantically inconsistent RAM output rows. | Producer, address, big-endian value, predecessor, cardinality, range and rollback checks. |
| V2.1-004 | Legacy V1 target could remain detached from concrete V2 bytes. | Identity-checked `causal_bridge` binds legacy target, RAM operation, four byte outputs and witness sequence. |
| V2.1-005 | V2 certificate branch bypassed proof obligations. | Dedicated validator checks certificate digest, operation/version identities, target coverage, bridge and frontier. |
| V2.1-006 | Forged RAM coverage bypassed the negative oracle. | Trace-bound complete coverage is mandatory for a `PROVEN` RAM result. |
| V8 receipt | Truthy malformed `target_reached` became `OBSERVED`. | Exact type and temporal-shape validation before the UNKNOWN join. |
| V9 replay | Repeated capture appeared twice in the manifest. | One trace identity per operational cycle; SQLite replay remains idempotent. |

All repaired paths have adversarial regression coverage. No repair required a
second or third failed attempt; the user’s three-attempt stop rule was not
triggered.

## Evidence and validation

The final checkpoint is the commit and CI run recorded below. The local checks
covered the full assembled test surface and the evidence-specific path:

* Windows Release CTest: 176/176 passed.
* GNU/Linux Release CTest evidence set: 12/12 passed.
* Direct `tests/thor_evidence_*_test.py`: V0–V9, 24/24 passed.
* SQLite `PRAGMA integrity_check`: `ok` after capture import.
* `git diff --check`: clean for tracked changes.
* Changed source and test files remain within the 500-line project limit.

`SOURCE_OWNED` remains `1,475,368 / 3,145,728`; delta is `0`.

## Remaining explicit frontier

The following remain UNKNOWN and are intentionally not inferred by this
stabilization:

* access width outside the checked `MOVE_LONG_D2_TO_RAM` operation;
* complete overlap/range semantics outside the checked target interval;
* completeness of same-value writers outside the bounded capture;
* IRQ/exception interaction;
* causal input-read provenance;
* global writer completeness and whole-program control provenance;
* termination and complete consumer extent for the `0x03BDD8` held-out stream;
* DMA timing and complete register-mapped hardware aliases from V4.

The FF13CC canary may be used only with its checked ROM high24, incremented
D5.low8, FF188C destination, concrete A372 operation and four byte outputs.
The validator rejects address-only edges, PC-2 inference, input-causal edges,
detached V1/V2 identities and frontier deletion. No claim is made for a global
last-writer theorem or a whole-game causal graph.

## Final status

The foundation is stabilized for a narrow, evidence-bound V1 FF13CC canary
continuation. Starting broader V1 work still requires preserving the explicit
frontier above and must not promote SOURCE_OWNED or the held-out UNKNOWNs.
