# M12-GFX-1 — Whole-ROM Graphics Decompiler Sweep

Status: complete static campaign; runtime hardware layers remain explicit blockers.
No ROM, extracted asset, PNG, M13 work, or ASM-to-C++ migration was added.

## Baseline and identity

- Baseline commit: `afa3d1fc48386cfc710b32e22911d04b2c7eb6d0`; canonical ROM size: `3145728`.
- Canonical CRC32/SHA-1/SHA-256: `C4728225` / `2944910c07c02eace98c17d78d07bef7859d386a` / `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`; baseline SOURCE_OWNED: `1427873` (45.39086023966471%).
- After campaign SOURCE_OWNED: `1427873` (45.39086023966471%); gain: `0` bytes.
- Canonical ROM was read-only; all outputs are local metadata under the ignored build directory.

## Ancient format proof

The independent parser implements command blocks, RAW, extended RAW, RLE, extended RLE, LZ backreferences, LZ length extension, bit-stream blocks, and explicit zero termination. It bounds source reads, block declarations, history, and output. Decoder validity is recorded as candidate evidence only.

Known vector `0x16943C`: consumed `1217` bytes, output `3072` bytes, SHA-256 `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a`.
The existing native 0x3820 vector check reports the same values; the independent parser repeats every accepted decode before recording it.

## Whole-ROM strict census

- Offsets tested: `3145725`; viable header offsets: `3020239`.
- Strict-valid streams: `7577`; unique spans: `7577`; overlapping streams: `4917`.
- Compressed bytes across unique strict records: `1409840`; decompressed bytes: `4411088`.
- Global candidates have no independently closed container size, so their boundary status is `SELF_DELIMITING_ONLY`, not ownership.

## Graphics classification

Classification counts: `{"COMPRESSED_GENERIC": 4037, "GFX_TILE_CANDIDATE": 417, "PALETTE_CANDIDATE": 1192, "TILEMAP_CANDIDATE": 1931}`.
Tile, palette, and tilemap scans are heuristic secondary evidence. No visual similarity or decoder validity promotes a ROM span.

## 0x3820 and provenance coverage

- Static absolute JSR callers found: `52`.
- Caller PCs: `0x00C394, 0x00D3C8, 0x00D4EE, 0x00D54A, 0x00D650, 0x014558, 0x01458E, 0x0145C4, 0x0145FC, 0x024A20, 0x02A67E, 0x02B1D0, 0x02DB52, 0x02F6A0, 0x03A7FE, 0x03ACB4, 0x03ADC0, 0x03B236, 0x03B28A, 0x03B2FE, 0x03C07C, 0x03C276, 0x03C27E, 0x03C286, 0x03C5CA, 0x03C5D2, 0x03C5DA, 0x03C5E6, 0x03C9DA, 0x03C9E2, 0x03C9EA, 0x03CBE0, 0x03CBE8, 0x03CBF0, 0x03CC5A, 0x03CC62, 0x03CCD8, 0x03CCE0, 0x03CCE8, 0x03CEA6, 0x03CEAE, 0x03CEBA, 0x03CEC2, 0x03CECA, 0x03D048, 0x03D38E, 0x03D3A0, 0x03D5AE, 0x03E61A, 0x03E662, 0x03E704, 0x03E820`.
- Existing exact chains and known streams remain governed by prior M12 transactions; this sweep does not re-promote them.
- Existing runtime evidence observes decoder PCs `0x003820` and `0x003830`; reader correlation covers `0x152340..0x211F78` (75,969 unique ROM bytes), but it has no per-invocation registers, RAM destination, or DMA record.
- Existing static ID3 control (not a new sweep promotion): ROM `0x1AE1A8..0x1AE8AA`, output RAM `0xFF2FA8..0xFF3FA8`, DMA/VRAM `0x4000..0x4FFF`, 0x800 words, output SHA-256 `36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277`.
- CRAM source/line mappings and SAT captures are unavailable in retained artifacts; no fabricated mappings or previews are claimed.

## Promotion decisions

- Strict streams wholly in UNKNOWN: `6836`; rejected for missing consumer/container proof: `6836`.
- Promoted spans: `0`; SOURCE_OWNED remains unchanged by design.
- Largest remaining UNKNOWN is governed by the existing baseline manifest; no safe >=16 KiB graphics promotion closed in this pass.
- Existing blocker accounting is unchanged: B `623036` bytes / 113 ranges, F `58789` / 56, G `1036030` / 589 before and after.

## Deterministic local artifacts and gates

- JSON report: `build\m12-gfx1-whole-rom\whole_rom_report.json`; SHA-256: `fdd0f9e2ded5a8fd000193b7ddfda1bf9873d3d4c086fe1b203da91213ba6fc1`.
- Raw tile candidates: `47365` tiles in `200` retained spans.
- Palette candidates retained: `100`; tilemap candidates retained: `100`.
- Parser unit tests, Python compilation, `git diff --check`, source-size check, and relevant project tests are recorded by the final worklog entry.
- Implementation SHA: `06db09a741b9aae465cb9a6a8ea09604615c1fb3`; exact final-SHA CI: GitHub Actions `34643596746` — success (Build, Test, Complete).
