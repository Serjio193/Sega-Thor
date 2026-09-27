# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `23091822242c219dcb10c94cef24cda2f95e42a94988464bf1ace9998b8f0b75`;
master graph `1a4bf0b11eef9ba3584625ac0faf5d0030adc8f99cf434cfb13d1d021c5cb5c7` → `d5bb5c902f2d875c8d1ad4d56d77a474438172bbf8541d980bbf882dda60b02f`.
MAP-1 nodes: 485 → 486; edges: 497 → 498; conflicts: 0.
Session artifact SHA-256: `b39c0e9c75ea375e6c37815fef7695cea1eaa7e0de55d283c3ccaf8606eeda3d`; receipt `132cb5d896ed872f2e1f9d02e64771104068ed33dc247eb7909c369b7eaaaf69`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 5,433 | 5,433 | +0 |
| claim | 7,971 | 7,971 | +0 |
| relation | 2,319 | 2,320 | +1 |
| evidence_ref | 61,453 | 62,108 | +655 |

Runtime import added 0 objects, 0 claims, 1 relations and 655 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Executed instruction objects: 1,940 → 1,940; runtime occurrence references: 3,954,888 → 3,954,888.
Canonical relations: `EXECUTED_NEXT` 2,094 → 2,094; `OBSERVED_NEXT_PC` 173 → 174. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 2 edges / 14 occurrences.

`SOURCE_OWNED`: 1,487,672 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56678, 'ASSET': 1085110, 'DATA': 245464, 'INCBIN': 1758476}`.
Hashes before → after: structure `fbcfd513709c0c493466e6341ae3523f84370cf9fb7566bd89f6366ee82d1a56` → `4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc`; evidence `cfcc9ee8e3808a514ef838cc50dbb0335461862815bf9ae005016d22cdd3c9d5` → `5b3c3ade7f9b48542fd044bdb24e8aa9b55ce54742350590f4e4a6248441b487`; emission `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` → `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a`; combined `2535c5113dc4a6b0a0d95260632dbac66fb29a49a8ee643ad3d2525691cafbff` → `aae5653373ec6ea3ba61851ca3ef342018e258904e84e53bb1076b69042706b1`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 6.313/16.583/11.704 s; knowledge DB 313,364,480 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
