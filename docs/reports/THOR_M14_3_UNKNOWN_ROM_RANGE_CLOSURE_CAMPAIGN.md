# M14.3 UNKNOWN ROM range closure campaign

**Result:** `STOP_NO_EXACT_CLASSIFICATION`. The full UNKNOWN queue was ranked
and screened using the accepted M14.2C canonical SQLite generation and its
M14.2B global evidence graph. No exact map operation was justified. This is a
bounded no-classification result with explicit blockers, not a claim of ROM
closure.

The deterministic queue covers all 768 UNKNOWN ranges / 1,658,056 bytes. Its
ranking hash is `af04d666b708e4d085237a04c22a76ccff7c708818874eb10ae3dda4b449f092`.
The input generation is `gen-m14-2c-598c42312e80e1d8`; graph hash is
`3d0b19f11a1d8fa03a7c68a98756ef6c87f345a4aa74781166a9900bebe06d78`, map
hash is `d520a6da1be5d730c4352eacc75ce70fd47a23465d6899282cea7188ad416f88`,
and emission hash is
`44a2332b0b433c635e33767886ffff35985ad31131e3dc3b4dea5e6984b17d92`.

| Rank | ROM range | Bytes | Executed PCs | Incoming refs | Graph degree | Independent artifacts | Hypothesis label | Exact blocker |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 1 | `0x061588..0x061CD2` | 1,866 | 177 | 16 | 218 | 4 | decoder EOS | Closed CFG and byte roundtrip unavailable |
| 2 | `0x001F72..0x0023C6` | 1,108 | 125 | 37 | 179 | 5 | monotonic pointer family | Closed CFG and byte roundtrip unavailable |
| 3 | `0x002992..0x002AA4` | 274 | 83 | 8 | 92 | 5 | unclassified | Closed CFG and byte roundtrip unavailable |
| 4 | `0x005670..0x0062CC` | 3,164 | 62 | 4 | 69 | 2 | decoder EOS; monotonic pointer family | Closed CFG and byte roundtrip unavailable |
| 5 | `0x008F72..0x0091F2` | 640 | 47 | 14 | 65 | 2 | monotonic pointer family | Closed CFG and byte roundtrip unavailable |

Rank 1 is the strongest runtime-seeded candidate, but its 1,866-byte emission
contains only 650 bytes of exact instruction-object coverage, with uncovered
gaps. `EXECUTED_NEXT` and `OBSERVED_NEXT_PC` establish scoped observations;
they do not prove a closed routine extent. The current graph has no direct
branch/call-target, ROM-read, pointer-target, or audio-read relation for this
candidate. Its `decoder_eos` label is a `HYPOTHESIS`, not a resource class.

The queue groups 27 ranges under missing closed-control-flow / canonical-byte
roundtrip proof, 302 under hypothesis-only boundary or format claims, and 439
without exact consumer or extent proof. Across the queue, the missing
capabilities are: canonical ROM bytes 27; closed control flow 27; assembler
roundtrip 27; decoder/consumer 60; resource length 60; table count and
addressing interpretation 49 each; exact boundary 48; structural padding
proof 182; exact consumer and extent 439 each. These categories overlap where
a range has multiple unresolved obligations.

The largest generic improvement is to supply the canonical ROM byte artifact
to a generic decoder and CFG closure pass, then require closed-flow and exact
assembler roundtrip before proposing ASM extents. Analysis windows remain
non-extents. Hypotheses did not produce operations.

| Metric | Before | After | Delta |
| --- | ---: | ---: | ---: |
| Full ROM bytes | 3,145,728 | 3,145,728 | 0 |
| Canonical ranges | 2,489 | 2,489 | 0 |
| Gaps / overlaps | 0 / 0 | 0 / 0 | 0 / 0 |
| UNKNOWN ranges | 768 | 768 | 0 |
| UNKNOWN bytes | 1,658,056 | 1,658,056 | 0 |
| Newly classified ranges / bytes | — | 0 / 0 | 0 / 0 |
| `SOURCE_OWNED` bytes | 1,487,672 | 1,487,672 | 0 |

No boundary, reference, symbol, class, emission, or ownership operation was
generated. Full ranked top-30 details and all-range hash are in the adjacent
machine-readable [receipt](THOR_M14_3_UNKNOWN_ROM_RANGE_CLOSURE_CAMPAIGN.json).
