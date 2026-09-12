# THOR Evidence Engine V0.1 — foundation repair

Status: PASS for the bounded V0.1 repair; V1 remains BLOCKED and was not
implemented. Baseline SHA: `38e702cbae29e2b8f6d09c70b9ffd2e73db0d739`.
Final publication SHA and CI are recorded in the delivery and worklog.

## Audit findings addressed

P1-A was reproduced with a real 189-event first-epoch prefix, a false
`RAW_END.complete=false`, a wrong event count, duplicate footer, records after
the footer and duplicate JSON keys. V0.1 now parses the raw envelope before
normalization, requires exactly one header and final complete footer, checks the
declared count and contiguous sequence, validates the declared two epochs and
passes every event through the existing sealed-event validator. A prefix ending
at a valid `EPOCH_END` is rejected because the receipt declares two epochs.

P1-B was fixed with `receipt.py` and `capture/run_v01.ps1`. The receipt is
created before launch and finalized only after raw/config/launch/result hashes
exist. It separately identifies the Lua collector, normalizer, harness,
EmuHawk/core, config, ordered watch plan, mode, reverse flag, ROM, state and
scenario. Normalization requires the receipt, exact raw path and raw hash, and
an exact raw-header identity match. The collector hash in the corrected
captures is `05834fae...629d829`; the normalizer hash is a different
`a071c12d...c502fb0`.

P1-C was fixed by making `report.py` derive claims from capture witnesses and
hashed API source receipts. Empty evidence produces no `PROVEN` capability;
wrong SAT/fields prevent the input/oracle conclusion; missing or equal reverse
hook controls prevent `HOOK_ORDER=PROVEN`. Every result carries witness names
or an empty witness list.

## Raw completion and identity contract

The raw schema is `thor.evidence.raw.v0.1`. Its first record is an exact
`RAW_HEADER` containing capture ID, receipt hash, ROM/state identity, mode,
reverse flag and ordered watch-plan hash. Its last record is an exact complete
`RAW_END` with the event count. JSON duplicate keys are rejected by the same
object-pairs validator used by the normalizer. A valid transport seal proves
transport and declared source completion only; it does not prove causal or
runtime completeness.

Historical V0 artifacts are classified
`VALID_ONLY_FOR_V0_HISTORICAL_EVIDENCE` because their old capture schema and
collector identity cannot be reconstructed truthfully. They were not mutated.
Three repeats of the same bounded V0 probe were run with the corrected receipt
and the final watch plan (including the input-poll callback):

| capture | classification | raw SHA256 | event stream SHA256 |
|---|---|---|---|
| v01r3-probe-a | VALID_FOR_V1 | see capability matrix | `825bc70e...f347748` |
| v01r3-probe-b | VALID_FOR_V1 | see capability matrix | `825bc70e...f347748` |
| v01r3-reverse | VALID_FOR_V1 | see capability matrix | `85e5ae3c...5013ff` |

Raw headers differ by launch receipt identity as intended. Event streams of the
two normal probes are identical; reversed hook installation changes the event
stream. All three preserve SAT `0088090187810088`, fields `00100080`, six A372
execution hits and one FF13CC write per epoch. Instrumentation remains bounded
and no guest memory/register writes were introduced.

## V1 FF13CC precondition matrix

| Future obligation | V0.1 status | Boundary |
|---|---|---|
| exact instruction instances | SAFE_ALTERNATIVE | selected EXEC witnesses plus checked decoder path can identify instances |
| A372 to FF13CC pairing | BLOCKED | A374 is preserved as callback PC; no PC-minus-two rule |
| selected operation width | SAFE_ALTERNATIVE | obtain from checked static instruction form and validated pairing |
| relevant overlapping-writer completeness | BLOCKED | exact hooks do not establish absence of overlapping stores |
| interruption boundary | BLOCKED | affected proof must detect or bound interruption |
| generic width/overlap outside canary | UNKNOWN_ALLOWED | outside declared local scope |
| generic same-value writers | UNKNOWN_ALLOWED | no last-writer inference permitted |
| global IRQ/exception model | UNKNOWN_ALLOWED | only selected pairing boundary matters |
| causal input reads | UNKNOWN_ALLOWED | neutral/Right remains a negative control |
| savestate provenance and DMA/VRAM continuation | UNKNOWN_ALLOWED | explicit frontier outside FF13CC version proof |

Therefore V0.1 does not authorize V1. It establishes the corrected storage and
capture boundary and identifies the two real local blockers: A372/write pairing
and relevant-write completeness, with interruption handling required for the
affected proof. It does not implement register propagation, RAM lifetime,
last-writer inference or a causal graph.

## Tests and validation

`tests/thor_evidence_v0_test.py` passes 8/8 focused tests. The fixtures cover
all listed envelope attacks, wrong collector/mode/watch identity, missing
capability witnesses, wrong oracle, absent/equal reverse control, rollback and
retry, repeated equal values across epochs, backward temporal links and the
negative case where the original ROM low byte differs from incremented D5.low8.
The corrected report payload is emitted at ignored
`build/thor-evidence/v0/capability_matrix_v01.json`; its controls report raw
event determinism, normalized event determinism, changed reverse order, oracle
match and ownership delta zero.

The existing Release and GNU/Linux-equivalent builds and focused CTest remain
green; the focused V0.1 CTest is registered beside the V0 test. The known
pre-existing full Debug MSVC failure in `src/core/ram_flag_routine.cpp`
(`std::to_string`) remains outside this repair. `git diff --check` and the
tracked-source size check are required before publication. `SOURCE_OWNED` is
unchanged at `1,475,368 / 3,145,728`, delta `0`.

## Decision quality and STOP

The repair closes the three P1 foundation defects without weakening any
UNKNOWN. It provides truthful V0.1 captures and a checked report while keeping
causal provenance, ownership promotion, new ROM investigation and M13 outside
the stage. STOP after publication and CI; V1 requires a separately authorized
bounded work package for the two local blockers above.

The most dangerous missing negative fixture remains a syntactically valid,
complete two-epoch stream with an omitted overlapping writer or forged causal
edge. V0.1 intentionally does not infer either relation; V1 must add that
fixture before promoting any FF13CC provenance claim.
