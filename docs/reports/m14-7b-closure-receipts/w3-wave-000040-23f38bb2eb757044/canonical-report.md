# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `56fa3140c05d1dcef3e2896f06d0161c4747334a8ae72c3e0fb2a80da0099860`;
master graph `228a2b1c1eb338fc85bb3e3127af7c1ccf9e4dcdc9f56970b5212ecb4aa79cca` → `181fd856728810852952a54dc5e601466388716fc4b6e332f082e7304158f11f`.
MAP-1 nodes: 464 → 464; edges: 476 → 476; conflicts: 0.
Session artifact SHA-256: `f41f2078c2f7bfcd09e9239bd1d0312c9ebdeac081b01876d52e395681a21bd0`; receipt `47cde882f7cb213bdbbd331356e362b7c221cbb31f4ddff18349bede8b3c3087`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 5,423 | 5,423 | +0 |
| claim | 7,961 | 7,961 | +0 |
| relation | 2,310 | 2,310 | +0 |
| evidence_ref | 60,220 | 60,627 | +407 |

Runtime import added 0 objects, 0 claims, 0 relations and 407 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Executed instruction objects: 1,930 → 1,930; runtime occurrence references: 3,954,888 → 3,954,888.
Canonical relations: `EXECUTED_NEXT` 2,085 → 2,085; `OBSERVED_NEXT_PC` 173 → 173. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 2 edges / 16 occurrences.

`SOURCE_OWNED`: 1,487,672 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56678, 'ASSET': 1085110, 'DATA': 245464, 'INCBIN': 1758476}`.
Hashes before → after: structure `5cc2963dbb0daf6ce9747b930bb9d2be2e21d483c0b2ded03cfbe6b6b81d9f7c` → `5cc2963dbb0daf6ce9747b930bb9d2be2e21d483c0b2ded03cfbe6b6b81d9f7c`; evidence `39e959a68db0111f5a352da5ff2ebb4a7605e6d05800e8965e5e423c78ac5e61` → `bf4642fb2f422fe7bc944aef0ea7e4646284699d2cd7c0beee94edf9ff691157`; emission `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` → `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a`; combined `ed285391323a56549cbb203258f78af720a9a61830a9011debeed185ed9333e1` → `22131fd7b90bdb9d0cc6f9f97114fda6b69743e2cb32b66831fcb89a42d655d7`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 7.949/20.197/14.599 s; knowledge DB 303,366,144 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
