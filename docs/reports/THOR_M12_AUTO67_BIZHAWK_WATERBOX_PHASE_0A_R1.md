# M12-AUTO67-BIZHAWK-WATERBOX-PHASE-0A-R1 — cold-reset parity

**Baseline:** `6109e6bbc15b6d6d87493fb5d6e2357b68f48f7f`
**Classification:** `PASS_STOCK_REBUILD`

Phase 0A-R1 validates the unmodified local GPGX rebuild against the shipped
GPGX from independent clean power-on boots. The previous QuickSave1 result is
reclassified as `CROSS_WBX_STATE_INCOMPATIBLE`; it is not a CPU/emulation parity
oracle. Waterbox consistency checks were not disabled.

## Source and boundary

* BizHawk: `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`.
* Genesis-Plus-GX: `051d430d3d1b54625f9900c8f152d7f232e06daf`.
* WSL2 Ubuntu 24.04 native checkout: `/home/serji/bizhawk-2.11.1`.
* Rebuilt source is clean and unmodified; no native-ring patch was applied.
* Authoritative install `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64` was not
  overwritten. Rebuilt core was tested only in the separate
  `C:\Dev\SegaThorTools\BizHawk-2.11.1-stock-rebuild-test` copy.
* Canonical ROM SHA-256:
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0.

## Cold-reset scenario

Both installations were launched from clean ROM power-on with the same
controller schedule and no state load:

* steps 1–30: neutral;
* steps 31–60: `Right` held;
* steps 61–300: neutral;
* checkpoints: 1, 10, 30, 60, 120, 300.

The bounded Lua probe did not use global execution tracing. At every checkpoint
both cores produced identical frame, M68K PC, A4, A5, selected RAM, and VDP/DMA
write counters:

| step | frame | PC | A4 | A5 | RAM FF188A | RAM FF188C | VDP writes | DMA writes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2 | 618 | 12582916 | 760 | 0 | 0 | 0 | 0 |
| 10 | 11 | 906 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 30 | 31 | 912 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 60 | 61 | 920 | 12582916 | 0 | 0 | 0 | 43 | 43 |
| 120 | 121 | 13038 | 12582916 | 12582912 | 0 | 0 | 92 | 92 |
| 300 | 301 | 13044 | 12582916 | 12582912 | 0 | 0 | 260 | 260 |

The official and rebuilt checkpoint files were byte-for-byte identical for this
record set. Result: **cold-reset deterministic parity PASS**; no first
divergence exists in the 300-frame interval.

## Rebuilt-core self-savestate

A separate test used only the rebuilt core: run 120 frames, save a new state,
advance 20 `Right` frames, reload that rebuilt-core state, replay the same 20
frames, and compare frame/PC/A4/A5/selected RAM.

```text
save=PASS
load=PASS
after20  frame=141 pc=13044 a4=12582916 a5=12582912 ram_ff188a=0 ram_ff188c=0
replay20 frame=141 pc=13044 a4=12582916 a5=12582912 ram_ff188a=0 ram_ff188c=0
equal=true
```

Result: **rebuilt-core self-savestate PASS**.

## Cross-WBX state result

Loading the shipped QuickSave1 into the rebuilt `.wbx` is recorded separately
as `CROSS_WBX_STATE_INCOMPATIBLE`. The official installation restored frame 2118;
the rebuilt installation stayed at frame 1. This is expected executable/
initial-memory consistency behavior and is excluded from parity conclusions.
No consistency check was bypassed.

## Validation

* Stock Waterbox sysroot, emulibc, libco, libcxx, nyma/zlib, and GPGX build:
  **PASS**.
* Core load, ROM startup, frontend input, and cold-reset run: **PASS**.
* Cold-reset parity through 300 frames: **PASS**.
* Rebuilt-core save/load/replay: **PASS**.
* Debug CTest (`build-current-debug`): **195/195 PASS**.
* Release CTest (`build-current-release`): **194/195 PASS**; existing
  `oasis_re_import_gpgx_coverage_self_test` fails with `invalid JSON at 0:
  missing value`.
* `git diff --check`: **PASS**.
* Source-limit: **PASS**.
* `github_ci`: `PENDING_EXTERNAL_VERIFICATION`.

Stop after Phase 0A-R1. Native-ring work remains a separate future checkpoint.
