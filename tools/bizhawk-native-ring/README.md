# BizHawk 2.11.1 native GPGX trace ring probe

This is an experimental, reproducible patch for the exact BizHawk 2.11.1
source commit used by AUTO67. It is not the project default and is not wired
into the Sega-Thor harness.

## Source and patch identity

* BizHawk tag: \`2.11.1\`
* BizHawk commit: \`bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5\`
* Genesis-Plus-GX submodule commit: \`051d430d3d1b54625f9900c8f152d7f232e06daf\`
* \`bizhawk-2.11.1-native-ring.patch\` SHA-256:
  \`A6E303140D5A85E11C604A0CFDFAD42413395C49E4A70B1415C1AA8A627609E5\`
* \`genesis-plus-gx-native-ring.patch\` SHA-256:
  \`139C2A15D27DDDED659BED964349076A3E2EFC91B8B43BDFBD350AA8200D85EB\`

The first patch changes the BizHawk C# interop declaration, the GPGX Waterbox
makefile, and the C interface. The second patch changes the M68K execution
point and adds \`core/debug/trace_ring.[ch]\`.

## Apply and build

Run from a clean checkout of the exact tag:

\`\`\`text
git clone --branch 2.11.1 https://github.com/TASEmulators/BizHawk.git bizhawk-2.11.1
cd bizhawk-2.11.1
git checkout bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5
git submodule update --init waterbox/gpgx/Genesis-Plus-GX
git -C waterbox/gpgx/Genesis-Plus-GX checkout 051d430d3d1b54625f9900c8f152d7f232e06daf
git apply C:/Github/Sega-Thor/tools/bizhawk-native-ring/bizhawk-2.11.1-native-ring.patch
git -C waterbox/gpgx/Genesis-Plus-GX apply C:/Github/Sega-Thor/tools/bizhawk-native-ring/genesis-plus-gx-native-ring.patch
make -C waterbox/gpgx release
\`\`\`

The required BizHawk Waterbox sysroot/compiler must already be present. This
checkpoint does not commit a custom \`gpgx.wbx\` or any other binary. In the
development checkout used for this probe the build stopped before compilation
because \`waterbox/sysroot/bin/musl-clang\` and \`musl-gcc\` were absent; therefore
no custom binary hash or parity/performance receipt is claimed.

## Ring and bulk API

The native record is a fixed 16-byte structure:

\`\`\`c
uint64_t sequence;
uint32_t pc;
uint16_t opcode;
uint16_t reserved;
\`\`\`

Capacity is \`4096\`. The hot path records the PC immediately before opcode fetch,
then records the fetched 16-bit opcode with a sequence increment and one ring
slot write. It performs no allocation, string formatting, disassembly, managed
call, register/RAM/VDP mutation, or branch/timing operation.

The exported bulk functions are:

* \`gpgx_native_trace_ring_latest()\`
* \`gpgx_native_trace_ring_count()\`
* \`gpgx_native_trace_ring_copy(start_sequence, destination, capacity)\`
* \`gpgx_native_trace_ring_reset()\`

\`gpgx_native_trace_ring_copy\` returns ordered records beginning at the requested
sequence, clamped to the records still retained by the ring. The C# declarations
are present for a future evidence-format adapter; no Lua callback is installed
by this patch.

## Status

This is a probe artifact only. The stock Lua capture backend, predecessor
resolver, Dispatcher, Worker, Cartographer, MAP-1, Session Map, and SOURCE_OWNED
remain unchanged. The custom build and required matched-slice parity test must
be completed in a toolchain-equipped environment before any integration
decision.

`bizhawk-native-snapshot-freeze.patch` is an additional experimental managed
host patch for the 1B freeze-to-Worker proof. It exposes one synchronous Lua
call that copies the native ring after an ordinary discovery occurrence; it
does not add an instruction callback or change the ring recorder. Apply and
build it only in the same isolated native-ring source checkout/install.

## 1B synchronous freeze-to-Worker managed bridge

The additional `bizhawk-native-snapshot-freeze.patch` SHA-256 used for the 1B
runtime proof is `328b87d7ba61ceae5836a85d96d85d81e1141f9c1b9f0a6643def0aa6d45e5cc`.
It changes only `BizHawk.Client.Common/lua/LuaHelperLibs/GenesisLuaLibrary.cs`:
the diagnostic Lua method resolves the existing native latest/count/copy
exports through the Waterbox departure-only adapter, copies one ordered ring
snapshot synchronously, and returns a compressed record payload plus bounds and
timing metadata. It adds no per-instruction Lua callback and does not alter the
native ring or production capture path. The exact isolated Release host build
passed; the resulting runtime did not enter the existing register resolver,
which is recorded as `STOP_NATIVE_RESOLVER_OUTPUT_MISSING` in the 1B receipt.

## Live-forward Worker 1B scaling

Use a clean BizHawk 2.11.1 checkout at the pinned commit above, with
`core.autocrlf=false`, and apply the cumulative Worker 1A patch pair first.
Those two `*live-forward-worker-1a.patch` files include the earlier ring work
from the clean baseline. Then apply the incremental Worker 1B patches below;
the first targets the BizHawk repository and the second its GPGX submodule.
Run these commands from WSL/Linux, replacing `/mnt/c/Github/Sega-Thor` with the
workspace mount path if needed:

```sh
git apply --ignore-whitespace /mnt/c/Github/Sega-Thor/tools/bizhawk-native-ring/bizhawk-2.11.1-live-forward-worker-1a.patch
git -C waterbox/gpgx/Genesis-Plus-GX apply --ignore-whitespace /mnt/c/Github/Sega-Thor/tools/bizhawk-native-ring/genesis-plus-gx-live-forward-worker-1a.patch
git apply --ignore-whitespace /mnt/c/Github/Sega-Thor/tools/bizhawk-native-ring/bizhawk-2.11.1-live-forward-worker-1b.patch
git -C waterbox/gpgx/Genesis-Plus-GX apply --ignore-whitespace /mnt/c/Github/Sega-Thor/tools/bizhawk-native-ring/genesis-plus-gx-live-forward-worker-1b.patch
```

The incremental BizHawk patch SHA-256 is
`6C7C92F9FA75D8A1D28F5C08A3787F1B550C43C483181FA7584EFCF79CE383BA`; the
GPGX submodule patch SHA-256 is
`1C4F8D33CDD46613685E1909E39A5056E6C8A23303BE68BA67B60DFB9E55797B`.
The first patch adds managed budget, metrics and lifecycle APIs; the second
adds dynamic native Worker pool and per-slot transition counters. The host
patch deliberately does not change the submodule gitlink.

`live_forward_scaling.lua` drives one hundred serialized capture cycles for
every configured Worker slot. The Python host validates each FLOW_V1 segment,
compares a second binary export and ENTRY/EXIT metadata after CPU execution
advances, writes the exact identity ACK, and requires per-slot native
start/completion/analysis/release counters to match the generation before the
next cycle. Each geometric Worker-count step has bounded native-memory and
two-pass host-disk preflight. Natural depth-20 scaling and forced
depth-at-least-count concurrency remain separate measurements.

The runtime requires the 1B BizHawk/GPGX patches and an installed Waterbox
sysroot. Run it against the isolated 2.11.1 install and canonical ROM:

```powershell
python tools/bizhawk-native-ring/live_forward_scaling_runtime.py `
  --install build/thor-evidence/live-forward-worker-1b/coherent-bizhawk `
  --rom "build/reference/Beyond Oasis (USA).bin" `
  --script tools/bizhawk-native-ring/live_forward_scaling.lua `
  --output-dir build/thor-evidence/live-forward-worker-1b
```

The runner requires 100 cycles per slot and stops before allocation when the
native budget or available disk space cannot hold the pool and one two-pass
result wave. A pool startup or a single capture cannot produce PASS. On
2026-09-17 the recovered musl sysroot built WBX SHA-256
`4AC692A115CB5543BB3C2260FC04FDACD2CF5D963DF59C17D87A12A30ADD3CD4`. The
tested coherent install preserves the old `gpgx.wbx.zst` sidecar under a
backup name: BizHawk prefers that compressed file over an adjacent
`gpgx.wbx`, so do not restore the stale sidecar as `gpgx.wbx.zst` during this
campaign.

The real 100-cycle campaign verified natural counts 1 through 64 and forced
counts 2 through 128. Natural 128 and forced 256 stopped because CPU execution
stalled before immutable reread; neither failed count passed and no larger
count was attempted. This is a runtime correctness stop, not a measured RAM
ceiling. Per-count receipts and audit JSONL files are under
`build/thor-evidence/live-forward-worker-1b/campaign-100cycles-proven/` and
`campaign-forced-100cycles-proven/`. The result and remaining CI publication
status are in
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.md` and its JSON
receipt.
