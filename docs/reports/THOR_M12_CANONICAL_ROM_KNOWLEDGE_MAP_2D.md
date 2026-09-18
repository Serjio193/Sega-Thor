# M12 Canonical ROM Knowledge Map 2D

Status: `PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1`.

The map imports the accepted AUTO61 ownership manifest, exact 2B runtime linkage, and
Carver-5 format candidates as hypotheses. It keeps unknown classifications, runtime
observations, source ownership, and emission intervals as separate records. No new runtime
campaign or instrumentation was used. Raw FLOW/session data stays under ignored `build/`.

## SOURCE_OWNED reconciliation

AUTO60 and AUTO61 are different accepted checkpoints, not competing snapshots of the same
state. Exact intervening promotions explain the full delta. The old Carver-1..5 corpus is
still a valid historical snapshot of AUTO60; it is stale as the current ownership base.

| Checkpoint | SOURCE_OWNED bytes | Maximal intervals | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| AUTO60 | 1,427,873 | 759 | `942cc777e71d5ce2894e28ff51a0d1f3c8f78b27c77acf37baf34cec3ff2706b` |
| GFX2 | 1,475,262 | 758 | `f9892bdfcb038a06bd993fd67fdec0e675fca1a937137c4da5e834e7f22353c0` |
| GFXMAX | 1,475,368 | 759 | `996ce8100ff8354d682d217b34b7c61bd7e5cad49ec5a4cb5373f69ef76be70b` |
| AUTO61 | 1,475,600 | 759 | `c77b985d0b9041f0f2b35fd0f49d04211001b5afd5e363156e02504e120475ce` |
| GFXMAX_ROOT_C | 1,475,346 | 758 | `bd503a9221a0132f5aa477a9a2424ce75eea9dc7f9d2d5f8e996033e38a7c4ec` |

The AUTO60→AUTO61 delta is **47,727 bytes** in **8** new
disjoint intervals; there are 0 AUTO60-only bytes and the net maximal-interval count delta is 0.
The manifests share 1,427,873 owned bytes with identical source kind, classification, and confidence; both endpoint manifests report zero conflict bytes.

| Transition | Delta | New intervals |
| --- | ---: | --- |
| AUTO60 → GFX2 | +47,389 | `[0x18f252,0x190ee7)` (7,317), `[0x191f8e,0x194bda)` (11,340), `[0x194c52,0x196289)` (5,687), `[0x196b87,0x19908f)` (9,480), `[0x199d35,0x19d232)` (13,565) |
| GFX2 → GFXMAX | +106 | `[0x00c92c,0x00c980)` (84), `[0x02e1d8,0x02e1ee)` (22) |
| GFXMAX → AUTO61 | +232 | `[0x03b95e,0x03ba46)` (232) |
| GFXMAX_ROOT_C → GFXMAX | +22 | `[0x02e1d8,0x02e1ee)` (22) |

The older GFX-MAX screen-root-c manifest is rejected as current input: it is missing
`22` bytes from the descriptor-candidate checkpoint, at
`[0x02e1d8,0x02e1ee)`.

## Canonical map

ROM identity is `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` (3,145,728 bytes). The emission partition has
2,461 contiguous intervals with exact coverage, no gaps or overlaps, and no out-of-bounds rows.
It contains 3,794 canonical objects, 1,670,128 UNKNOWN bytes, and
1,475,600 SOURCE_OWNED bytes (46.9080607096%). Runtime observations do not change that ownership.

| Measure | Value |
| --- | ---: |
| Executed M68K instruction objects | 330 |
| Unique executed ROM bytes | 1,244 |
| Runtime occurrences referenced | 199,630 |
| Executed instructions not fully SOURCE_OWNED | 225 |
| SOURCE_OWNED code ranges never observed / bytes | 508 / 53,420 |
| Typed data / graphics / audio / Z80 objects | 772 / 138 / 1 / 1 |
| EXECUTED_NEXT unique relations / occurrence refs | 338 / 195,794 |
| OBSERVED_NEXT_PC unique relations / terminal facts | 26 / 1,600 |
| Exception-event next edges excluded from EXECUTED_NEXT | 47 / 1,726 |
| Carver-5 hypotheses | 1,007 |
| Conflicts imported | 0 |
| Evidence references | 7,968 |
| Emission ASM / DATA / ASSET / INCBIN bytes | 56,134 / 233,676 / 1,085,110 / 1,770,808 |

Objects by canonical type: `AUDIO_DATA` 1, `GRAPHICS_STREAM` 138, `M68K_INSTRUCTION` 330, `POINTER_TABLE` 35, `ROM_DATA` 737, `ROM_RANGE` 1,794, `UNKNOWN` 758, `Z80_PROGRAM` 1.

Claims by status: `DERIVED_EXACT` 1,516, `HYPOTHESIS` 1,007, `OBSERVED_RUNTIME` 330, `STATIC_VERIFIED` 3,406.

Byte coverage by claim status is a union per status; semantic and hypothesis intervals can
overlap, so those status totals are not additive. The receipt also contains exact byte counts
for every classification, source kind, status, and emission type.

The Carver-5 report’s 1,015 bounded format candidates remain HYPOTHESIS evidence; duplicate
ranges share canonical claims (stored unique hypothesis claims: 1,007). Its Stage-4
`168` blocker count is not imported as claim conflict: the exact Stage-5 IntervalDB conflict
collection is empty, and the blockers describe unresolved candidate overlaps rather than
contradictory accepted classifications. No `CALLS`, `READS`, `WRITES`, or `POINTS_TO` relation
is inferred. Of 385 2B `EXECUTED_NEXT` graph edges, only 338 have M68K instruction records
at both endpoints and are imported (195,794 occurrence refs); the remaining 47 connect
exception-event records to instructions and are excluded (1,726 occurrence refs). Terminal
facts remain address-only `OBSERVED_NEXT_PC` relations.

## Determinism and audit

Structure hash: `7f9c95f8f05e29db54991e3f6abee94e4fd80d5013fbd0675f5abe9defdc18fe`.
Evidence-index hash: `cb43a5fdfa5a1f592a8f1fac30cdab6a88052ac1719ae872ba25058eb4a69364`.
Emission hash: `d76325cfa6d6312f28333f0633f74260214e5ae8a92fc2984fcc565872cbda8f`.
Combined map hash: `589e97fe2e070ab7f9a71a3439e19f3b965a01d24ce82b2bd42cffc9f129232a`.
The independent auditor returned `PASS_INDEPENDENT_CANONICAL_ROM_KNOWLEDGE_AUDIT`. Reimport counts and all three component hashes were unchanged.

The compact JSON receipt contains the exact interval/object/claim/relation/evidence-reference
index and source SHA-256 values without ROM bytes or raw runtime lineage. The 979 MB 2B
session database, FLOW stream, segment index, and materialized ROM remain local.

Validation results are recorded in `docs/WORKLOG.md`; this checkpoint does not modify the
production AUTO67 runner, predecessor logic, Worker/FLOW runtime, or the ownership manifest.
