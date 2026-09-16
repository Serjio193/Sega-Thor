# M12-AUTO67-BIZHAWK-WATERBOX-PHASE-0A — stock GPGX Waterbox reproduction

**Baseline:** `36adb35df38dabd2cad65f90b2f502f442db3afd`
**Classification:** `STOP_STOCK_REBUILD_PARITY_FAILURE`

Phase 0A reproduced the unmodified BizHawk 2.11.1 GPGX Waterbox build under
WSL2 Ubuntu 24.04. No native-ring patch, AUTO67 production code, Lua transport,
Dispatcher, Worker, resolver, Cartographer, Session Map, or C++ source was
changed. The authoritative Windows install was not overwritten.

## Exact source and toolchain

* WSL distribution: `Ubuntu-24.04`, WSL2; native filesystem:
  `/home/serji/bizhawk-2.11.1` (not `/mnt/c`).
* BizHawk commit: `bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5`.
* Genesis-Plus-GX submodule: `051d430d3d1b54625f9900c8f152d7f232e06daf`.
* Musl submodule: `2063abc4e16c84218757b1db10d3cdf9f36ef3f8`.
* `clang-18`: Ubuntu clang `18.1.3 (1ubuntu1)`.
* Sysroot wrapper: `/home/serji/bizhawk-2.11.1/waterbox/sysroot/bin/musl-clang`.
  SHA-256: `9125791649f7cb2409b80f8282c20c2336e80c70f0ba820e08c4e251a78dc14f`.
* libcxx procedure completed with `LLVM_TAG=llvmorg-18.1.8`.

The source checkout was clean before compilation and contains no
`trace_ring.c`/native-ring additions.

## Build receipt

The following stock commands completed successfully:

```text
CC=clang-18 ./waterbox/musl/wbox_configure.sh
./waterbox/musl/wbox_build.sh
make -C waterbox/emulibc -j2
make -C waterbox/libco -j2
./waterbox/libcxx/do-everything.sh
./waterbox/nyma/build-and-install-zlib.sh
make -C waterbox/gpgx release
```

The only compiler diagnostics were upstream warnings in GPGX (`flash_cfi.c`,
`memz80.c`, `z80.c`) and zlib's existing fallback warning; all commands exited
zero.

Rebuilt artifact:

```text
/home/serji/bizhawk-2.11.1/waterbox/gpgx/obj/release/gpgx.wbx
size 4,026,304 bytes
sha256 0d0102f3e74a2f93f490db6a0865fae1c1af78a614c5c9290001f90ca059a3ca
```

The shipped authoritative install contains only the compressed artifact
`C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\dll\gpgx.wbx.zst`, size 425,315,
SHA-256 `23A05F32CEB790F21AC7550E388401861E7A5333731AFE69A351E86D916D5845`.
A separate test copy was created at
`C:\Dev\SegaThorTools\BizHawk-2.11.1-stock-rebuild-test`; the rebuilt raw
`gpgx.wbx` was placed there and the compressed shipped file was preserved as
`gpgx.wbx.zst.official-preserved`.

## GUI and ROM smoke proof

The test copy launched:

```text
EmuHawk.exe "C:\Github\Sega-Thor\local-roms\Beyond Oasis (USA).md" --load-slot 1
```

The window reached `Beyond Oasis (U) [!] [Genesis] - BizHawk` and remained
responsive. With the rebuilt raw core selected, a Lua smoke script executed
10 frames of `Right` input and exited 0:

```text
start=1
end=11 input=Right frames=10
```

This proves core load, ROM/video frontend startup, QuickSave1 option handling,
and input dispatch at the frontend level. The saved state itself is the parity
blocker below.

## Rebuild parity stop

The same bounded Lua probe was run against the official installation and the
separate rebuilt-core installation with the canonical ROM
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`,
`--load-slot 1`, three neutral frames, then ten `Right` frames. The probe
recorded PC, A4, A5, two RAM bytes, VDP write count, and frame count.

| installation | loaded | after neutral 3 | after Right 10 |
|---|---|---|---|
| official shipped core | frame 2118, PC 35706, A4 239, A5 44374 | frame 2121, PC 35536, A4 239, A5 44374, VDP 16 | frame 2131, PC 35536, A4 239, A5 44374, VDP 128 |
| rebuilt stock core | frame 1, PC 620, A4 12582916, A5 760 | frame 4, PC 920, A4 12582916, A5 0, VDP 43 | frame 14, PC 924, A4 12582916, A5 0, VDP 43 |

The rebuilt core loads and runs, but the existing QuickSave1 cannot be restored
into the rebuilt binary (the run remains at frame 1 rather than the required
frame 2118). Therefore the required same-state behavioral parity is not proven
and the valid stop is `STOP_STOCK_REBUILD_PARITY_FAILURE`. No production
runtime was switched to this artifact and no native-ring work follows this
checkpoint.

## Validation receipt

* Stock Waterbox sysroot/emulibc/libco/libcxx/nyma/GPGX build: **PASS**.
* Core load, ROM startup, QuickSave1 launch option, and input smoke: **PASS**.
* Same QuickSave1 parity at PC/A4/A5/RAM/VDP checkpoints: **STOP** as above.
* Debug CTest (`build-current-debug`): **195/195 PASS**.
* Release CTest (`build-current-release`): **194/195 PASS**; existing
  `oasis_re_import_gpgx_coverage_self_test` aborts with `invalid JSON at 0:
  missing value` (no source change in this checkpoint).
* Focused AUTO67 regressions: unchanged; no production code changed.
* Source-limit: **PASS**.
* `SOURCE_OWNED = 1,475,600 / 3,145,728`, delta 0.
* `git diff --check`: **PASS**.
* `github_ci`: `PENDING_EXTERNAL_VERIFICATION`.

Stop after Phase 0A. The stock Windows installation remains authoritative.
