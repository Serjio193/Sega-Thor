# THOR M12 AUTO67 — Live Opportunistic Reverse Engineering

Date: 2026-09-13  
Status: **PASS — SUCCESS LEVEL 4 / SUSTAINED LIVE RE**

## Publication identity

BASELINE SHA: `019fed68d7e906daabe184f3b74017c853b78ce3`  
IMPLEMENTATION SHA: `0bdf500d0443ff80d45e284824a49d85a5ac68e7`  
EXACT CI RUN: [34765072664](https://github.com/Serjio193/Sega-Thor/actions/runs/34765072664)  
ROM: `3,145,728` bytes, CRC32 `C4728225`, SHA256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

The final report is non-owning. Live observations created no SOURCE_OWNED
bytes and did not add ROMs, savestates, assets or payloads.

## Live session

LIVE LAUNCHER: `src/tools/thor_evidence/auto67_live.py` with
`src/tools/thor_evidence/capture/live_opportunistic.lua`  
BIZHAWK RESULT: canonical ROM loaded; bounded live capture exited `0` after
600 frames from hardware reset with normal controller inputs.  
SESSION DURATION: `20.437 s` wall time; baseline capture-disabled run was
`12.734 s` for the same 600 frames (`+60.49%` wall-time delta).

The callback itself only performed fixed-ring append/counter work: measured
capture self-time was `0.027 s` over 600 frame samples (`45 us/frame`), with
no static analysis, SQLite work, chain reconstruction, persistence, or UI I/O
inside callbacks. Worker CPU time was `1.809 s`; BizHawk reached the requested
frame limit and never waited synchronously for a worker.

## Workers and bounded context

WORKERS CONFIGURED: `16`  
PEAK WORKERS BUSY: `16`  
WORKER LEASES: `519`  
WORKER RETURNS: `519`  
WORKER STATES AT CLOSE: all `IDLE`  
ROLLING WINDOW CAPACITY: `64`  
ROLLING WINDOW MAX UTILIZATION: `64`  
WINDOW OVERWRITES: Lua `11,639`; Dispatcher current-window overwrite count
`64`  
RAW EVENT BACKLOG: **ZERO / NONEXISTENT**

The Lua producer observed `11,703` bounded events and retained only the latest
64. The launcher sampled `2,266` current events for dispatch; stale snapshot
contents were discarded when the producer advanced. No pending raw-event FIFO
exists. The machine-readable operator proof is
`THOR_M12_AUTO67_VISUALIZATION_PROOF.json`.

## Sampling and knowledge

EVENTS OBSERVED: `11,703` Lua events; `2,266` Dispatcher-sampled events  
EVENTS RETAINED AS PROOF: `0`  
EVENTS DROPPED/OVERWRITTEN: `11,639` producer-ring observations, plus `64`
bounded Dispatcher-window overwrites  
SEEDS CONSIDERED: `2,266`  
SEEDS DISPATCHED: `519`  
AVERAGE SEED AGE: `0.002329 s`  
MAX SEED AGE: `0.016000 s`  
KNOWN REJECTED BEFORE DISPATCH: `1,681`  
KNOWN DISCOVERED BY WORKER: `0`  
ACTIVE COLLISIONS: `66`  
INVESTIGATION MERGES: `66`  
SAME-SESSION KNOWN REPLAY: **PASS**

NEW CHAINS: `0`  
NEW BRANCHES: `519` bounded live branch fingerprints  
NEW EDGES: `519` observation edges  
INVESTIGATIONS CREATED: `519`  
PROVEN: `0`  
WAITING_RUNTIME: `0`  
BOUNDED_UNRESOLVED: `519`  
BLOCKED: `0`  
EXHAUSTED: `0`  
STRUCTURES ENUMERATED: `0`  
PROMOTION CANDIDATES: `0`  
BYTES PROMOTED: `0`

`PROVEN` in the synthetic worker contract means a bounded investigation can
close and return its worker; the real session correctly kept all live
observations `BOUNDED_UNRESOLVED` and made no semantic or ownership claim.

## Dispatcher/visualization gates

DUPLICATE CLAIM PREVENTION: **PASS**  
WORKER RETURN-TO-POOL: **PASS**  
ROLLING-WINDOW BOUNDEDNESS: **PASS**  
EMULATOR NON-BLOCKING: **PASS**  

The same launcher serves a local operator view from a bounded snapshot API.
It displays frame/epoch, event and novelty rates, window utilization, all 16
worker states, investigation IDs, chain fingerprints, stages, seed age and
task runtime, claims, known rejections, collisions, merges and knowledge
counts. UI updates can be dropped; no UI call is on the callback, claim or
worker execution path. The live proof shows two workers concurrently active,
workers returning to IDLE, same-session known rejection and active merges.
Supported visible transitions include:

`IDLE -> LEASED -> WORKING`  
`WORKING -> KNOWN -> RETURNING -> IDLE`  
`WORKING -> PROVEN -> RETURNING -> IDLE`  
`WORKING -> WAITING_RUNTIME -> RETURNING -> IDLE`  
`WORKING -> MERGED -> RETURNING -> IDLE`

MANUAL ADDRESS SELECTION: **NO**  
SERIAL ONE-ADDRESS FALLBACK: **NO**

## Regression and reconstruction gates

AUTO65/AUTO66/AUTO67 targeted tests: `12/12` PASS.  
Windows Debug: `186/186` PASS.  
Windows Release: `186/186` PASS.  
GNU/Linux build/link: PASS.  
GNU/Linux CTest: `186/186` PASS.  
`git diff --check`: PASS.  
Source file policy: `625` governed files, all `<=500` lines.

FULL-LAYOUT ASM: **PASS** — 80 entries, gaps `0`, overlaps `0`,
`full_layout.asm` SHA256
`45E67BC19FD9AAFFFD7DEE9716DA5A8C218B97D185EE1D16F7C7D0A33E337C51`.  
ROM BYTE-EXACT: **PASS** — `3,145,728/3,145,728` bytes exact.

## Stop reason

AUTO67 stopped at the bounded 600-frame controlled validation limit after
proving sustained live sampling, multiple concurrent leases, same-session
knowledge feedback, active collision/merge handling, worker return, bounded
overwrite and non-blocking capture. Production mode remains open-ended until
the user closes BizHawk. No SOURCE_OWNED increase was required or inferred;
the next evidence step would require genuinely new gameplay/state coverage.
