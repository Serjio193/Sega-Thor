# M14.7B — real capture canonical closure

Status: `PASS_REAL_CAPTURE_CANONICAL_CLOSURE_V1` for one bounded capture. The source campaign outcome remains `STOPPED_FRAME_LIMIT`; closure proves the captured evidence bundle was fully accounted and consumed, not that the campaign completed normally.

Selected capture: `FLOW_V1`, 1,196,400 bytes, SHA-256 `88270c71eadadd291060663d8ea082f9600e3d3a4a2b9fb42ef405bd306b7e01`. It was selected by deterministic minimum-size among valid, supported, parseable candidates with indexed windows, real occurrences, and compatible canonical ingestion. Its byte-identical historical pass1/pass2 files remain; only the selected continuous capture was deleted.

## Closure evidence

- Accounting: 24,925 physical records = 1,979 merged + 22,946 overlap duplicates already known + 0 unresolved + 0 rejected; unaccounted 0.
- All 128 indexed segments passed identity, byte-span, stream-boundary and CPU sequence checks. The envelope retained all 12 FLOW fields and exact 48-byte record hex.
- Independent ROM linker audit passed: 12,848 M68K instructions linked across 218 exact ranges; 128 segments; zero unresolved, unsupported, or identity mismatches. Z80 occurrences remain runtime evidence and are not projected onto the M68K ROM.
- Canonical map: `gen-m14-7-29314da7a34a38e6` → `gen-e10dd56fe8b68d0b-5c2d1b47`; map hash `515f551f…58cac` → `12a45d63…5a45a5`; `SOURCE_OWNED=1,487,672` before and after.
- Pre-cleanup self-check passed: 2,490 emission ranges, zero gaps/overlaps, 2,921 occurrences (1,979 direct, 942 derived), 256 ordered CPU-specific paths, 122 paths with repeated PCs, 124 canonical branch alternatives, zero unresolved/conflicts, 37,719 raw byte offsets reverified.
- The closure receipt lists the exact raw and 11 generated staging files by path, byte size, and SHA. `cleanup --closed-only` deleted exactly 12 files / 104,552,852 bytes. The raw and disposable staging artifacts are absent. Segment index remains.
- Post-delete map queries pass: 1,979 direct occurrences + 942 derived occurrences, all 256 paths, source hash/index provenance, zero unresolved, unchanged map hash and path digest. Both same-byte historical copies remain at SHA-256 `88270c71…6b7e01`.

## Corpus dry-run

Read-only scan of `C:\Github\Sega-Thor\build` after deleting the selected raw and task-generated staging attempts:

- Raw candidates: 1264 files / 31,714,495,200 bytes.
- Supported format readers: 1264 files / 31,714,495,200 bytes.
- Duplicate redundant paths: 246 files / 17,199,644,640 bytes across 139 SHA groups.
- Missing format readers: 0 files / 0 bytes.
- Unique content blobs queued for ingestion: 1018.

The corpus classification is a read-only format/hash pass. It does not claim every remaining capture has been parsed or that any is already `NO_NEW_KNOWLEDGE`; those decisions require per-experiment ingestion and comparison. Full file-level classification and queue are in `build/thor-evidence/m14-7b-real-capture/corpus-dry-run.json`.
