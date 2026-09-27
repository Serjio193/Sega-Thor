# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `b37d9c4c81386121f6afda6e661e1661388ec99eba322add890bdc6bf4a63ff2`;
master graph `2064177834cec8589c1c7ccfaa5a388206412e184e791b250405d888ab7ae518` → `1a4bf0b11eef9ba3584625ac0faf5d0030adc8f99cf434cfb13d1d021c5cb5c7`.
MAP-1 nodes: 464 → 485; edges: 476 → 497; conflicts: 0.
Session artifact SHA-256: `6729aae5d862da39631aa4b9ed8f9ba37b87c0965630333af925ab7f9156f4d6`; receipt `455369980ef93df072504bf3ab498cdb858799d202a720bdef6b31b3db0f3c3e`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 5,423 | 5,433 | +10 |
| claim | 7,961 | 7,971 | +10 |
| relation | 2,310 | 2,319 | +9 |
| evidence_ref | 61,035 | 61,453 | +418 |

Runtime import added 10 objects, 10 claims, 9 relations and 418 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Executed instruction objects: 1,930 → 1,940; runtime occurrence references: 3,954,888 → 3,954,888.
Canonical relations: `EXECUTED_NEXT` 2,085 → 2,094; `OBSERVED_NEXT_PC` 173 → 173. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 2 edges / 20 occurrences.

`SOURCE_OWNED`: 1,487,672 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56678, 'ASSET': 1085110, 'DATA': 245464, 'INCBIN': 1758476}`.
Hashes before → after: structure `5cc2963dbb0daf6ce9747b930bb9d2be2e21d483c0b2ded03cfbe6b6b81d9f7c` → `fbcfd513709c0c493466e6341ae3523f84370cf9fb7566bd89f6366ee82d1a56`; evidence `8e3f90f87070449a927a08e0e3e8656a788c9512dada0ad5434d91cf97215171` → `cfcc9ee8e3808a514ef838cc50dbb0335461862815bf9ae005016d22cdd3c9d5`; emission `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` → `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a`; combined `81b81b2affb72effcb056506a85762c875253785311cf67c9d588cb78bd048e6` → `2535c5113dc4a6b0a0d95260632dbac66fb29a49a8ee643ad3d2525691cafbff`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 6.615/16.264/11.233 s; knowledge DB 309,293,056 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
