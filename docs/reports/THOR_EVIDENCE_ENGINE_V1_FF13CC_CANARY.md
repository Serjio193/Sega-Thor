# THOR Evidence Engine V1 — FF13CC causal-capability canary

Status: **PROVEN for the bounded canary slice**. This report is the V1
baseline for the first engine-derived provenance query. It does not promote
ROM ownership, implement V2, or claim general causal completeness.

BASELINE SHA: `c74e68d4a9c8013ca42891e8a5aead865386f91f`.
FINAL SHA (implementation commit): `b940a2b017afb3d0147108635a1823866f0cf9ff`.
The dynamic capture is the sealed two-restore canary run:

| Artifact | Identity |
|---|---|
| ROM | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| QuickSave state | `7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970` |
| capture id | `b8e929ef-b07e-441e-a06a-ea591b402424` |
| raw SHA256 | `e69c73e6c71677066d6a711d5ab1052a27f112fb43f35d243a138b50f499b09` |
| receipt SHA256 field | `155da3a3d112696805f3dc005d14dc2239ba9db50442697c210fea655abf6657` |
| checked static producer SHA256 | `33ab59edbb3d9391f834e4848fabb6003dd7e7181cdaaa2182137d584de82e421` |
| engine certificate SHA256 | `af8dafbf276a527df0e589ca876803c6ed3261d20de65b1a4b4229ae31f1b0e5` |

The capture contains two complete restore epochs. In epoch 1 the selected
call is bounded by EXEC `A342` through `A436`; six independent `A372` EXEC
instances occur at raw sequences `17, 32, 47, 62, 77, 92`. The first output
write is raw sequence `18`, paired through its explicit `exec_seq=17` witness.
No PC-2 rule is used; the callback PC is retained as observation metadata.

## Proven query

The engine derives and checks these operations from the checked static JSON
and dynamic witnesses:

```
selector = RAM[FF1858] == 0
root     = selector ? A438 : A480
offset   = sign_extend16(RAM[FF188C])
dest     = FF13CC + offset
old      = RAM.word[FF188A]
counter  = (old.low8 + 1) & FF
record   = ROM.long[root + 8*i]
D2       = (record & FFFFFF00) | counter
RAM.long[dest] = D2
```

For the selected epoch the witnesses are selector `0`, offset `0`, old
counter `0`, ROM read `A438 = 00880901`, destination `FF13CC`, and write value
`00880901`. The dependency graph has separate temporal versions for epochs,
execution instances, RAM/register slices, ROM bytes, and the output. The
resulting query proves:

* D2 high 24 bits come from ROM bytes `A438..A43A`.
* D2 low 8 bits come from `FF188A.low8 -> D5.low8 -> addq.b #1 -> D2.low8`.
* ROM byte `A43B` is absent from the low-byte dependency set.
* `FF188C` is an ADDRESS dependency with checked sign extension; it is not a
  value/provenance edge.
* `FF1858` is a CONTROL dependency selecting `A438`; it is not a value edge.
* The output has both VALUE and ADDRESS witnesses for the `A372` write.

The SQLite sidecar persists `operation_instance` and
`provenance_dependency` rows in one transaction. Re-importing the same
certificate is idempotent; a differing payload with an existing identity is a
hard failure.

## Temporal graph model

Each version identity includes trace, epoch, location, bit slice, and the
producing event/operation. Equal RAM values in another epoch cannot alias this
version. Each operation includes epoch and EXEC sequence, so the six `A372`
executions remain six instances. A partial write creates a new slice version
and preserves only slices allowed by its checked rule.

## Supported and unsupported M68K rules

The bounded rule set is `LEA absolute`, sign-extending `ADDA.W`, `MOVE.W`
RAM-to-D5, `TST.B`, `BEQ`, `MOVE.L (A0)+,D2`, `ADDQ.B #1,D5`, `MOVE.B D5,D2`,
`MOVE.L D2,(A5)+`, and the checked byte-slice merge. Unsupported forms,
unknown transforms, missing witnesses, interrupt boundaries, guessed widths,
and generic last-writer inference cannot enter a PROVEN edge.

## Engine-derived chain and roots

ROM root is selector-controlled `A438`; bytes `A438..A43A` feed high24 and
`A43B` is excluded. RAM roots are `FF188A` and `FF188C`, each epoch-local.
`FF1858` is the CONTROL root selecting `A438`. `A5=FF13CC` plus the checked
sign-extended `FF188C` gives the ADDRESS root `FF13CC`. The selected interval
has 70 EXEC witnesses and six distinct `A372` instances. VALUE, ADDRESS and
CONTROL roles are retained separately.

Example `explain` output (abbreviated):

```
cde937... RAM/FF13CC 00880901 a372-write-output
  <- VALUE PROVEN MOVE_LONG_D2_TO_RAM witness=17
    7a86f1... REGISTER/D2 00880901 byte-slice-merge
      <- VALUE PROVEN MERGE_HIGH24 witness=17
        2e57f8... REGISTER/D2 00880900 move-long-preserved-high24
          <- VALUE PROVEN ROM_HIGH24_BYTE witness=13
      <- VALUE PROVEN MERGE_LOW8 witness=17
        a8c372... REGISTER/D2 00000001 move-byte-counter
          <- VALUE PROVEN MOVE_BYTE_D5_TO_D2_LOW8 witness=16
```

## Acceptance matrix

| ID | Result | Evidence |
|---|---|---|
| C01 | PASS | sealed raw envelope, receipt binding, canonical ROM identity |
| C02 | PASS | two epochs; version IDs include epoch and event instance |
| C03 | PASS | six distinct `A372` operation instances and output versions |
| C04 | PASS | checked static forms and explicit EXEC/READ/WRITE witnesses |
| C05 | PASS | `FF188C` sign-extended ADDRESS transform |
| C06 | PASS | `FF1858` CONTROL branch selects `A438` |
| C07 | PASS | writer sequence observed as `0030/0006`, `0060/000C`, `0080/0010` in the prior accepted field trace; no completeness claim |
| C08 | PASS | high24/low8 slice proof reaches `00880901` |
| C09 | PASS | no guessed width, last-writer, address-only, or PC-2 edge |
| C10 | PASS | the accepted negative input-poll observation is retained; no INPUT causal edge is emitted |
| C11 | PASS | SAT/DMA remains a separate structural join; no same-frame causal edge is claimed |
| C12 | PASS | certificate, explain query, SQLite idempotence, and negative fixtures |

## Explicit frontier

`access_width`, overlap/range semantics, same-value writer completeness,
IRQ/exception interaction, and input-read causality remain `UNKNOWN`. They are
not silently converted to facts by storage or normalization. They do not block
this exact FF13CC canary because the proof uses a checked long write, a zero
offset, a single selected root, and an explicitly negative input-causal result.
They remain required capabilities for any broader V1 query that needs them.

The missing negative fixture that would be most dangerous without this slice
is a forged low-byte edge from ROM `A43B` (or an address-equality edge from an
unrelated epoch). Both are rejected by the certificate validator and are
represented in `tests/fixtures/thor_evidence_v1_canary/negative_cases.json`.

## CRITICAL FINDINGS

None for the bounded canary. The engine rejects cross-epoch merges, forged
edges, ROM low-byte provenance, unknown transforms, and PC-2 inference.

## NON-BLOCKING FINDINGS

The current canary capture has no positive INPUT event; the accepted negative
input-poll evidence is therefore carried as an explicit no-edge result. The
writer sequence is observed evidence, not a completeness proof.

## V1 REQUIRED CAPABILITIES

For this canary: sealed receipt-bound capture; checked static forms; epoch and
execution-instance identities; RAM/register bit-slice versions; explicit
VALUE/ADDRESS/CONTROL roles; ROM high24 and counter-low8 transforms; and
idempotent SQLite import.

## V1 ALLOWED UNKNOWNS

Access width outside the checked long store, overlap/range semantics outside
the selected destination, same-value writer completeness, IRQ/exception
interaction, and input-read causality may remain UNKNOWN at this boundary.

## TESTS MISSING

No missing test blocks this canary. The next bounded slice must add a positive
INPUT witness before claiming input causality and a separate interruption
fixture before claiming IRQ completeness.

## Files and verification

The bounded implementation is in
`src/tools/thor_evidence/canary_engine.py`; capture is in
`src/tools/thor_evidence/capture/canary.lua` and `run_canary.ps1`; temporal
relations are persisted by the existing `Store` sidecar schema. The helper
test is `tests/thor_evidence_v1_canary_test.py` and is registered with CTest.

This gate does not authorize new ROM reverse engineering, source ownership,
or a second canary. The next V1 work may consume this certificate and add only
the smallest additional causal slice whose static forms and witnesses are
available.

## TESTS / CI / DECISION QUALITY / STOP REASON

The V0, V1-gate, dense-gate and V1-canary helpers pass, as does the Release
CTest registration. Release `oasis_smoke` builds. The existing full Debug
MSVC build remains blocked by the pre-existing `std::to_string` error in
`src/core/ram_flag_routine.cpp`; no unrelated fix was made. A Linux-equivalent
build directory was not available in this Windows session. Decision quality is
bounded: the certificate is generated from static and dynamic evidence and an
oracle is used only for the observed expected value check. STOP REASON:
FF13CC canary C01-C12 are closed; do not start V2 or broaden the slice.
