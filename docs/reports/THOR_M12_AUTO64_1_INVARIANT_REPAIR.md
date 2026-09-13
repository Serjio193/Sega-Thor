# THOR M12 — AUTO64.1 invariant repair

## 1. Baseline SHA

`f99e7e6d9069ea9d79f5dd2f58ffc15b64f5a136` (`M12 add AUTO64 knowledge
coverage scheduler`). Scope was limited to the two validation regressions;
AUTO64 coverage/scheduler behavior, gameplay discovery, C++ and M13 were not
redesigned or started.

## 2. ASM failure reproduction

The clean generated expanded layout was based on
`build/m12-auto60-contiguous-islands-a/materialized/manifest.json` and copied
to the fresh `build/m12-auto641-full-bypass/` output. The bypass path called
`re_full_split_run.write_layout()` before this repair, then invoked exactly:

```text
build/m11-9/vasm/vasmm68k_mot.exe -m68000 -no-opt -Fbin -o build/m12-auto641-full-bypass/rebuilt.rom build/m12-auto641-full-bypass/full_layout.asm
```

The assembler exited `1`. It emitted normal vasm 1.8g startup text and then
four `error 75 ... label ... redefined` diagnostics. No stale files were
present in the fresh output; the failure was deterministic.

## 3. Exact duplicate labels

| Label | Alias source | Definition source | Alias / definition ROM address | Layout definition line |
|---|---|---|---:|---:|
| `loc_00B856` | `code/sub_00B79A.asm:3` | `code/sub_00B852.asm:8` | `$00B856` / `$00B856` | 11337 |
| `loc_00B912` | `code/sub_00B79A.asm:6` | `code/sub_00B912.asm:4` | `$00B912` / `$00B912` | 11454 |
| `loc_00E2D4` | `code/sub_00B79A.asm:5` | `code/sub_00E2D4.asm:4` | `$00E2D4` / `$00E2D4` | 15446 |
| `loc_00E7FC` | `code/sub_00B79A.asm:4` | `code/sub_00E7FC.asm:4` | `$00E7FC` / `$00E7FC` | 16171 |

The old assembler output was:

```text
error 75 in line 11337 of "full_layout.asm": label <loc_00B856> redefined
error 75 in line 11454 of "full_layout.asm": label <loc_00B912> redefined
error 75 in line 15446 of "full_layout.asm": label <loc_00E2D4> redefined
error 75 in line 16171 of "full_layout.asm": label <loc_00E7FC> redefined
```

## 4. Classification of each duplicate

All four are class **A**: same address and same semantic target. They are
redundant `loc_<address> equ $<address>` aliases in `sub_00B79A.asm` for
labels defined by the slices at the corresponding ROM addresses. No class B
conflict, class C generated-name collision, class D stale-output mixture,
class E nondeterminism, or class F cause was observed.

## 5. AUTO62 repair path

AUTO62's original repair was the `filter_redundant_layout_aliases()` function
in `src/tools/re_auto_promote.py`, called from `materialize()`. Its inputs
were generated ASM lines and the set of labels defined by all generated code
slices. It suppressed only a `loc_<address> equ $<same-address>` line when the
target label had a real definition in another slice; other aliases remained in
the output.

## 6. Regression root cause

The repair lived only in the AUTO promotion materializer. The independent
full-layout writer in `src/tools/re_full_split_run.py::write_layout()` copied
ASM lines directly and therefore bypassed the AUTO62 contract. AUTO64's
full-layout validation exercised that path. The generated sources and manifest
were otherwise stable, and the four aliases were recreated from identical
inputs in the same order.

## 7. Code fix

`re_full_split_run.py` now owns the single canonical alias filter. It collects
defined labels across the entire layout, removes only provably equivalent
same-address aliases, and raises a clear `ValueError` for a conflicting
same-name alias instead of assembling ambiguous output. `re_auto_promote.py`
now delegates its layout writing to `FULL.write_layout()`; no parallel
materializer rule remains. Fresh outputs are still required by the full split
runner, and AUTO materialization still removes an existing target before
regeneration.

## 8. Regression tests

`tests/re_full_split_test.py` now covers equivalent alias removal, conflicting
alias failure, and two identical materializations producing byte-identical
`full_layout.asm`. `tests/re_auto_promote_test.py` exercises the shared
canonical filter. Both passed on Windows and GNU/Linux-equivalent CTest.

## 9. First clean assembly proof

Fresh output: `build/m12-auto641-full-clean-run1/` from the current AUTO62
manifest. The vasm invocation was the command shown in section 2 with the
fresh run-1 output paths. Result: assembler exit `0`, full ROM match, and
`first_difference = NONE`.

## 10. Second clean assembly proof

Fresh output: `build/m12-auto641-full-clean-run2/`, regenerated from the same
manifest and ROM after the first output. Result: assembler exit `0`, full ROM
match, and `first_difference = NONE`. The generated layout SHA256 was
`b1d3724357fe4f05bc0badae166b049aac9fb56c69eb9dcce70ecc1b4e701f17` for both
runs; rebuilt ROM bytes were also identical.

## 11. Canonical size/CRC/SHA

Both clean runs produced:

```text
SIZE:   3,145,728
CRC32:  C4728225
SHA256: eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263
```

## 12. First differing byte

`NONE`. Both rebuilt ROMs are byte-for-byte identical to the canonical local
ROM.

## 13. GNU hang reproduction

The prior GNU command was:

```text
wsl.exe bash -lc "cd /mnt/c/Github/Sega-Thor && ctest --test-dir build/m12-auto2-linux-gnu --output-on-failure"
```

It stopped at test `project_file_line_limit` after approximately six minutes
in the AUTO64 run. An independent bounded invocation of the underlying test
also failed to return within 30 seconds before the repair:

```text
wsl.exe bash -lc "cd /mnt/c/Github/Sega-Thor && cmake -P tests/check_file_limits.cmake"
```

The old workspace-wide traversal scope contained 60,301 directories and
467,946 files, of which 30,363 matched the source-like extensions. CMake's
recursive glob stalled before the `foreach` body could report a last path; no
child subprocess or pipe wait was involved.

## 14. GNU hang root cause

The root cause was recursive metadata traversal of the whole workspace,
including ignored `build*` trees, generated Evidence Engine/runtime artifacts,
and external/generated files, multiplied by `/mnt/c` filesystem metadata
latency. It was not a test timeout, symlink loop, or CTest subprocess
deadlock.

## 15. File-limit fix

`tests/check_file_limits.cmake` now obtains a deterministic Git inventory for
tracked and non-ignored untracked files under explicit governed pathspecs:
`CMakeLists.txt`, `cmake/`, `src/`, and `tests/`. It filters the same source
extensions, reads every resulting governed file, and fails closed if Git
cannot enumerate them. Ignored build/runtime trees, ROM-derived captures,
`.git` internals, and external installations are outside the policy boundary.
The repaired run reports `inventory=629` and `617 governed files`; it does not
weaken the 500-line limit.

## 16. GNU regression proof

After the fix, the direct GNU file-limit check completed in `3.47 s` and the
full GNU CTest completed `183/183` with zero failures. The GNU configure and
build/link also passed. The known WSL make clock-skew warnings remained
environment warnings and did not affect the successful build/test result.

## 17. Windows validation

The Visual Studio 18 2026 tree was rebuilt in both configurations. Debug CTest
passed `183/183` in `37.13 s`; Release CTest passed `183/183` in `26.61 s`.
The file-limit test completed in `2.76 s` and `2.72 s`, respectively.

## 18. Knowledge Coverage regression

AUTO64 targeted tests passed for import, provenance analysis, coverage
derivation, static queue scheduling, deterministic selection, and anti-repeat
behavior. The tests preserve AUTO63 unresolved evidence, derive an unresolved
A6 frontier into `INV-AUTO64-A6`, and reject a synthetic unrelated frontier
without inventing A6. Existing history was not reopened or deleted.

## 19. SOURCE_OWNED before/after

Unchanged:

```text
BEFORE: 1,475,600 / 3,145,728 = 46.9080607096%
AFTER:  1,475,600 / 3,145,728 = 46.9080607096%
DELTA:  0 bytes
```

## 20. Remaining blockers

No AUTO64.1 validation blocker remains. The repository still contains the
previously existing ignored/generated local build and evidence artifacts; they
were not removed or staged. M12 semantic ownership and the already recorded
AUTO64 investigation statuses remain unchanged and are outside this repair.
