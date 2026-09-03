# Current Task

TASK: M5 — VDP data model and rendering primitives
WHY: M4 is verified; the next dependency for faithful scene reconstruction is a narrow, testable representation of Mega Drive video state used by Beyond Oasis.
CURRENT MILESTONE: M5
UNDERSTANDING CONFIDENCE: 96%
STATUS: ACTIVE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 native graphics decompressor matches original 68000 behavior for both observed formats.
- M4 local asset inspector decodes verified ROM graphics into Genesis 4bpp tile sheets.
- `oasis_inspect` was verified on ROM offset `0x16943C`: 1217 compressed bytes -> 3072 bytes -> 96 tiles -> 128x48 PGM.
- General CI and ROM-backed reference workflow pass.

## M5 Definition of Done
- [ ] extend VDP state with 64 KiB VRAM, 128-byte CRAM and 80-byte VSRAM storage;
- [ ] provide bounded byte/word read/write operations required by translated code;
- [ ] define and test Genesis tile attribute decoding: tile index, palette, priority, horizontal flip, vertical flip;
- [ ] define minimal plane-cell representation without implementing a full emulator;
- [ ] define minimal sprite attribute representation only after the raw format is documented;
- [ ] keep hardware representation independent from SDL/GPU APIs;
- [ ] add synthetic tests for memory bounds and tile attributes;
- [ ] document implemented VDP semantics and explicitly list unsupported semantics;
- [ ] keep every file <= 500 lines;
- [ ] CI green.

## Constraints
- Do not implement a general-purpose Mega Drive emulator.
- Do not add rendering libraries to `oasis_core` yet.
- Do not invent Beyond Oasis-specific VDP behavior without evidence.
- Prefer raw hardware-level structures first; semantic scene abstractions come later.
- Do not start M6 until M5 acceptance criteria pass.

## Exact next action
Replace the placeholder VDP scaffold with bounded VRAM/CRAM/VSRAM storage and add a tested decoder for the standard 16-bit Genesis tile attribute word.
