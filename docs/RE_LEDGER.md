# Bounded G0 reverse-engineering ledger

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
