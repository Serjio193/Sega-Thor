# THOR Evidence Engine V4 — ROM / resource / hardware bridge

BASELINE: `39e3f5f09ccc8ddd5c21d140cd57345d42a9d2cc`
FINAL SHA: publication commit; exact SHA and CI are recorded in the final gate response.
CI: exact publication CI recorded in the final gate response.
PASS/PARTIAL/BLOCKED: **PASS**

## Architecture added

`v4_domains.py` adds one typed cross-domain graph over the existing V3/RAM
identities. It has canonical ROM roots, checked resource-transform contracts,
video/hardware versions, bounded DMA transfers and explicit dependency roles.
The existing SQLite sidecar is extended with V4 tables; no second database or
ownership path is introduced.

## ROM roots and resource transforms

ROM roots require the canonical 3,145,728-byte ROM identity and reject offsets
outside the image. `ROM_DATA`, `ROM_CODE_CONSTANT`, `IMMEDIATE_CONSTANT`,
`EXTERNAL_STATE`, `RAM`, `REGISTER` and `HARDWARE` remain distinct root kinds.
The `0x3820` fixture records the checked Ancient-decoder interop contract,
input/output hashes and output size without claiming a new ROM discovery.

## Hardware and DMA

The bridge validates VRAM, CRAM, VSRAM and SAT ranges. A DMA transfer keeps
source identity, destination domain/range, execution context and status. It
emits typed VALUE and EXECUTION edges; address edges remain explicit when a
descriptor/root is supplied. Invalid and zero-length transfers fail closed.

## Cross-domain graph

V4 can import V3 graph identities as external upstream nodes and link ROM,
constant, register/RAM and hardware targets without flattening VALUE, ADDRESS,
CONTROL or EXECUTION roles. New generalized relations default to
OBSERVED/PROVISIONAL/UNKNOWN/CONFLICT; no V2 uncertainty is promoted.

## SQLite persistence

The existing sidecar now persists roots, resource transforms, hardware
versions, DMA transfers and typed dependencies. Imports are transactional and
idempotent for immutable payloads; identity collisions reject.

## Tests

`thor_evidence_v4_test.py` covers ROM/resource/DMA/hardware integration, domain
and ROM guards, graph roles, SAT explain output, SQLite reopen/idempotence and
conflicting duplicate rejection. V0–V3 helpers remain green.

## Known defects carried

The V2.1 fabricated-certificate, incomplete adapter coverage, SQLite semantic
validation, V1 validator bypass and FF188A false-proof defects remain in
`THOR_EVIDENCE_ENGINE_KNOWN_DEFECTS.md`. V4 deliberately does not repair them.

## New defects

No blocker discovered. V4 does not prove real-time DMA scheduling, same-frame
VDP publication, complete register-mapped hardware aliases, palette semantics,
or global IRQ/exception behavior. These are explicit frontier items.

## Blockers repaired / deferred defects

No prior V2/V3 blocker was repaired. The only repairs are V4-local validation
and identity containment. Full Ancient decoder execution, DMA timing, VDP
register semantics and hardware causality remain deferred.

SOURCE_OWNED before/after/delta: `1,475,368 / 3,145,728` ->
`1,475,368 / 3,145,728`, delta `0`.

NEXT STAGE: V5 static enumeration + non-owning Carver bridge.

TRANSITION DECISION: V4 PASS. Per the user's sequential instruction, stop
after V4 publication and await the next stage request; V5 has not started.
