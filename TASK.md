# Current Task

TASK: M4 — Local asset inspection tool
WHY: The graphics decompressor is now verified against original 68000 behavior, so the next safe step is to inspect decoded graphics from a user's local ROM without committing commercial assets.
CURRENT MILESTONE: M4
UNDERSTANDING CONFIDENCE: 96%
STATUS: ACTIVE

## Preconditions completed
- M2 canonical USA ROM identification is complete.
- M3 routine `0x00003820` is statically documented and dynamically verified.
- Native `decompress_graphics` matches original 68000 traces for both observed formats.
- Synthetic decompressor tests pass.
- General CI and ROM-backed reference verification pass.

## M3 completion evidence
- Routine bytes: `[0x3820, 0x3B3E)`.
- 52 direct absolute JSR callers identified.
- `A0` source and `A1` destination/end-pointer contract established.
- Format A oracle: ROM `0x16943C`, consumed `1217`, output `3072`, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`.
- Format B oracle: ROM `0x1894EA`, consumed `112`, output `128`, SHA-256 `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9`.

## M4 Definition of Done
- [ ] add a CLI/local tool path that accepts the user's ROM;
- [ ] refuse unsupported/unknown ROM revisions for address-based inspection;
- [ ] invoke the verified graphics decompressor at an explicit ROM offset;
- [ ] decode Genesis 4bpp tile bytes into pixel indices;
- [ ] support a local palette/input path needed for useful visual inspection;
- [ ] export only user-generated/local debug output (PNG or simple intermediate format);
- [ ] ensure generated output is ignored by Git;
- [ ] add synthetic unit tests for tile decoding/palette conversion;
- [ ] document command usage and output semantics;
- [ ] CI green.

## Constraints
- Never commit ROM bytes or extracted commercial assets.
- Generated inspection files are local-only.
- Keep reverse-engineering offsets out of gameplay logic.
- Keep files <= 500 lines.
- Do not begin M5 until M4 acceptance criteria pass.

## Exact next action
Implement a small reusable Genesis 4bpp tile decoder and tests, then expose it through a local ROM inspection command that feeds it data from `decompress_graphics`.
