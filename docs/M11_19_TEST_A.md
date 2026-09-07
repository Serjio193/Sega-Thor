# M11.19 Test A — Genesis-Plus-GX live coverage

Date: 2026-09-06
Scope: Test A only; no Stable-Retro, live-map, Atlas, RL or C++ gameplay
translation work.

## Source and tools

- Official upstream: `https://github.com/ekeeke/Genesis-Plus-GX.git`
- Upstream commit: `27426f00aa68f9f358c86919e8a40985326fa05b`
- Baseline worktree: `C:\Github\Genesis-Plus-GX`, branch `master`
- Instrumented worktree: `C:\Github\Genesis-Plus-GX-instrumented`, branch
  `experiment/gpgx-live-coverage`
- Git for Windows: `2.55.0.2`
- MSYS2 UCRT64 GCC: `16.2.0`; GNU Make: `4.4.1`; zlib: `1.3.2-2`
- RetroArch x64: `1.22.2`
- Canonical ROM: 3145728 bytes (`0x300000`), external only

The ROM and all generated reports, bitmaps, replay files and DLLs are outside
the Sega-Thor repository. The baseline source tree remained unmodified; its
generated DLL is untracked in the external worktree.

## Builds

Baseline, from the unmodified upstream worktree:

```bash
cd /c/Github/Genesis-Plus-GX
make -f Makefile.libretro platform=win -j$(nproc)
```

Result: PASS. The generated `genesis_plus_gx_libretro.dll` SHA-256 was
`69DF3811EF2FFDC82BC15FD04E97181A82CC517F3BA84F2123CF1D0F0E639959`.

The required clean hook build was attempted before source changes:

```bash
cd /c/Github/Genesis-Plus-GX-instrumented
make -f Makefile.libretro platform=win HOOK_CPU=1 -j$(nproc)
```

It failed at link time with multiple definitions of `cpu_hook`. The upstream
header provides a tentative definition in every translation unit, while GCC
16's default `-fno-common` rejects that pattern. The minimal cause-directed
change was making the header declaration `extern`; no upstream behavior was
otherwise changed for this build. The final hook build passed with the same
command. Instrumented DLL SHA-256:
`0C4D4BE459D7E5702335467B8C7DD0FB16D77F07D230F9E9B7C0F1AF8819CF73`.

## Instrumentation

The existing upstream `HOOK_CPU` path calls `cpu_hook` with `REG_PC`
immediately before M68K instruction fetch. The added callback does only this:

1. accept `HOOK_M68K_E` events whose PC is below `0x300000`;
2. set one bit for that byte address in a fixed 393216-byte bitmap;
3. increment a cheap instruction-start counter.

The bitmap is address-level and is the source of truth. No 256-byte map cells
are used. The callback contains no logging, file I/O, JSON, disassembly,
allocation, mutex, IPC, GUI or AI. Report and bitmap export occurs only during
core unload/deinit, outside the instruction callback. Checkpoints are bounded
to at most 64 entries and sampled every 60 frames.

## Measurement

Both DLLs loaded the canonical ROM through RetroArch. The identical replay was
the external 600-frame file
`C:\Github\gpgx-test-roms\test-a-600.rpl`, SHA-256
`B36FFD6782DD8ADDD1C84CC992C516B2F18BFF7B16CE918BE56C9F5A3344CB0C`.
This replay contains no input and therefore covers startup/title behavior;
it is not sufficient to prove movement gameplay or human playability.

| Metric | Result |
|---|---:|
| Baseline median, 600 frames | 8.274 ms |
| Instrumented median, 600 frames | 15.177 ms |
| Slowdown ratio | 1.8342x |
| Total M68K instruction starts | 6488885 |
| Unique instruction-start PCs | 2188 |
| First checkpoint unique PCs (frame 60) | 173 |
| Last checkpoint unique PCs (frame 600) | 2188 |
| Core-reported nominal FPS | 59.92 |

Seven fast-forward process wall-clock runs were used for each median. The
timing includes frontend process startup and is not a core-internal timer.

Two independent instrumented runs matched exactly:

- report SHA-256:
  `CBFF415FF07CCF328A451B1B9F27FAB64E5AA17235D8AA23D9CEE88C762BCFA2`
- bitmap SHA-256:
  `BF97D2CCCB1CA1400C3A386B5A62AB8C07C5A185BE5C95BA7C78C595A366674E`

Coverage increased at every recorded checkpoint from 173 to 2188 unique PCs.

## Result

`TEST A: FAIL`.

The hook implementation, ROM-bound bitmap, coverage growth, deterministic
replay and measured slowdown satisfy their individual checks. The overall test
does not pass because the available replay was a no-input startup/title
segment, and native visual/manual input verification was unavailable in this
run. Gameplay correctness and human playability therefore remain unverified.

Exact recommendation: stop here. Do not start Test B or add an alternative
instrumentation architecture. Capture a real input-bearing RetroArch gameplay
replay or run the equivalent segment interactively, then repeat the identical
baseline/instrumented Test A measurements.
