# M11.43 — Complete GPGX Checkpoint Canonicalization Contract

## Result

`CHECKPOINT_CANONICALIZATION_COMPLETED_NEW_AUTHORITATIVE_IDENTITY`

This milestone started from `c59dd1bb62e03df0639484de214a482204eb0d63` and
resolved the M11.42 blocker
`M11.42_BASELINE_BLOCKED_CHECKPOINT_CANONICALIZATION_INCOMPLETE`. It did not
resume M11.42 PHASE 2, classify interpreter executions, add semantics, generate
blocks, run shadow promotion or change the 28-range registry.

## Pinned inputs and layout evidence

The controlled ROM is
`C:\Github\gpgx-test-roms\Beyond Oasis (USA).md`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. The
external source checkout is commit `d60d079934977aa6973e220d123533387159f66e`
and the pinned DLL is SHA-256
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.

The contract guards state version `GENPLUS-GX 1.7.6` and `STATE_SIZE=0xfd000`.
The exact x64 model proves:

| type | size | relevant offsets |
| --- | ---: | --- |
| `FM_SLOT` | 80 | `DT=0` |
| `FM_CH` | 400 | `connect1=336`, `connect3=344`, `connect2=352`, `connect4=360`, `mem_connect=368` |
| `FM_ST` | 1060 | raw nested YM state |
| `FM_3SLOT` | 32 | raw nested YM state |
| `FM_OPN` | 1168 | raw nested YM state |
| `YM2612` | 3576 | serialized raw base `140652` |
| `Z80_Regs` | 88 | `daisy=72`, `irq_callback=80`; serialized base `144504` |

The YM base is the byte after the serialized `config.ym3438` flag at `140651`.
The observed pinned-DLL Z80 base is independently identified by its
`daisy` pointer at `144576` and by the following 64-byte active cartridge
mapping at `144592`. The available `state.c` source arithmetic would place this
boundary 90 bytes earlier; the external checkout's `state.o`/DLL timestamps
prove a source/object artifact mismatch. The canonicalizer therefore fails
closed to the pinned binary contract instead of silently trusting a conflicting
source-only offset.

The active USA cartridge path serializes mapping bytes and `cart.hw.regs`
field-by-field; its `cart_hw_t` function pointers are not serialized. Optional
SVP wholesale state is not present in this active baseline and is not included
in the contract.

## Representation table and raw classification

The table is generated from mirrored field offsets and checked for bounds and
non-overlap. It contains 55 host-pointer spans (24 `FM_SLOT.DT`, 30 YM channel
connection pointers and `Z80_Regs.daisy`), one function-pointer span
(`Z80_Regs.irq_callback`) and 111 ABI-padding spans. Total ABI-padding bytes are
281. No address-specific or semantic spans are cleared.

Three independent raw current-process runs were captured; their complete
10,362,880-byte evidence files were identical. A fresh canonical run and the
preserved M11.41 current raw evidence differ at exactly 110 bytes per record
across 10 records, 1,100 bytes total. Every differing byte is in the proven YM
host-pointer spans or the Z80 `daisy` span. There are no differing semantic or
unknown bytes.

The historical M11.35 rejection at `0x03A7AE` remains preserved verbatim in the
earlier reports; M11.43 does not reinterpret it or resume candidate attribution.

## Identity proof

The new canonicalizer copies the full raw buffer, checks exact version and size,
and clears only the representation table. It leaves the raw evidence untouched.
Adversarial tests cover every representation span, nearby semantic mutation,
unknown version rejection, bounds, non-overlap, idempotence and offset `140734`.

Five independent current 600-frame `BASIC_BLOCK_NATIVE` proofs agree on:

| result | value |
| --- | --- |
| authoritative aggregate | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` |
| video SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` |
| total guest instructions | 6,488,773 |
| translated instructions | 5,826,857 |
| interpreter instructions | 661,916 |
| translated share | 89.7991% |
| registered ranges | 28 |
| boundary yields | 140,065 |
| interrupted resumptions | 274 |
| original starts inside translated ranges | 0 |

Canonical replay of raw evidence from current, exact M11.39 checkout and the
preserved M11.41 baseline produced the same new aggregate. The M11.41
`c9236218f55fb18f7f1d5095e4970b25f228de588bd03bd7cbbffccc2e225fd1` identity is
superseded: its adapter cleared semantic bytes in addition to representation
bytes. M11.39, M11.40 and M11.41 history remains unchanged.

The complete project checks passed after the final source change: MSVC Debug
CTest 56/56, MSVC Release CTest 56/56, and GNU-equivalent MinGW CTest 56/56.
The Release layout test runs with assertions enabled. `git diff --check` and
the project source-file line-limit check also passed.

## Scope and hygiene

Implementation changes are limited to developer-only hybrid tooling and its
tests. Generated blocks, production runtime, ROM/assets, emulator binaries and
run evidence were not tracked. `game.srm` remains untouched and untracked.
M11.42 PHASE 2 and the 95% coverage decision remain the next separate
milestone.
