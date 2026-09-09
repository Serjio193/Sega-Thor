# M11.42 — Restart: Remaining Interpreter Attribution and 95% Coverage Gate

## Result

`M11.42_BASELINE_BLOCKED_CHECKPOINT_CANONICALIZATION_INCOMPLETE`

M11.42 stopped at PHASE 1. No interpreter remainder ledger, candidate
semantic expansion, mechanical generation, shadow certification, hardware
promotion or 95% coverage claim was made.

## Restart gate

The run used baseline commit `5c19e22a93150ab17f51c69ed5aadaa59651a70a`, the
canonical USA ROM, the external GPGX DLL, the unchanged cold-reset neutral
600-frame scenario, `BASIC_BLOCK_NATIVE` and the existing 28-range registry.

| identity / metric | required | fresh M11.42 run |
| --- | --- | --- |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | same |
| GPGX DLL SHA-256 | `140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd` | same |
| frames | 600 | 600 |
| registry ranges | 28 | 28 |
| total guest instructions | 6,488,773 | 6,488,773 |
| translated instructions | 5,826,857 | 5,826,857 |
| interpreter instructions | 661,916 | 661,916 |
| translated share | 89.7991% | 89.7991% |
| video sequence SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` | same |
| boundary yields | 140,065 | 140,065 |
| interrupted resumptions | 274 | 274 |
| starts inside translated ranges | 0 | 0 |
| authoritative checkpoint aggregate | `c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1` | `d5de401ceb64da875219d3ca2564160b954d55a217f4bb47ff0dc204c547ae36` |

Two fresh current runs, one ordinary and one with opt-in raw evidence,
produced the same `d5de...` aggregate. The first differing checkpoint was
ordinal 0, frame 60: current canonical state hash
`3e4e14f9cab0a64d51008e016fc64b93a17136d036db3ceb71b62fbf547b696a` versus
the preserved M11.41 repaired run's
`ea1a8919a052cfe5996067d02f70327056635116a63dcf2cbc542808bcf19a09`.

## Exact remaining identity defect

The fresh raw evidence buffers are both `10,362,880` bytes. Their first raw
difference against the preserved M11.41 repaired evidence is frame 60,
serialized-state offset `140654`, with current bytes `68 66 FE 7F` and prior
bytes `C9 6C FE 7F`. This is the already-known host address representation in
the first `FM_SLOT.DT` field.

Applying the committed M11.41 canonicalization spans leaves the first
remaining difference at offset `140734`, byte `0x68` versus `0xC9`. The
external GPGX `FM_SLOT` layout and the serialized pattern identify this as the
first byte of the next `FM_SLOT.DT` host pointer. The committed adapter's
assumed `ym_offset=140651` and `slot_size=84` therefore do not cover the
actual serialized pointer layout completely. This proves the M11.41 identity
repair is incomplete; it does not prove any guest semantic or execution-state
difference.

## Stop decision and preserved history

The required authoritative aggregate was not reproduced, so PHASE 2 and every
later phase are invalid for this run. M11.39's
`HOT_PATH_DYNAMIC_COVERAGE_80_PROVEN`, M11.40's negative baseline result and
M11.41's checkpoint-provenance history remain preserved. No interpreter PC was
classified, no candidate was promoted, and no 95% result is claimed.

All raw evidence and generated run directories remain local ignored artifacts.
No ROM, extracted asset, emulator binary, generated run evidence or
`game.srm` is tracked.

## Validation

The developer-only hybrid target built successfully. Final Debug, Release and
GNU-equivalent full CTest each passed `55/55`, including semantic,
generator/provenance, checkpoint identity and boundary tests. The source-limit
check, `git diff --check` and tracked-artifact hygiene check passed. Shadow
certification and candidate native proof were not run because PHASE 1 failed.
