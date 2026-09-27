# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `538484c927897038646a4b4325a060cfa6926ea5a642b3020c71633b635f6257`;
master graph `d5bb5c902f2d875c8d1ad4d56d77a474438172bbf8541d980bbf882dda60b02f` → `e5de3f624cb9eee18d8e5f410a596fa58c09b3d8db13297a19a9fd83d1a61f66`.
MAP-1 nodes: 486 → 486; edges: 498 → 498; conflicts: 0.
Session artifact SHA-256: `6f8777084fd64470b9d94a57165da460dcc098099633683754d505d27ffadb52`; receipt `932b1ea5d46e4757f52861350418d11c91d70609ea7efdc97b348db058e50edd`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 5,433 | 5,433 | +0 |
| claim | 7,971 | 7,971 | +0 |
| relation | 2,320 | 2,320 | +0 |
| evidence_ref | 62,108 | 62,164 | +56 |

Runtime import added 0 objects, 0 claims, 0 relations and 56 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Executed instruction objects: 1,940 → 1,940; runtime occurrence references: 3,954,888 → 3,954,888.
Canonical relations: `EXECUTED_NEXT` 2,094 → 2,094; `OBSERVED_NEXT_PC` 174 → 174. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 2 edges / 14 occurrences.

`SOURCE_OWNED`: 1,487,672 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56678, 'ASSET': 1085110, 'DATA': 245464, 'INCBIN': 1758476}`.
Hashes before → after: structure `4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc` → `4b77d26778e52a620602c556e25ecba3382c8a309f13a7ab27525a6114d341bc`; evidence `5b3c3ade7f9b48542fd044bdb24e8aa9b55ce54742350590f4e4a6248441b487` → `d068e7c393db47101f906dd05f09515e1819b0d36bcb2f2a281ff117970c900f`; emission `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a` → `4c8a1951a65a5683012f25cd037c918dcbeaad2b3806a5deaa6d8b5d8e9e317a`; combined `aae5653373ec6ea3ba61851ca3ef342018e258904e84e53bb1076b69042706b1` → `2a9dd405b6670404df7b93f955f18e741fb3edb3a8c10507afa3e7d4f6c40e91`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 7.830/21.301/15.634 s; knowledge DB 316,612,608 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
