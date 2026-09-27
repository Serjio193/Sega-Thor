# Runtime ROM Properties v1 — corrected runtime re-seal

**Date:** 2026-09-27
**Status:** `PASS_SEAL_REPLAY`; M0–M9 remains partial
**Scope:** Re-seal the post-`ARG16()` fix, replay it through the canonical map,
then record bounded parity and performance results. No property definitions
were added.

## Sealed identity

| Identity | Value |
|---|---|
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| ROM size | 3,145,728 bytes |
| Classifier schema | `thor.rom-properties.v1` |
| Proof contract SHA-256 | `10da481265de188d7a11049f34d24dd33f3b0e34b5ff797d12541ee241356c78` |
| Runtime/build ID | `e202add134725f7460c2a67d866d0b2e1c97ba5f9175ca5b4f76a4e961640889` |
| Release core SHA-256 (live run) | `dcd39613aef68a945f29927ead467326f6ac892d92f924466ee6f4b5f3b0f630` |
| Debug core SHA-256 | `c43623cd54407e9554f9a505f55402406e8a79bdd2ebcca07157403f65bfb12f` |
| Run ID | `corrected-live-20260927-120f-01` |
| Capabilities | `5` (`M68K_EXECUTED_ENCODING`, `M68K_DATA_READ`) |
| Validation state | `PARTIAL` |
| Checkpoint SHA-256 | `9233d8d626519180b1e4d06cb6a541ea0d9f470689337921a7078483b9056c69` |

The canonical JSON build manifest binds both Waterbox configurations, pinned
BizHawk/GPGX/musl revisions, compiler identity, relevant source hashes, ROM
identity, schema, proof-contract hash, managed assemblies, and the Release
EmuHawk executable. Its SHA-256 is the runtime/build ID above. The live
checkpoint uses the Release artifact; Debug is pinned as the paired build.

Local artifacts are under ignored
`build/runtime-rom-properties-live/`: `corrected-core-build-manifest.json`,
`corrected-live.rom-properties.v1`, `corrected-live.ranges.tsv`, and
`corrected-live-overlay.json`. The raw 120-frame live dump is
`corrected-live-map.hex`.

## Replay acceptance

| Check | Result |
|---|---|
| Fresh corrected Release live run | PASS, 120 neutral-input frames |
| ROM identity | Exact |
| Runtime/build identity | Exact in checkpoint and range metadata |
| Proof-contract identity | Exact in checkpoint and range metadata |
| Range partition | 172 ranges; full coverage; 0 gaps; 0 overlaps |
| Bitmap → ranges → bitmap | Exact |
| Canonical overlay | PASS, 2,625 combined ranges |
| Canonical map hash before/after | Same: `a901cde397632b9b5153682f683fbab281358883c6bedd0e8da3e3312ae8bc13` |
| `SOURCE_OWNED` before/after | 1,487,672 / 1,487,672 bytes |
| Ownership delta | 0 |
| Unauthorized canonical promotions | 0 |

The runtime bitmap contains 502 bytes with no proven property, 3,141,722 bytes
with `M68K_DATA_READ`, and 3,504 bytes with both M68K bits. This broad data-read
footprint remains raw access evidence; it does not imply semantic ownership or
explain the caller responsible for every read.

After this neutral run was sealed and replayed, a separate 3,600-frame
`menu-start-directional-v1` controlled-input run used the same corrected
Release core/build ID and proof contract. It was sealed independently as
`scenario-menu-start-directional-20260927-3600-01` with capabilities 255 and
validation state `PARTIAL`. Its checkpoint SHA-256 is
`a9eef1d7d9c53ba47cf93451b90ff59049df8d9d68c068d371bf9a879c9c0e46`.
That checkpoint also has a fresh 2,362-range export, exact bitmap roundtrip,
full coverage with no gaps/overlaps, and a 4,643-range canonical overlay with
zero ownership delta and zero promotions.

| Scenario property mask | Bytes | Proven live use |
|---:|---:|---|
| `0x0000` | 502 | No accepted property |
| `0x0004` | 2,932,738 | M68K direct ROM data read |
| `0x0005` | 42,322 | M68K direct ROM data read and completed instruction bytes |
| `0x000C` | 40,182 | M68K read plus direct Z80 banked ROM data read |
| `0x0014` | 129,984 | M68K read plus direct ROM-backed VDP VRAM source |

The controlled-input baseline and corrected-runtime parity traces are also
byte-identical for all 3,600 frames. Their SHA-256 is
`11b4316283bb84c7be5715fc64803b7ae3e62019646d089444c487ce0550300e`; final
M68K RAM hash is `f5da3738`, and Z80 bus `0000–1FFF` hash is `e6a598fa`. This is
broader deterministic input parity, still not full-game behavioral parity.

## Z80 regression and bounded parity

The permanent `oasis_runtime_rom_properties_z80_arg16` regression test checks
that the generated `ARG16()` replacement reads and observes both operand
bytes, advances PC by exactly two before returning, and leaves the next opcode
aligned in a short instruction-stream fixture. It passes in Windows Debug and
Release.

Parity used cold ROM boots, neutral input on every frame, and compared a
per-frame trace of M68K PC/SR and Z80 PC, plus final M68K/Z80 registers,
selected RAM markers, and M68K/Z80-bus RAM hashes. Baseline and corrected
instrumented traces are byte-identical:

| Frames | Result | M68K RAM FNV-1a | Z80 bus `0000–1FFF` FNV-1a |
|---:|---|---|---|
| 120 | PASS | `8511990e` | `e6a598fa` |
| 1,200 | PASS | `948c6400` | `e6a598fa` |
| 10,000 | PASS | `ac974310` | `e6a598fa` |

These are neutral-input runtime parity checks. They do not establish full-game
behavioral parity.

## Performance

Each run started from a fresh boot, used 1,200 neutral-input frames, disabled
clock throttling and VSync, and ran in the same Windows 11 host. Three repeats
were run per mode in rotated order. Mean FPS and sample standard deviation:

| Mode | Mean FPS | Sample SD | Individual FPS |
|---|---:|---:|---|
| A — clean baseline | 1,083.712 | 23.964 | 1,059.135; 1,107.011; 1,084.991 |
| B — instrumented, map disabled | 988.804 | 60.238 | 922.367; 1,004.184; 1,039.861 |
| C — map enabled | 761.212 | 32.734 | 744.417; 740.284; 798.935 |

| Overhead | Relative FPS decrease |
|---|---:|
| Instrumentation, A → B | 8.76% |
| Property map, B → C | 23.02% |
| Total, A → C | 29.76% |

The exact normalized emulator configuration hash used across both installs was
`e10445dfd575cdcdf978090d30bb547754f61ad7ff85863f1886d8c16859400a` after
excluding only path/recent-file fields and applying the same explicit
throttle, VSync, background, and cold-start settings.

All six benchmark runs had identical final M68K RAM hash `948c6400` and matching
PC/SR/D0/A7. The baseline and instrumented installs use the same EmuHawk
executable hash, but their `BizHawk.Emulation.Cores.dll` hashes differ
(`45d306…d2eb2` and `5e9f3e…05ca9a`). The A→B estimate therefore includes that
managed-assembly difference. FPS uses process CPU time with millisecond-scale
quantization; treat these as reproducible bounded measurements, not universal
hardware performance figures. No optimization was performed before recording
these results.

The full raw receipt, traces, configuration files, and per-run outputs are in
ignored `build/runtime-rom-properties-live/parity-performance/`.

## Milestone status after re-seal

- **M2 — limited PASS:** live M68K instruction/data observations are sealed for
  this neutral-input scenario. Data-read caller interpretation remains open.
- **M3 — limited PASS:** 40,182 bytes have accepted direct `Z80_DATA_READ`
  evidence in the controlled-input run. No `Z80_EXECUTED_ENCODING` bytes were
  proven because the observed execution path did not establish ROM origin for
  fetched instruction bytes. The permanent `ARG16()` regression passes.
- **M4 — limited PASS:** 129,984 bytes have accepted direct
  `VDP_VRAM_SOURCE` evidence. CRAM and VSRAM source paths remain unobserved.
- **M5 — PARTIAL tooling only:** exact-copy origin primitives are covered by
  host-side tests, but live exact-copy provenance has not been connected or
  accepted. Unsupported transformed values remain UNKNOWN. Audio is still
  open and does not gate completion of the base map.
- **M9 — PARTIAL:** both corrected checkpoints, range replays, canonical
  overlays, three-repeat A/B/C benchmark, and neutral plus controlled-input
  parity are present. Full-game parity and remaining subsystem coverage are
  still missing.

## Verification

GPGX Waterbox Debug and Release builds pass. The focused Windows Debug and
Release and WSL GNU/Linux Release property tests pass 4/4 each, including the
permanent Z80 regression. GNU/Linux build on the Windows-mounted checkout
reported filesystem clock-skew warnings; compilation completed and all four
tests passed.
`git diff --check` and the source-size limit check pass. No ROM or copyrighted
game data was added to the repository.
