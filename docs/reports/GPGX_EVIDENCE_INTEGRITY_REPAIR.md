# M11.23 — GPGX Evidence Integrity Repair

## Scope

This milestone repaired only confirmed evidence-pipeline defects. No new
runtime capture, hook, UI, replay, resource investigation, semantic naming or
trust promotion was performed.

## Findings and regressions

| Finding | Result | Regression |
|---|---|---|
| Adjacent ROM-read bytes could hide a first-reader or width change | **CONFIRMED / FIXED** | GPGX native merge test now requires reader changes and width changes to emit separate ranges |
| Range validation used only summed lengths | **CONFIRMED / FIXED** | Python correlation rejects duplicate/overlap ranges, including overlap with compensating coverage, and checks exact bitmap union |
| Importer parsed JSON with line-oriented regex | **CONFIRMED / FIXED** | `gpgx_json` structural parser accepts array/object range schemas independent of formatting and field order; malformed schema fails explicitly |
| Bounded classification trusted a supplied complete list | **CONFIRMED / FIXED** | Independent bounded decoder sequence must reach `0x060BC4` with exact ROM bytes; deleting 7/8 supplied instructions prevents upgrade |
| PC even-slot bitmap mixed with ROM-read byte bitmap | **FALSE for current importer** | Bitmap sizes/types are separate: PC importer requires `ROM_SIZE/16`, ROM-read correlation requires `ROM_SIZE/8` |
| Inclusive/half-open range ambiguity | **PARTIAL / DOCUMENTED** | Runtime ROM-read ranges remain inclusive byte ranges; importer static classification ranges remain explicit half-open `[start,end)` and boundary self-test covers start, end-1 and end |
| Persistent merge across incompatible GPGX revisions | **CONFIRMED / FIXED** | Known build IDs are checked; existing evidence with more than one known build or a new incompatible build is rejected |
| Runtime data conflict could suppress execution evidence | **FALSE** | `CODE_EXECUTED_AT_ADDRESS` facts remain emitted; `RUNTIME_DATA_CONFLICT` is only a classification diagnostic |

## Provenance

New GPGX captures emit schema/version, capture ID, canonical file SHA when
available, runtime buffer SHA, verified word-byte-swapped-16 relationship and
the full instrumented build ID. The known runtime buffer SHA for the canonical
ROM is
`9ab80b5bbf33d9067015ad705537997fc3198228d731f457889a1c17b39aa19d`.
The repaired external experiment build is
`d60d079934977aa6973e220d123533387159f66e`; the previous frozen build remains
an allowed compatibility identity. Captures without the new fields are
accepted only as `LEGACY_WEAK`; their history is not rewritten.

## Previous milestone validity

**STILL_VALID:** raw executed-PC addresses, raw ROM-read bitmaps, canonical ROM
identity, and unaffected execution facts from M11.19–M11.22.

**NEEDS_REGENERATION:** reader byte totals, grouped reader regions and
correlations that depend on the old producer grouping. No new runtime capture
was taken, so the retained old correlation output is not relabeled as repaired.

**INVALIDATED:** none beyond the old reader-grouping interpretation itself.

## Validation

- GPGX external rebuild: `make -f Makefile.libretro platform=win HOOK_CPU=1 -j$(nproc)`; GCC 16.2.0
- GPGX native range-merge and automatic-analysis tests: PASS
- Sega-Thor importer self-test, bounded-classification tests and reader-correlation tests: PASS
- Full Debug CMake build: PASS; CTest: 45/45 PASS; focused Release importer build and self-test: PASS
- Full Release MinGW build was attempted but is blocked by an unrelated existing
  `oasis_re_callee_effect` static-library link failure; no evidence-pipeline
  target failed. This local toolchain limitation is retained explicitly.
- No ROM, bitmap, DLL, savestate, extracted asset or runtime dump is tracked

## Decision

`GPGX_EVIDENCE_INTEGRITY_REPAIRED`

The next work is documentation-only planning of the bounded resource contract
for `0x02CFAA -> 0xD3B2` (resource 3, table `0x05CE96`, stream `0x1AE1A8`,
decompressor `0x3820`, RAM near `0xFF2FA8`). It is not implemented here.
