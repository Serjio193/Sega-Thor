# M12 END GAME → compact rolling master 2I

Status: `PASS_END_GAME_ROLLING_MASTER_COMPACT_V1`.

2I.1 progress repair: `PASS_POSTRUN_PROGRESS_HEARTBEAT_UI_V1`.
The backend publishes an atomic status snapshot at a 4 Hz bounded heartbeat;
the UI refreshes at 4 Hz and shows factual N/M progress or an indeterminate
animation when no denominator exists. Each of the nine stages has an explicit
state. The historical receipt captured a compact-only run before the 2I.2 coordinator was integrated; its later stages were not evidence and must not be replayed as PASS. The current coordinator enters Stage 5 and stops explicitly when accepted inputs are missing.

The current Worker Control window publishes one idempotent `END GAME` request. Lua consumes it at a complete round boundary and exits EmuHawk through `client.exitCode(0)`; it does not hard-kill the emulator.

The existing sealed interactive run `1789830432` was compacted without a new gameplay campaign. It read **94,703,572** FLOW records from **287,744** retained segments and produced atomic generation `master-0ad568aeb80644b0` (495,616 bytes). The pointer is `build/thor-evidence/live-worker-control-2h/rolling-master-2i/current.json`.

The compact master contains **1,908** unique instruction-PC/opcode facts, including **1,580** PCs absent from the seeded 2G instruction set, **2,528** unique compact edges, **2,175** new edge keys, and **287,744** terminal facts. `SOURCE_OWNED` stayed unchanged (delta 0).

The canonical 2G knowledge pointer and 2F closure were deliberately not replaced or claimed PASS. The interactive run has no post-run Cartographer/Archivist session or exact decoder range export for the new PCs, so raw FLOW and the segment index remain retained under the rollback policy. No cleanup deleted A or B.

The four executed compact stages expose exact progress counters; the five
unexecuted stages use indeterminate semantics and are marked
`SKIPPED_NOT_APPLICABLE`. Stage durations are persisted in
`status.json` (`stages.*.duration_seconds`). A maximum heartbeat age is not
claimed for the historical 2I run because it predates the heartbeat contract.

Validation: Worker Control tests 24/24 PASS; compact rolling-master fixture PASS;
post-run progress/heartbeat tests PASS; Python syntax compilation PASS;
`git diff --check` PASS. Native pixel-level GUI acceptance remains unverified
when no native app surface is available to the test harness.


## 2I.2 stage contract

The post-run backend now enters stages 5–9 automatically after compacting. Stage 5 accepts only a hash-checked ordered FLOW spool plus an accepted canonical master/knowledge pair and publishes a new paired generation atomically; replay is idempotent and runtime refresh must keep `SOURCE_OWNED` and emission ownership unchanged. Stage 6 inspects ordered FLOW for actual indirect consumers and reports `NO_DELTA` when none exist. The current main tree has no accepted 2F closure runner, so the coordinator stops with `STOP_RECONSTRUCTION_UNAVAILABLE` instead of claiming stages 7–9. Raw FLOW remains retained on every STOP; cleanup is prohibited until a genuine stage 8 PASS.

The historical compact-only receipt above remains a historical result. It is not re-imported by the new coordinator.


## 2I.2a terminalization

Terminal status publication is now centralized. `COMPLETE`, `PARTIAL_COMPLETE`, `STOPPED`, and `FAILED` snapshots always have `active_stage = null` and no ACTIVE stage. STOP/ERROR blocks later stages; compact-only mode uses explicit `SKIPPED_NOT_APPLICABLE`. A stale heartbeat with a live backend remains non-terminal and is displayed as unresponsive. Raw FLOW and accepted generations remain retained on STOP/ERROR.
