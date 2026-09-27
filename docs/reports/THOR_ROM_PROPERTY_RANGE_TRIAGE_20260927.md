# Runtime ROM Property Range Triage — 2026-09-27

## Iteration 1: `0x158296–0x261124`

**Result:** no property added. Classified coverage remains 311,338 bytes
(9.897168%) before and after this iteration. This is an intentional fail-closed
result: the available evidence does not bind these observed reads to a complete
consumer operation.

### Evidence checked

- The accepted corrected-runtime checkpoint is
  `scenario-menu-start-directional-20260927-10000-01`, with ROM SHA-256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`, build
  ID `e202add134725f7460c2a67d866d0b2e1c97ba5f9175ca5b4f76a4e961640889`, and
  proof-contract SHA-256
  `10da481265de188d7a11049f34d24dd33f3b0e34b5ff797d12541ee241356c78`.
- In that checkpoint, the target interval is one contiguous `M68K_DATA_READ`
  only span of 1,085,070 bytes. No accepted executed-encoding, VDP-source, or
  audio-payload bit is present there.
- The saved W6 discovery summary lists eight M68K ROM-read intervals that
  intersect the target: `0x165080–0x165090`, `0x1650B4–0x1650D6`,
  `0x165836–0x165846`, `0x165854–0x165878`, `0x16610C–0x166126`,
  `0x166320–0x16633E`, `0x260002–0x260012`, and `0x260122–0x260148`.
  These sparse intervals are from a separate discovery artifact and do not
  provide a byte-complete read-to-consumer chain for the accepted checkpoint.
  Its VDP/audio summaries are aggregate counters without ROM-origin links.
- Static evidence elsewhere in the repository establishes individual graphics
  streams and consumers of decoder `0x3820`, but it does not establish that
  every observed byte in this target was consumed by those calls in the
  accepted live run. Compressed bytes cannot inherit a VRAM-source property.

### Why current runtime hooks cannot prove the chain

The M68K data-read callback receives logical address, value, and width and
resolves exact backing ROM bytes. It does not retain the instruction/consumer
context or follow the value into a RAM write. The direct VDP source callback
proves consumed ROM-backed DMA bytes only; it cannot recover ROM origins after
a RAM copy or decoder transformation. The host-side exact-copy provenance
primitive is not wired into live RAM writes. Therefore a general promotion of
the target, or a compressed-graphics promotion based only on nearby static
stream evidence, would exceed the current proof contract.

### Coverage accounting

- `CLASSIFIED_COVERAGE before`: 311,338 / 3,145,728 (9.897168%)
- `CLASSIFIED_COVERAGE after`: 311,338 / 3,145,728 (9.897168%)
- `NEW_CLASSIFIED_BYTES`: 0
- `property responsible`: none
- Bitmap coverage is unchanged at 3,145,226 / 3,145,728 (99.984042%); the
  502 zero-mask bytes remain unknown.

Largest observed-only ranges remain:

| ROM range | Bytes |
|---|---:|
| `0x158296–0x261123` | 1,085,070 |
| `0x28CF3A–0x2FFFFF` | 471,238 |
| `0x03C9CC–0x05FFFF` | 144,948 |
| `0x062B1C–0x080009` | 120,046 |
| `0x11D9E0–0x139C9B` | 115,388 |

### Mechanism needed for a future proof

For ROM→RAM→VDP, a runtime adapter must preserve per-byte physical ROM origin
through an explicitly supported exact copy, invalidate it on every unsupported
RAM write/transform, and mark a ROM property only when a completed VDP write
consumes those exact origins. For decoder input, a separate compressed-source
property needs a closed decoder entry/exit contract, exact consumed source
extent, physical origin checks, and live evidence that the decoder completed.
Until such evidence exists, the 1,085,070 bytes remain `OBSERVED_UNCLASSIFIED`.

No capture was extended. No property schema, proof contract, canonical map,
ownership, or GUI code was changed.
