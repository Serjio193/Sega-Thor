# THOR M12 AUTO67.6R3C — generic register-writer targeting

## Result

**NEGATIVE — generic targeting is not gameplay-safe at the measured bounded
scale.** The implementation generates runtime writer targets from the existing
static M68K register-definition decoder, but a real BizHawk run with only 500
of the 30,042 statically proven candidate hooks reached 63 ms maximum frame
time and one frame over 50 ms. The full set was therefore not installed after
the fail-closed performance gate; it would be an unsafe escalation, not a
valid acceptance proof.

This checkpoint does not change architecture, `SOURCE_OWNED`, worker/DB
design, graph merging, BizHawk, or AUTO68 scope.

## Static generic configuration

`register_writer_candidate_report()` reuses `register_writes()` and emits a
bounded text target file consumed by `predecessor_burst.lua`. Producer PCs are
not embedded in the Lua runtime configuration. The static scan found:

| set | count |
|---|---:|
| A4 writers | 16,732 |
| A5 writers | 13,590 |
| unique PCs | 30,042 |
| duplicate PCs | 0 |

The canary PCs are present in the generated file: `0x002234` targets A5 and
`0x0027BE` targets A4. The runtime Lua source contains neither producer
literal. The generated file is `build/auto67-6r3c-targeted-idle-500.register-writer-targets.txt`.

The main opcode distribution and compact unsupported/truncated counts are in
the machine-readable JSON report. Unsupported writer forms remain excluded;
they are never accepted as runtime targets.

## Real BizHawk gate

The run used the canonical ROM, QuickSave1, 16 prestarted workers, the existing
launcher and native capture path, `targeted_idle`, 30 frames, and a generated
prefix of 500 static candidates. It returned code 0. All 500 hooks installed,
215 targeted callbacks fired, 10 candidates became hot, and hook-install
errors were zero. The run observed 81 leases and 81 returns, peak busy 16,
11 active collisions, zero duplicate active claims, raw backlog
`0 / NONEXISTENT`, queue drops 0 and DB errors 0.

The relevant frame timings were:

| installed hooks | p50 | p95 | p99 | max | >16 ms | >33 ms | >50 ms |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 16 | 17 | 22 | 23 | 94 | 0 | 0 |
| 2 (R3B reference) | 16 | 17 | 22 | 23 | 89 | 0 | 0 |
| 50 | 16 | 19 | 34 | 34 | 36 | 1 | 0 |
| 200 | 17 | 20 | 39 | 39 | 29 | 1 | 0 |
| 500 | 41 | 46 | 63 | 63 | 30 | 30 | **1** |

The 500-hook artifact is `build/auto67-6r3c-targeted-idle-500.json`.

## Semantic boundary

The generated target list and static canary inclusion are proven. The exact
generic live burst resolver was not accepted in this checkpoint because the
bounded performance gate failed before a safe full generic burst run. The
previous R3B proof remains the semantic reference for the canary, including
latest-reaching-writer and bounded fail-closed behavior; R3C does not replace
that proof with a simulated transition.

The 30,042-hook run was intentionally not started after the 500-hook real
failure. This is reported as a negative result, not as an unmeasured PASS.

## Validation

The focused R3C test covers static candidate de-duplication, canary candidate
inclusion, absence of hard-coded producer PCs in the Lua source, and truthful
full-source/installed-hook metrics from the real artifact. Existing R3B and
AUTO67 tests remain applicable. Final build/CTest and source-limit results are
recorded in the closing worklog entry.

Machine-readable proof: `docs/reports/THOR_M12_AUTO67_6R3C_GENERIC_REGISTER_WRITER_TARGETING.json`.
