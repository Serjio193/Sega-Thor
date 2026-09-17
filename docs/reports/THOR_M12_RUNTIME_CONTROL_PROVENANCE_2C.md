# M12 Runtime Control Provenance 2C — Phase 0 result

**Status: `STOP_NO_RUNTIME_PROVENANCE_GAP`**

This checkpoint starts from accepted 2B commit
`6bac80aa93a3a6cb1c22d1416147c525223946d2` (`M12: link live FLOW_V1 execution to exact ROM ranges`). Phase 0 used its existing saved BizHawk/GPGX evidence; it did not rerun a runtime or scaling campaign.

## Evidence and method

The source evidence is 2B run `1789714283`: 16 Workers, 100 completed cycles
per Worker, depth 20, and 65,536 bytes per Worker. The accepted 2B independent
auditor passed all 1,600 segments, 199,630 instruction occurrences, and 1,600
terminal `next_pc` facts. The canonical ROM is 3,145,728 bytes with SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The Phase 0 classification read the saved `<QQIIHHI` FLOW_V1 records and
separated instruction, control-flow, branch-outcome, and asynchronous exception
flags. The parent 2B independent audit had already validated every saved
instruction PC/opcode occurrence against raw records, canonical ROM bytes, and
a fresh decoder. This phase did not treat static reachability or possible
targets as runtime facts.

| Runtime classification | Occurrences |
| --- | ---: |
| BRA (direct unconditional branch) | 1,148 |
| Bcc (conditional branch), taken / not taken | 25,388 (12,181 / 13,207) |
| DBcc (conditional loop branch), taken / not taken | 44 (44 / 0) |
| BSR (direct subroutine branch) | 2,507 |
| Direct JSR | 290 |
| Direct JMP | 0 |
| Indirect JSR | 0 |
| Indirect JMP | 0 |
| Returns | 1,760 |
| Other instruction occurrences | 168,493 |
| Additional asynchronous exception events (vector 30) | 863 |

The instruction categories sum to 199,630. Exception events are separate
non-instruction records. All 290 JSR occurrences are at `PC=0x002380`, decode
as `JSR abs.l`, and have exact bytes `4EB900060000`; FLOW records
`next_pc=0x00060000` for each. The destination comes from the instruction's
absolute-long extension, not a memory value loaded into a register. It is
therefore a direct encoded target and does not demonstrate the 2C memory-origin
provenance chain.

## Gap decision

There were no actual indirect JMP/JSR occurrences, so there is no consumer
occurrence to classify as `SOURCE_RESOLVED_EXISTING`, `SOURCE_UNRESOLVED`,
`SOURCE_TRANSFORMED`, `SOURCE_NON_MEMORY`, or `SOURCE_UNKNOWN`. In particular,
the saved run contains zero observed unresolved memory-origin targets. The
Phase-0 gap test therefore stops before adding a native sensor:

```text
STOP_NO_RUNTIME_PROVENANCE_GAP
```

This is a bounded negative for the saved execution, not a claim that no other
gameplay path contains indirect transfers. A future sensor proposal needs a
real accepted runtime indirect JMP/JSR whose target provenance is unresolved by
existing evidence. No Exodus prediction, static branch expansion, or
unexecuted jump-table entry was added to runtime truth.

## Preserved boundaries and validation

No Worker 1B lifecycle/scaling, FLOW_V1, 2B linkage, Cartographer, Archivist,
AUTO67, predecessor logic, native execution core, Lua capture path, or
`SOURCE_OWNED` value changed. Ownership was 0 bytes before and after, delta 0.
No new BizHawk campaign was run. Since this result adds documentation only,
Debug/Release builds and tests were not rerun; the parent 2B checkpoint's
full validation remains recorded in its report and receipt.

The compact receipt records the exact ROM, raw segment, segment index, and
range-export hashes. Raw FLOW, session SQLite, and runtime evidence remain in
the ignored local `build/thor-evidence/live-forward-rom-link-2b/` directory and
are not part of this checkpoint.
