# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `d3d8a6fcb4d7e7d81b0fdf34e468944baa56ef33a714d905b19d6254fe6fdf7d`;
master graph `181fd856728810852952a54dc5e601466388716fc4b6e332f082e7304158f11f` → `2064177834cec8589c1c7ccfaa5a388206412e184e791b250405d888ab7ae518`.
MAP-1 nodes: 464 → 464; edges: 476 → 476; conflicts: 0.
Session artifact SHA-256: `8f63fcd9a150b2159d162b66915f5e042f447f6a9c9ad78b3548f85a44796cb0`; receipt `3e6bb0d5a0da173cbc27b787a661c9306fb8e18d8d8376e07941b6bcee806ffd`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 5,423 | 5,423 | +0 |
| claim | 7,961 | 7,961 | +0 |
| relation | 2,310 | 2,310 | +0 |
| evidence_ref | 60,627 | 61,035 | +408 |

Runtime import added 0 objects, 0 claims, 0 relations and 408 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Executed instruction objects: 1,930 → 1,930; runtime occurrence references: 3,954,888 → 3,954,888.
Canonical relations: `EXECUTED_NEXT` 2,085 → 2,085; `OBSERVED_NEXT_PC` 173 → 173. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 2 edges / 16 occurrences.

`SOURCE_OWNED`: 1,487,672 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56678, 'ASSET': 1085110, 'DATA': 245464, 'INCBIN': 1758476}`.
Hashes before → after: structure `5cc2963dbb0daf6ce9747b930bb9d2be2e21d483c0b2ded03cfbe6b6b81d9f7c` → `5cc2963dbb0daf6ce9747b930bb9d2be2e21d483c0b2ded03cfbe6b6b81d9f7c`; evidence `bf4642fb2f422fe7bc944aef0ea7e4646284699d2cd7c0beee94edf9ff691157` → `8e3f90f87070449a927a08e0e3e8656a788c9512dada0ad5434d91cf97215171`; emission `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` → `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a`; combined `22131fd7b90bdb9d0cc6f9f97114fda6b69743e2cb32b66831fcb89a42d655d7` → `81b81b2affb72effcb056506a85762c875253785311cf67c9d588cb78bd048e6`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 6.666/16.466/11.368 s; knowledge DB 306,180,096 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
