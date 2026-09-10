# M11.61 — Exact callee preservation/effect closure for `0x062AE0`

Status: **COMPLETE — `CALLEE_A5_PRESERVATION_PROVEN_EFFECTS_BLOCKED`**

Baseline: `424c92c3f583e40c70cc82dcf1a7e9488483e4a6`.

## Scope and result

This milestone examined only callee `0x062AE0` while the M11.60 generation
G0 has `A5 = 0x00FF001A`. No other callee, typed RAM model, subsystem,
gameplay meaning, `0x60BCC` path, `0x061258` lifetime or new routine search
was added.

The natural G0 contract is closed for preservation and observed effects:
486/486 entries and returns preserve A5 exactly, with 2,302 observed data
effects and no hardware hook events. The complete static CFG still contains a
reachable indexed `JSR` at `0x062CEC` whose target is not recoverable from
the bounded slice. Therefore the milestone result is
`CALLEE_A5_PRESERVATION_PROVEN_EFFECTS_BLOCKED`: the natural path contract is
usable evidence, while the all-static-callee effect boundary remains blocked
by `INDIRECT_CFG`.

The M11.60 transaction gate remains
`BOUNDED_A5_TRANSACTION_BLOCKED_PARENT_LIFETIME`. Proving this callee does
not promote G0 to a transaction or typed data.

## Phase 1 baseline

Before the callee observer, the authoritative 600-frame run retained:

- checkpoint:
  `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`
- video:
  `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`
- accounting: `6,488,773` total
  (`6,488,692` interpreter + `34` TableCopy + `40` RamFlag +
  `7` ParentSuffix)
- native fallback/divergence: `0/0`
- shadow comparisons/divergence: `5/5` and `0`

## Exact callee CFG boundary

The bounded slice rooted at `0x062AE0` reaches the following returns:

`0x062B1A`, `0x062C36`, `0x062CBC`, `0x062D0A`,
`0x062D62` and `0x062D6A`.

It has one direct nested call, `0x062B4E BSR.W 0x062D4C`, and one unresolved
indexed call, `0x062CEC JSR (...,D3.W)`. Direct branches reach the
`0x062CE2`, `0x062CF0` and `0x062D4C` regions; the decoder reports one
unresolved control-flow edge at `0x062CEC` and no unsupported instruction in
the expanded slice. The boundary classification is **`INDIRECT_CFG`**.

The natural G0 path never reaches either nested call or any later return.
It exits through the early `0x062B1A RTS`; this is a dynamic path fact and
does not erase the latent static CFG edge.

## A5 preservation

Every natural entry from the M11.60 parent call at `0x0601E2` has
`A5=0x00FF001A` on entry and on the observed return at `0x0601E6`:

- entries: 486
- returns: 486
- equal A5 entry/exit: 486
- unequal A5 entry/exit: 0
- direct nested calls observed: 0
- indexed nested calls observed: 0

The static slice contains no direct write to A5 and no A5 spill/reload. The
natural classification is **`A5_PRESERVED_EXACT`** for the observed G0 path.
The latent indirect CFG path keeps whole-callee preservation outside this
narrow natural proof.

## Complete natural effect set

The observer recorded every type-2 read and type-4 write while the callee was
active. All observed addresses are safe main RAM, G0-relative RAM or stack;
hook types were execution/data only: type 1 = 3,446, type 2 = 1,636,
type 4 = 666, and all other hook-type counts are zero.

| PC | natural effect | address class | count |
|---|---|---|---:|
| `0x062AE0` | read `FF0013`, byte | SAFE_RAM | 486 |
| `0x062AFE` | read `FF001A`, byte | G0_RELATIVE (+0) | 486 |
| `0x062B08` | write `FF001E`, byte | G0_RELATIVE (+4) | 175 |
| `0x062B0C` | read `FF05B2`, byte | SAFE_RAM (A6-derived) | 175 |
| `0x062AEA` | write `FF077C`, byte | SAFE_RAM (A4-derived) | 1 |
| `0x062AEE..0x062AFA` | exceptional A4/A6-derived byte reads/writes in `FF05C5..FF0626` and `FF077C..FF077F` | SAFE_RAM | 8 |
| `0x062B16` | write `FF077C` or `FF0780`, byte | SAFE_RAM (A4-derived) | 486 |
| `0x062B1A` | read return longword from `FF0B16..FF0B66` | STACK | 486 |

The exceptional row is one initialization path; its exact addresses are
`FF0624` read, `FF077D` write, `FF0626` read, `FF077E` write,
`FF05C5` read, `FF077F` write and `FF05C7` write. The return longword
reads vary with the saved stack and remain stack effects, not G0 aliases.
No ROM read, hardware access, derived G0 alias or unresolved runtime address
was observed.

## Natural paths and ordering

Two repeated traces are byte-identical:

`80D4CDCBC9BD85013C70B15FE621070F5BB65AFEC8B78A43784D0790A04BDAC4`.

Across all 486 calls there are three path sequences:

1. 310 calls: `62AE6 -> 62AFE -> 62B04 -> 62B16 -> 62B1A`.
2. 175 calls: `62AE6 -> 62AFE -> 62B04 -> 62B08 -> 62B0C -> 62B12 ->
   62B16 -> 62B1A`.
3. 1 call: `62AE6 -> 62AEA -> 62AEE -> 62AF2 -> 62AF6 -> 62AFA ->
   62AFE -> 62B04 -> 62B16 -> 62B1A`.

Each sequence begins after the `0x062AE0` entry read of `FF0013`, then
performs its listed G0/A4/A6 effects, and ends with the `0x062B1A` stack
return read before `0x0601E6`. No interrupt or hardware hook event occurs
inside the observed callee interval.

## Nested-call minimum contract

The direct nested `0x062D4C` call and the indexed `0x062CEC` call have zero
natural executions for every G0 entry. Thus they introduce no natural A5 or
G0-window effects in this scenario. Their static reachability, especially the
indexed target at `0x062CEC`, prevents a whole-callee effect claim for paths
outside the observed G0 distribution. No recursive subsystem reconstruction
was performed.

## M11.60 and typed-data update

This result proves the `0x062AE0` natural preservation/effect contract only.
M11.60 still has unresolved preservation/effect evidence for other G0-crossed
callees and the parent-owned endpoint at `0x06027E`; therefore the raw
transaction gate remains blocked. M11.59 external writers, the
`0x061258` A5 escape and M11.58 alternate consumers remain unchanged.
The typed-data gate remains fail-closed:
`TYPED_DATA_BLOCKED_OVERLAPPING_ACCESS`.

Astra was unavailable locally. The static decoder and repeated runtime trace
are authoritative.

The post-change native and shadow 600-frame identity checks retain the same
checkpoint and video hashes. Native override accounting is 6,488,692
interpreter + 81 translated = 6,488,773 total with zero fallback/divergence;
shadow remains 5/5 comparisons with zero divergence and the expected five
emulated fallback entries.

## Regression and governance

`Callee62AE0Observer` and its runtime integration are developer-only
`src/tools/hybrid` code. `tests/hybrid_callee_62ae0_test.cpp` locks the
entry/return equality, zero nested-call contract and representative
FF0013/FF001A/FF077C effects. No ROM/GPGX type enters `oasis_core`.

CI: GitHub Actions run 34445181596 passed build and test for commit
`de248d98b5ca9fee058c7d26dd031e0eb202ed26`.
M11.62 may target only the dominant remaining blocker in the already-bounded
G0 lifetime: one remaining callee/effect or the latent `0x062CEC` CFG edge.
