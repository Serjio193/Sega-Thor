# M12 Targeted Dynamic Sprite Capture V1

Date: 2026-09-23
Result: `PASS_TARGETED_DYNAMIC_SPRITE_CAPTURE_V1`

## Runtime identity

- ROM: canonical user-supplied Beyond Oasis (USA), SHA-256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- Runtime: installed stock BizHawk 2.11.1/GPGX at
  `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64`
- Scenario: `src/tools/re_bizhawk_m11_8_natural_scenario.txt`
- Start: hardware reset; natural Start/movement/action inputs; 1,800 frames
- Capture: `build/m12-targeted-dynamic-sat-v1-run2/capture.json`
- Capture SHA-256: `c02b9e178662710b4e30c6c52527337c918bf63cdfb41ebe7dcf420d8c444b1f`

## Evidence

The capture contains 1,801 SAT snapshots (one at every frame boundary) for
VRAM SAT base `0xD000`, plus 15,889 writes to the accepted shadow range
`0xFF13CC..0xFF1523` with runtime PCs. Entry-level analysis found 124 SAT
transitions across entries 0 through 7; 123 transitions have an exact current
SAT entry equal to the corresponding shadow entry and a bounded writer-PC
window.

The first closed witness is SAT entry 0, transition `714 -> 715`. The source
shadow state is frame 714, changed offsets are `0,1,4,6,7`, and the published
entry is:

```text
01 04 00 00 84 00 01 1D
```

The writer family observed in the source window includes `0x00B754`,
`0x00B768`, `0x00B772`, and `0x00B77E`; `0x001FD6` and `0x03B46A` are also
present as reset/clear writes. A later movement witness is entry 0,
transition `1500 -> 1501`, changing byte offset 5, with the same producer
family. The measured publication relation is bounded to the source-shadow
frame and following SAT frame, documenting a one-frame DMA/publication lag;
it is not claimed as same-instruction causality.

## Boundary

This closes targeted dynamic SAT entry, shadow source, and writer-PC evidence
only. It does not prove an object/player label, controller-to-entity causality,
or a new semantic sprite group. `PLAYER_LABEL_PROVEN = NO` remains unchanged.
`SOURCE_OWNED` remains `1,487,672` with delta `0`; no ROM/assets were added and
no commit/push was performed.
