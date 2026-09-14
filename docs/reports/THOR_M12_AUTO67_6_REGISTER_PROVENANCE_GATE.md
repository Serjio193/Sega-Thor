# THOR M12 — AUTO67.6 register provenance gate

Status: **NEGATIVE — FAIL CLOSED**

Baseline: `44a627ead299348f701e4e25f5bf17cb496776a8`

No runtime code was changed. The current AUTO67 capsule path does not contain
enough evidence to resolve an A4/A5 reaching definition, so no `chain_step`
was emitted and no producer PC was invented.

## Exact current capsule contract

The live frozen capsule is `O67V` version 2. Its physical header is 24 bytes;
each record is five little-endian uint32 values, 20 bytes total:

```text
sequence | frame | address | pc | kind_code
```

It contains no register values, opcode, execution epoch, instruction-occurrence
identity, or complete predecessor interval. `BUS_WRITE_PC` and `BUS_EXEC_PC`
therefore cannot establish which definition of A4 or A5 reaches a consumer.

## Primary canary

AUTO67.5 statically proved:

```text
BUS_WRITE_PC pc=0x0027EC address=0xC00004
0x0027EC: MOVE.W (A5),-4(A4)
```

The real 180-frame BizHawk run observed this canary at frames 90 and 120.
The frozen capsules remained O67V v2 and supplied zero register-context or
producer-instruction records. The result is:

```text
producer instruction: UNKNOWN
register version:      UNKNOWN
consumer instruction: 0x0027EC
chain_steps:           []
frontier:              REGISTER_PROVENANCE
```

The full AUTO67.5 materialized canary payload, including its eight observed
facts, two static causal facts, empty `chain_steps`, frontier and hash, is
preserved in the JSON report under `full_auto67_5_canary_payload`.

Temporal adjacency, same-frame occurrence, or the register name alone is not
causal evidence. The exact missing proof is a bounded predecessor execution
trace with A4/A5 values, opcode, epoch, sequence and a completeness/no-gap
witness.

## Existing evidence to reuse

`capture/focused_register_slice.lua` and `capture/dense.lua` already show the
usable BizHawk mechanism: `event.on_bus_exec_any` plus `emu.getregister`.
They are separate raw capture paths, not inputs to the AUTO67 frozen capsule
worker. Reusing that mechanism in a versioned targeted capsule is the smallest
future repair; this checkpoint intentionally does not implement it.

## Short real BizHawk proof

Artifact: `build/auto67-6-negative-proof.json`

- 180 frames;
- 16 workers;
- 425 leases and 425 returns;
- 54 observed facts;
- 0 causal facts;
- 0 chain steps;
- register provenance resolved: 0;
- canary register provenance unresolved: 1;
- unsupported facts: 0;
- queue drops: 0;
- DB errors: 0;
- raw backlog: `NONEXISTENT`;
- emulator return code: 0.

The short run did not start long gameplay, graph merging, AUTO68, or ownership
promotion. The historical AUTO67.4 database was not modified.

## Validation

- `tests/thor_evidence_auto67_4_test.py -q`: 8 passed;
- `tests/thor_evidence_auto67_3_test.py -q`: 3 passed;
- Python compilation: passed;
- real BizHawk short proof: passed with the negative gate above.

The resolver-specific positive/negative matrix is intentionally not claimed:
there is no resolver implementation to test until the targeted runtime evidence
contract is added under a separately authorized AUTO67.6 repair step.
