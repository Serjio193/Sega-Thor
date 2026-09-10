# Bounded G0 reverse-engineering ledger

## M12.1 ASM promotion boundary

M12.1 transactionally promotes only six exact source-owned intervals inside
`0x06042A..0x0611F4`: `0x06042A..0x060484`, `0x060490..0x0604B0`,
`0x060B50..0x060CDA`, `0x0611D6..0x0611E0`, `0x0611E0..0x0611EA`, and
`0x0611EA..0x0611F4`. They total 546 bytes and are backed by exact local
assembler output plus full-ROM exactness. The remaining 2,984 bytes are kept
as UNKNOWN blobs because dispatch/case and continuation boundaries are not
closed. The gap census is `POSSIBLE_CODE_BYTES=0`, `UNKNOWN_DATA_BYTES=0`,
`UNRESOLVED_BOUNDARY_BYTES=2,984`; these categories are not additional
ownership claims. No range below is a typed data structure or portable routine.

The legacy global census marked `0x06042A`, `0x0611DC`, and `0x0611E6` as
unsupported status-register forms. The current exact decoder normalizes these
`MOVE SR` encodings, and the affected slices reassemble exactly. This resolves
the decoder limitation only; it does not close the remaining target CFG.

One M12.2 proposal is recorded for `0x00DE00..0x00E338` (1,336 bytes, 31
observed PCs, 87 static xrefs). It was not started.

## M12.0 ASM completion boundary

M12.0 changes project sequencing, not the M11 G0 evidence. The authoritative
full-ROM materialization is the local M11.15 manifest: 203 exact 68000 ASM
ranges totaling 13,550 bytes, 136 canonical-local-ROM blob ranges totaling
3,132,178 bytes, 0 gaps and 0 overlaps. M11.17 independently classifies 10
bounded data ranges totaling 1,164 bytes, but they remain blob-backed and are
not source-owned in the exact rebuild.

The 8 blob ranges intersecting bounded observed execution evidence are P0
until split and promoted transactionally. The highest concentration is
0x06042A..0x0611F4 (3,530 coarse bytes, 76 unique observed PCs and 77 static
xrefs); this is the single M12.1 target. All other unclassified ranges remain
P3 code/data/asset ambiguity. No range is promoted by this ledger.

## M11.64 closure

Status: `G0_PORTABILITY_BOUNDARY_PROVEN_M11_LINE_CLOSED`.

G0 is the parent-owned region addressed by `A5 = 0xFF001A`. M11.60 proves its observed lifetime from `0x060182` materialization through parent restore at `0x06027E` and return at `0x060284`. M11.61–M11.63 close the natural evidence cluster while retaining static, hardware, ownership, and continuation blockers. This ledger is deliberately conservative: a natural trace does not become a whole-static contract, and raw storage does not become a typed object.

| Item | Classification | Current evidence |
|---|---|---|
| `0x060182` G0 materialization | `PROVEN_NATURAL_CONTRACT` | A5 equals `0xFF001A` on the bounded natural path. |
| `0x062AE0` consumer | `PRESERVATION_PROVEN_EFFECTS_BLOCKED` | Natural preservation observed; latent `0x062CEC` indirect CFG blocks whole-static closure. |
| `0x061934` consumer | `PROVEN_NATURAL_CONTRACT` | Natural entries/effects closed; latent `0x061F60` remains static-only debt. |
| `0x0623AC` consumer | `HARDWARE_BOUNDARY` | Six byte-write PCs to VDP `0x00C00011`, 407 events. |
| `0x060286` continuation | `NOT_YET_CLOSED` | Deferred; not executed in M11.64. |
| Parent restore/return | `PROVEN_NATURAL_CONTRACT` | `0x06027E` restore followed by `0x060284 RTS`. |
| External writers/aliasing | `UNRESOLVED` | Typed ownership is blocked. |
| Interrupt and unresolved runtime effects | `NATURAL_ZERO` | Zero events in the bounded natural evidence. |
| Broader `0x061258` lifetime | `NOT_YET_CLOSED` | Outside bounded G0 interval. |

## Required gates

A register/lifetime: parent-owned and exact on observed natural entries/returns. B call/effect: natural cluster substantially closed but whole-static debt remains. C hardware: VDP dependency proven at the source PCs, with no abstraction layer. D typed-data: overlapping/shared raw access and external-writer closure are absent, so typed promotion is blocked.

`0x060286` would not change the architecture: `NO_ARCHITECTURAL_DECISION_CHANGE`. Keep all deferred edges and ownership questions explicit until new evidence is independently collected.
