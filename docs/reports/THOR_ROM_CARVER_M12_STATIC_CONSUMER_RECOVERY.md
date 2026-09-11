# Thor ROM Carver M12-CARVER-4 — Global Static Consumer Recovery

Status: complete static pass, fixed point reached, no new ownership transaction.
The pass starts from `origin/main` `5cfd51fa6bdf24edb791520cc06fe494e61deace`
and does not repeat the M12-CARVER-3 runtime sweep.

## Result

| measure | baseline | final | delta |
| --- | ---: | ---: | ---: |
| `SOURCE_OWNED` bytes | 1,427,873 | 1,427,873 | 0 |
| `SOURCE_OWNED` percent | 45.3908602397% | 45.3908602397% | 0 |
| ROM bytes | 3,145,728 | 3,145,728 | 0 |
| remaining UNKNOWN ranges | 758 | 758 | 0 |

No candidate, entropy match, decoder-only result, or static reference was
promoted. Existing exact promoter contracts remain authoritative. No ROM,
asset, unknown `dc.b`, M13 work, or ASM-to-C++ migration was added.

## Static census and consumer graph

The analyzer consumed 559 canonical-compatible non-runtime static artifacts
from the existing build evidence root. It emitted 24,172 deduplicated typed
references:

| edge type | count |
| --- | ---: |
| `CODE_TO_ROM_RANGE` | 21,618 |
| `CODE_TO_POINTER_TABLE` | 259 |
| `TABLE_TO_ROM_RANGE` | 405 |
| `CODE_XREF` | 1,890 |

All 758 UNKNOWN ranges are represented in `remaining_unknown.ranges` in the
machine-readable report, with range id, half-open bounds, size, family id,
static reference count, and blocker. `gap_details` additionally retains the
existing Carver left/right neighbors, pointer/xref evidence, consumers,
conflicts, and detector evidence.

The global clustering produced 136 consumer families. Families are formed by
shared parser/consumer ancestry or static PC ancestry, with the unresolved
blocker family used only when no stronger shared ancestry exists. The largest
families and every member range are in `consumer_families` and
`remaining_unknown.largest_families`; no one-gap manual campaign was used.

## Parser contracts and containers

The pass recorded 2,116 reusable static contract observations:

| observed shape | count |
| --- | ---: |
| count × stride | 5 |
| fixed size | 154 |
| sentinel termination | 1 |
| unresolved contract shape | 1,956 |

Zero exact boundary bytes intersecting an UNKNOWN range were recovered. Static
container/bank and code/data candidates remain evidence-only. The existing
candidate reports yielded 2,725 candidates; 0 were promoted and 2,725 were
rejected because no new exact ownership transaction was available. Exact
reassemblable UNKNOWN code bytes recovered: 0.

## Blockers and conflicts

The complete post-pass UNKNOWN census is unchanged:

| blocker | ranges | bytes |
| --- | ---: | ---: |
| B — known/observed family without a closed consumer contract | 113 | 623,036 |
| F — candidate/conflict overlap | 56 | 58,789 |
| G — no available scenario/static contract closure | 589 | 1,036,030 |
| **total** | **758** | **1,717,855** |

No F conflict was silently removed or reclassified. The stored global conflict
census remains 168 blocking conflicts, with 0 resolved. The static pass itself
reached the Carver fixed point with an empty expansion queue, no range split,
no reclassification, and no new provenance edge in the final reconciliation.

The negative result is global: the remaining large families require either a
closed exact container/parser contract, a conflict-resolution evidence class,
or new coverage/evidence unavailable in this stage. This is the stop condition;
the pass does not begin detector expansion or another runtime sweep.

## Deterministic identity

Canonical ROM:

- size: `3,145,728`
- CRC32: `C4728225`
- SHA-1: `2944910c07c02eace98c17d78d07bef7859d386a`
- SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

Static run output is ignored build data at
`build/m12-carver-m12c4-static-recovery-e/`. Its deterministic hashes are:

- IntervalDB SHA-256: `4968fb1b2a4d35ed8df69b21e0de0e1606e93827effa8d2c30be9dadd9d183c3`
- evidence-state SHA-256: `386ac99f5d6d8d4a814c3c06c1dc1d5ed25a6130d46fd51c635039cb7d67f306`

## Validation and publication

The targeted static-recovery test and Python compilation pass. Windows Release
build passed; Windows Release CTest passed 138/138 with the source-limit test
also passing separately. WSL GNU build passed and WSL CTest passed 138/138 when
the source-limit test was excluded; its `/mnt/c` source-limit invocation was
stopped after more than eight minutes because of the large pre-existing root
build-artifact set. The equivalent Windows source-limit command passed. The
known unrelated MSVC Debug blocker remains the pre-existing
`src/core/ram_flag_routine.cpp:180-181` `std::to_string` failure (Debug build
exit 1); no M12-CARVER-4 file is involved.

Implementation SHA: `TO_BE_FILLED_AFTER_IMPLEMENTATION_COMMIT`.
Exact implementation-SHA CI: `TO_BE_FILLED_AFTER_IMPLEMENTATION_CI`.
Final publication SHA and exact final-SHA CI are reported with the completed
publication and are not used to alter the canonical ROM or this result.

Machine-readable source:
`build/m12-carver-m12c4-static-recovery-e/static_recovery_report.json`.
