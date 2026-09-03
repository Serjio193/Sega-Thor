# Current Task

TASK: M6 — Deterministic runtime frame/input skeleton
WHY: M5 now provides a tested portable representation of the Mega Drive video state needed by translated code. The next dependency is a platform-independent frame step and input model before translating world/player behavior.
CURRENT MILESTONE: M6
UNDERSTANDING CONFIDENCE: 95%
STATUS: ACTIVE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 graphics decompression is native and differentially verified.
- M4 local graphics inspection is verified against the reference ROM.
- M5 raw VDP state model is implemented and tested without renderer dependencies.

## M5 completion evidence
- VRAM: 64 KiB; CRAM: 128 bytes; VSRAM: 80 bytes.
- Bounded byte-span and big-endian word access implemented.
- Standard tile attributes decode tile index, palette, priority, H-flip and V-flip.
- Minimal plane-cell and four-word sprite-attribute views implemented.
- Sprite raw layout documented before structured use.
- `docs/VDP_MODEL.md` explicitly lists unsupported/full-emulator semantics.
- `oasis_vdp` synthetic test covers attribute decoding, storage and bounds.
- CI build/test completed successfully on the M5 head.
- No SDL/GPU/platform dependency was added to `oasis_core`.

## M6 Definition of Done
- [ ] define a platform-independent controller input snapshot;
- [ ] define one deterministic game-frame step API;
- [ ] keep time/frame progression explicit rather than reading wall-clock time in game logic;
- [ ] add deterministic tests proving identical state for identical initial state + input sequence;
- [ ] keep platform-specific polling outside `game` and `genesis` layers;
- [ ] document frame/input ownership and unsupported timing behavior;
- [ ] keep every file <= 500 lines;
- [ ] CI green.

## Constraints
- Do not implement SDL/windowing yet.
- Do not translate player/world logic as part of the frame-loop scaffold.
- Do not add variable-delta gameplay semantics without original-game evidence.
- Do not begin M7 until M6 acceptance criteria pass.

## Exact next action
Create a small `game/runtime` frame-state/input API with an explicit frame counter and deterministic `step()` behavior, then prove repeatability with synthetic input-sequence tests.
