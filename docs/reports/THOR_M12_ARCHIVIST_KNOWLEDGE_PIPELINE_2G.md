# M12 Archivist → Canonical Knowledge Pipeline 2G

Status: `PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1`.

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Archivist merge: `MERGE`; session graph `8b5e3818e6c93cd11387d2efcade01b00846b000f8b01a2f3e4f7e98fbd6e516`;
master graph `d20c67773beecb238c01f48e5ff7d7bfa57a19203aa2cdcdd0c84bf1bec317cb` → `cbe983b4d4f7019faa6904ddb2e8eaffa1533dc38922360bf47537fbaf402dc9`.
MAP-1 nodes: 369 → 718; edges: 406 → 762; conflicts: 0.
Session artifact SHA-256: `ad80876f1056ff52319ee5194b345036e2e41497621eac082bbfc76a5042f5bc`; receipt `2ca7c5b81ab339343436e7deeaf86909bf70446abe494c386f5ff3a5e2391538`.

| Measure | Before | After | Delta |
| --- | ---: | ---: | ---: |
| rom_object | 3,794 | 3,824 | +30 |
| claim | 6,259 | 6,289 | +30 |
| relation | 364 | 400 | +36 |
| evidence_ref | 7,968 | 9,476 | +1,508 |

Runtime import added 30 objects, 30 claims, 36 relations and 1,508 evidence references. Duplicate import added zero rows and preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.

Canonical runtime proof: `PASS_INDEPENDENT_ROM_RANGE_AUDIT`, 16 Workers × 100 cycles, 1,600/1,600 segments independently audited; 1,600 unique captures, 195,547 instruction occurrences, 1,600 terminal address facts; runtime SOURCE_OWNED delta +0.

Executed instruction objects: 330 → 360; runtime occurrence references: 199,630 → 395,177.
Canonical relations: `EXECUTED_NEXT` 338 → 369; `OBSERVED_NEXT_PC` 26 → 31. 2E pointer/offset/table relations: {'OBSERVED_CODE_POINTER_TO': 0, 'OBSERVED_CODE_OFFSET_TO': 0, 'OBSERVED_JUMP_TABLE_ENTRY_TO': 0}.
Exception endpoint edges excluded from instruction adjacency: 43 edges / 1,702 occurrences.

`SOURCE_OWNED`: 1,475,600 bytes; delta `0`. Emission bytes by type are unchanged: `{'ASM': 56134, 'ASSET': 1085110, 'DATA': 233676, 'INCBIN': 1770808}`.
Hashes before → after: structure `7f9c95f8f05e29db54991e3f6abee94e4fd80d5013fbd0675f5abe9defdc18fe` → `cf2cba9073b51d0822d96648f647086c5d0544da29efde080aec3085af6a02de`; evidence `cb43a5fdfa5a1f592a8f1fac30cdab6a88052ac1719ae872ba25058eb4a69364` → `2436fc163aec5959ac6f0e57aa39a2fe0daa063d9a26528b4192ee895a5b2677`; emission `d76325cfa6d6312f28333f0633f74260214e5ae8a92fc2984fcc565872cbda8f` → `d76325cfa6d6312f28333f0633f74260214e5ae8a92fc2984fcc565872cbda8f`; combined `589e97fe2e070ab7f9a71a3439e19f3b965a01d24ce82b2bd42cffc9f129232a` → `80828f5c178b5e7373e6530c11f98c02578aff43ab4661eb7c15b1724ff23d1f`.
Independent audit: `PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1`. Post-run processing only; CPU runtime overhead from 2G is 0. Archive/import/audit: 109.534/84.872/43.740 s; knowledge DB 10,764,288 bytes.

SQLite generations and runtime/session artifacts remain under ignored `build/`; this report and its compact JSON receipt contain no raw FLOW or lineage arrays.
