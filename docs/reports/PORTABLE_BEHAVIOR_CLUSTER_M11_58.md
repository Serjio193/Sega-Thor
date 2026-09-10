# M11.58 — Portable behavior cluster boundary and data-ownership gate

STATUS: **PORTABLE_BEHAVIOR_CLUSTER_CONTRACT_PROVEN_REPLACEMENT_BLOCKED**

BASELINE: `f5f0118325dad3b36961a546ca5e06fd866d8f95`

## Result

The existing `ParentSuffix` contract is the smallest architecture-neutral
composition of the proven `RamFlagRoutine` and the parent-owned suffix. Its
control, raw byte effects and opaque continuation are exact and resumable. The
composition is therefore a portable behavior cluster contract, but no typed
replacement is justified. The raw memory is a shared bus window with unresolved
alias and lifetime boundaries, so the cluster remains parameterized by raw
addresses and existing component contracts. No new production abstraction was
needed: M11.57 already introduced the composition without erasing either
component boundary.

This is not a portable subsystem result. Parent frame/SR restoration, the
hardware prefix, the shared epilogue and final RTS remain outside `oasis_core`.
No gameplay meaning is assigned and no `0x60BCC` investigation was expanded.

## Baseline gate

Two fresh `NATIVE_OVERRIDE` runs and two fresh `SHADOW_NATIVE` runs used the
canonical ROM and unchanged external GPGX library. All four runs retained:

| check | result |
|---|---|
| state checkpoint SHA-256 | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` |
| video sequence SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` |
| native total | `6,488,773` |
| interpreter remainder | `6,488,692` |
| TableCopy | `34` |
| RamFlag | `40` |
| ParentSuffix helper | `7` |
| fallback / divergence | `0 / 0` |
| shadow comparisons | `5/5`, zero divergence |

Accounting ownership remains explicit:
`6,488,692 + 34 + 40 + 7 = 6,488,773`. RamFlag and ParentSuffix are not
double-counted as a new cluster owner.

## Architecture-neutral composition

```text
parent-owned entry token + supplied base/register state
        |
        v
ParentSuffix: write first fixed byte (FF0012)
        |
        v
RamFlagRoutine: two flag read-modify-writes,
                three base+5 byte writes,
                one fixed byte write (FF0016),
                supplied return/continuation token
        |
        v
ParentSuffix: four remaining fixed byte writes
        |
        v
opaque parent continuation token -> parent-owned epilogue/RTS
```

* `CONTROL_DEPENDENCY`: ParentSuffix invokes the existing RamFlag executor once,
  then advances through its own ordered suffix tokens.
* `DATA_DEPENDENCY`: the base register value and all addresses are supplied by
  the adapter; RamFlag derives `base + 5`, while the suffix receives four
  fixed-address parameters. Core assigns no semantic field names.
* `CONTINUATION_DEPENDENCY`: both executors expose opaque tokens and can yield
  after each represented boundary. ParentSuffix clears its active state only
  at the handoff; the parent owns the continuation after that token.
* `ADAPTER_ONLY_DEPENDENCY`: ROM opcodes/PCs, GPGX fetch/prefetch/timing,
  hardware accesses, canonical continuation mapping and accounting remain in
  `tools/hybrid`.

The dependency-boundary test confirms that no ROM PC or GPGX type crosses into
`oasis_core`.

## Raw footprint and bounded ownership census

Widths are bytes. The table records effects, initialization/final guarantees,
address supply and provenance; it does not declare semantic fields.

| range | mode/width and ordering | initial dependency / final guarantee | address and owner | aliases, lifetime, bounded writers/readers | classification |
|---|---|---|---|---|---|
| `FF0012` | `WRITE 1`, first suffix effect | no read; byte is zero afterward | fixed address; ParentSuffix effect | `0x604F0`, `0x60BD2` writers; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF0628` | `READ_MODIFY_WRITE 1`, bit `0x10` | reads old byte; writes old value OR `0x10` | fixed adapter address; RamFlag effect | effect at `0x604C2`; address setup also appears at `0x6025A`, `0x6127A`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF06F2` | `READ_MODIFY_WRITE 1`, bit `0x10` | reads old byte; writes old value OR `0x10` | fixed adapter address; RamFlag effect | effect at `0x604C8`; address setup also appears at `0x60270`, `0x61286`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF001F..FF0021` | `WRITE 1` three times, after `FF0012` and flags | no read; each byte is zero afterward | supplied base register `FF001A` plus `5..7`; RamFlag effect | register-relative aliases and producer/consumer lifetime outside capture unresolved | `ALIASING_UNRESOLVED` + `LIFETIME_UNRESOLVED` |
| `FF0016` | `WRITE 1`, after derived writes | no read; byte is zero afterward | fixed adapter address; RamFlag effect | bounded writer `0x604DE`; no exclusivity inferred from absence elsewhere; lifetime unresolved | `LIFETIME_UNRESOLVED` |
| `FF0010` | `WRITE 1`, after RamFlag | no read; byte is zero afterward | fixed address; ParentSuffix effect | writers `0x604FA`, `0x60BDC`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF0011` | `WRITE 1`, after `FF0010` | no read; byte is zero afterward | fixed address; ParentSuffix effect | writers `0x60500`, `0x60BE2`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF0013` | `WRITE 1`, after `FF0011` | no read; byte is zero afterward | fixed address; ParentSuffix effect | writers `0x60506`, `0x60BE8`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| `FF0014` | `WRITE 1`, final suffix effect | no read; byte is zero afterward | fixed address; ParentSuffix effect | writers `0x6050C`, `0x60BEE`; lifetime not closed | `SHARED_WITH_KNOWN_EXTERNAL_CODE` |
| base `FF001A` | register input, no direct memory access | required on entry; base register is preserved | supplied parent register; parent/adapter owns it | register-relative aliases and lifetime unresolved | `ALIASING_UNRESOLVED` + `LIFETIME_UNRESOLVED` |

The bounded ROM/runtime evidence identifies exact access PCs where known. It
does not infer exclusivity from a single natural trace. The `0x60BCC` path has
a hardware prefix, but no hardware access is inside the portable composition or
its raw footprint; the prefix remains adapter-owned.

## Typed-data gate

`TYPED_DATA_CONTRACT_BLOCKED_SHARED_WRITERS_UNRESOLVED_ALIAS_LIFETIME`.
Stable offsets and widths are known for the byte effects, but the ownership
boundary is not: `FF0010..FF0014` are written by multiple bounded callers,
`FF0628/FF06F2` are addressed by other code, and the A5-derived range has no
closed alias or lifetime census. Initialization and external producer/consumer
closure are therefore incomplete. No C++ structure was added.

## Behavior-cluster gate

The raw-address cluster contract is complete: entry and output effects are
ordered, RamFlag is invoked exactly once, continuation tokens are explicit,
all represented boundaries are resumable, parent-frame and hardware effects
are absent from core, and no ROM PC enters core. Existing tests cover exact
write order, invalid token/base rejection and every represented event boundary;
the standalone RamFlag tests cover its own return and flag/output behavior.
Because typed replacement is blocked, the existing raw-parameter composition is
retained as the only abstraction. `PORTABLE_SUBSYSTEM_BOUNDARY_PROVEN` remains
false.

## Astra review

No Astra architecture reviewer is available in this workspace. The repository
contracts and prior bounded evidence are the authority for this milestone; no
external architecture change was inferred.

## Validation

* Debug MinGW CTest: `69/69`.
* Release MinGW CTest: `69/69`.
* GNU/UCRT-equivalent CTest: `69/69`.
* Fresh 600-frame shadow and native runs: exact hashes, zero fallback and zero
  divergence; shadow `5/5`.
* Core dependency-boundary, RamFlag, ParentSuffix, mechanical, continuation,
  caller-provenance and checkpoint canonicalization tests: passed through the
  full suites above.
* `git diff --check`, source line-limit and repository-hygiene checks: passed.
* Debug and Release rebuilds passed. The existing UCRT build passed its full
  CTest and line-limit checks, but a fresh relink stopped at `collect2.exe`
  exit 53 from the configured `C:\msys64\ucrt64\bin\c++.exe`; no source
  error was reported. This is recorded as a local toolchain limitation.
* `game.srm` remains unchanged and untracked; no ROM, asset, emulator binary
  or raw trace is tracked.

## Next milestone (proposed only)

M11.59 should close one dominant raw-data blocker: the alias/lifetime and
external-writer census for the `FF0010..FF0016` window and the `FF001A + 5..7`
derived range. Do not execute that work in M11.58, widen into `0x60BCC`, add a
typed structure, or claim a subsystem boundary.
