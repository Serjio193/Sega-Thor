# THOR Evidence Engine V1-GATE — local FF13CC capability closure

**BASELINE SHA:** `81f910415498f20702c790b3155985c57172375e`
**FINAL SHA:** recorded in the final publication delivery

This is a local capability gate only. It does not implement V1 provenance,
last-writer analysis, a slicer, register provenance, scheduler logic, new ROM
discovery, ownership promotion, 03BDA6/03BDD8, M13, or C++ migration.

## A372 static instruction certificate

Canonical ROM identity is size `0x300000`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The existing checked static producer contract and decoder source were reused;
their hashes are:

| evidence | SHA-256 |
|---|---|
| `src/tools/m12_a372_shadow_sat_producer.py` | `588450d3abe1531e186758091ea4cfb7d04692ea2bccb26bb61671f9008ab788` |
| `src/tools/re_slice_decoder.cpp` | `00457dfb5f1cb3e91805ea0545f13035879cf961b7ebf9669a51abef90d25109` |

The exact form is:

| field | value |
|---|---|
| address | `0x00A372` |
| bytes | `2A C2` |
| form | `MOVE.L D2,(A5)+` |
| width | 4 bytes |
| effective address | A5 postincrement |
| length | 2 bytes |
| next PC | `0x00A374` |

The width comes from the checked instruction form, not callback flags.

## Runtime pairing certificate

The selected instance is the first `window=test`, `epoch=1` A372 execution in
the corrected V0.1 raw capture
`build/thor-evidence/v0/v01r3-probe-a.raw.jsonl`.

| field | value |
|---|---|
| trace SHA-256 | `7b80d5d7999f4623f193a07aa38120ab20a76c6fbbc6087cf901d149a2f1c5f8` |
| epoch | `1` |
| execution instance | `epoch-1-seq-117` |
| pre/EXEC event | seq `117`, PC `0x00A372`, A5=`0xFF13CC`, D2=`00880901` |
| write event | seq `118`, callback PC `0x00A374`, destination `0xFF13CC`, value `00880901` |
| post/next boundary | seq `120`, PC `0x00A374`, A5=`0xFF13D0` |
| raw witnesses | seq `[117,118,120]`, schema `thor.evidence.raw.v0.1` |

The pairing is established by ordered event identity, checked `MOVE.L` semantics,
the pre-write A5/D2 state, the four-byte write value, and postincremented A5.
It does not use a universal `callback_pc - 2` rule. The callback PC is retained
as observed.

**PAIRING: PROVEN for this selected instance.** This is a local execution
certificate, not a general last-writer or causal graph.

## Writer coverage certificate

The selected local interval is raw sequence `117..120`. V0.1 captures exact
EXEC/READ/WRITE hook addresses; they do not contain every executed instruction
in the interval. Therefore the executed instruction count and complete memory
effect classification are unavailable.

| obligation | result |
|---|---|
| dense instruction coverage | unavailable |
| `NO_MEMORY_WRITE` classification for every executed instruction | unavailable |
| `WRITE_DISJOINT` classification | unavailable |
| `WRITE_TARGET_OVERLAP` enumeration | unavailable beyond selected hook |
| `UNKNOWN_MEMORY_EFFECT` exclusion | not proven |
| same-value overwrite retention | enforced by validator, not measured in runtime interval |

**WRITER COMPLETENESS: BLOCKED.** An exact-address hook miss is never treated
as evidence that an overlapping writer did not execute.

## Interruption certificate

The V0.1 raw stream has no dense control-flow or vector/stack discontinuity
certificate for the selected interval. No claim of “no interruption” is made,
and no handler instruction set is available for inclusion.

**INTERRUPTION: BLOCKED / UNKNOWN.**

## Four-byte value version

The bounded validator creates four distinct byte versions for the selected
`MOVE.L` instance. Each version contains trace, epoch, execution instance,
decoded operation, byte offset, destination, byte value, pre/post state and raw
witness sequence. Overlapping byte versions remain separate. No upstream D2
provenance is derived.

**VALUE VERSION: PROVEN as a local representation contract.**

## Adversarial negative fixtures

`tests/fixtures/thor_evidence_v1_gate/negative_cases.json` and
`tests/thor_evidence_v1_gate_test.py` cover:

- omitted overlapping writer in an otherwise complete-looking two-epoch case;
- forged A372 → FF13CC pairing;
- shifted callback PC without validated pairing;
- unknown memory-effect instruction;
- interruption/discontinuity without sufficient coverage;
- one-byte overlap, retained as a writer;
- same-value overwrite, retained as a writer.

The first five cases refuse `PROVEN`. The last two are accepted as explicit
writer classifications when dense completeness flags are present, proving that
one-byte and same-value writes cannot disappear from the coverage model.

## V1 readiness and stop

| obligation | status |
|---|---|
| exact A372 form/width | PROVEN |
| execution-instance identity | PROVEN for selected capture instance |
| A372 → concrete memory effect pairing | PROVEN for selected instance |
| effective destination | PROVEN for selected instance |
| four byte versions | PROVEN as representation |
| local relevant-writer completeness | BLOCKED |
| same-value writes cannot disappear | PROVEN by checker contract |
| interruption boundary | BLOCKED |
| adversarial omitted-writer rejection | PROVEN |
| forged causal-edge rejection | PROVEN |

**V1 READINESS: BLOCKED.** The fixed point is reached after the bounded local
evidence available here: pairing closes, but dense writer coverage and an
interruption boundary do not. No second materially equivalent runtime probe is
justified because the missing capability is coverage shape, not a changed
scenario result.

`SOURCE_OWNED` before/after is `1,475,368 / 3,145,728`; delta `0`.

## Tests, CI and decision quality

The new gate test is CTest-integrated as
`oasis_thor_evidence_v1_gate_helpers`. Existing V0/V0.1 tests remain required.
The exact final SHA and CI run are recorded in the final delivery after
publication. The known full Debug MSVC `std::to_string` failure remains outside
this bounded developer-only gate.

The result is **PARTIAL / V1 BLOCKED**, followed by a hard STOP. Even after a
future closure of writer completeness and interruption, implementing V1 still
requires separate authorization.
