# M11.55 — RamFlag caller and shared-data contract closure

STATUS: RAMFLAG_CALLER_CONTRACTS_PROVEN

BASELINE: `160422e8a280890a01f6e314a57f7e024b644ba0`

ROM identity: user-supplied `Beyond Oasis (USA).md`, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The external developer-only GPGX library SHA-256 was
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.

## Result

M11.55 closes dynamic attribution for every natural entry to `RamFlagRoutine`
in the frozen 600-frame cold-reset scenario: one direct call from `0x0604F6`
and three direct calls from `0x060BCC`, with zero unknown entries. The two
bounded caller regions have exact direct control-flow evidence and a tested
attribution schema. This proves caller-region contracts, not complete enclosing
functions or a portable subsystem.

The strongest safe labels are:

* `RAMFLAG_CALLER_CONTRACTS_PROVEN` — dynamic caller/return/A7/frame/cycle/
  register provenance is deterministic and exact for all four entries;
* `ROUTINE_CONTRACT_PARTIAL` — `0x0604F0` and `0x060BC4` are bounded caller
  regions, not closed routines;
* `SHARED_DATA_TYPED_GATE_BLOCKED` — byte provenance is exact for offsets
  `0,1,2,3,4,6`, but the A5-relative writes, lifetime, aliasing and type are
  not closed;
* `HARDWARE_PREFIX_PORTABLE_SUFFIX` for the audited `0x060BC4` → `0x060BCC`
  handoff, while the whole `0x060BC4..0x060CDA` caller region remains
  `HARDWARE_BOUNDARY_NOT_CLOSED` because sibling calls and memory effects are
  not semantically closed.

No conditional implementation gate opened. No `oasis_core` or production
runtime source changed. The attribution observer, bridge refactor and its test
remain in `tools/hybrid`, which is developer-only.

## Baseline and STOP gates

The unchanged dual-native run was reproduced twice after instrumentation. Both
runs reported `NATIVE_OVERRIDE`, `natural_calls=5`, `override_calls=5`,
`divergences=0`, and `fallback_emulated_calls=0`:

| identity | run E | run F |
|---|---:|---:|
| state checkpoint SHA-256 | `251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d` | same |
| video sequence SHA-256 | `5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58` | same |
| total guest instructions | 6,488,773 | 6,488,773 |
| interpreter remainder | 6,488,699 | 6,488,699 |
| TableCopy native instructions | 34 | 34 |
| RamFlag native instructions | 40 | 40 |
| RamFlag invocations | 4 | 4 |
| RamFlag boundary yields/resumptions | 1 / 1 | 1 / 1 |

The attribution JSON files were byte-identical. Since no production change was
made, this unchanged native proof is the post-instrumentation regression gate.

## Dynamic attribution

`caller_attribution.json` uses schema
`oasis.hybrid.caller-attribution.v1`. At each target entry the observer records
the invocation ordinal, classification, bounded caller PC, direct call-site PC,
stack A7, stack return PC, frame, cycle and refresh-cycle counters, all D/A/SR
register values, and the exact evidence source.

| ordinal | class | bounded caller PC | call site | return PC | A7 | frame | cycles / refresh |
|---:|---|---|---|---|---|---:|---:|
| 1 | `0x604F6` | `0x000604F0` | `0x000604F6` | `0x000604FA` | `0x00FF0BAC` | 115 | 189599 / 189599 |
| 2 | `0x60BCC` | `0x00060BC4` | `0x00060BCC` | `0x00060BD0` | `0x00FF0BA4` | 425 | 343846 / 344273 |
| 3 | `0x60BCC` | `0x00060BC4` | `0x00060BCC` | `0x00060BD0` | `0x00FF0BA4` | 425 | 669801 / 670557 |
| 4 | `0x60BCC` | `0x00060BC4` | `0x00060BCC` | `0x00060BD0` | `0x00FF0BA4` | 426 | 100661 / 100864 |

All four records classify as `NATURAL_HOOK_PREVIOUS_EXECUTE_AND_STACK_RETURN`;
`unknown_count=0`. The direct BSR.W opcode/extension and the stacked return
address are independently checked. `OTHER_PROVEN_CALLER` and
`UNKNOWN_CALLER` remain fail-closed classifications for future runs.

The earlier `0x0604EC` caller-region label is corrected here: ROM bytes at
`0x0604EC..0x0604EF` are `0000 0000` data, while code begins at `0x0604F0`.

## Exact bounded CFG and semantics

### Caller `0x0604F0` / call site `0x0604F6`

The exact slice `[0x0604F0, 0x060520)` contains 7 instructions, 1 basic block,
1 direct call, 1 direct branch, zero unresolved control-flow edges and zero
unsupported control-flow instructions. The proven edges are:

* `0x0604F6 -> 0x0604BC` (`BSR.W`, return `0x0604FA`);
* `0x060512 -> 0x0611D6` (direct branch).

Dynamic address provenance records writes to `0x00FF0012` at `0x0604F0`,
`0x00FF0010` at `0x0604FA`, `0x00FF0011` at `0x060500`,
`0x00FF0013` at `0x060506`, and `0x00FF0014` at `0x06050C`, each once in the
scenario. The call-site and return are closed, but the region exits to
`0x0611D6` without a proven owner/continuation contract. It is therefore not
promoted as a routine or subsystem.

### Caller `0x060BC4` / call site `0x060BCC`

The exact slice `[0x060B50, 0x060E50)` contains 111 instructions, 9 basic
blocks, 6 direct branches and 11 direct calls, with zero unresolved or
unsupported control-flow edges. The call block is `0x060BC4..0x060CDA` and
contains 84 instructions. Proven relevant edges include:

* `0x060BCC -> 0x0604BC` (`BSR.W`, return `0x060BD0`);
* sibling calls `0x060C18`, `0x060C2E`, `0x060C44 -> 0x060F8C`;
* sibling calls `0x060C5A`, `0x060C70`, `0x060C86`, `0x060CA4`,
  `0x060CBA -> 0x061032`;
* exits `0x060CD2 -> 0x0610C8` and `0x060CD6 -> 0x0611D6`.

The slice has 25 unresolved memory references even though its control-flow
edges are bounded. Register-relative/table accesses and sibling callees are
not semantically closed. This is a proven caller region, not a portable
routine contract.

### Per-instruction closure ledger

The following instruction-level effects are closed only to the stated bus or
control-flow effect; they are not claims about the enclosing function:

| PC | decoded effect | closure |
|---|---|---|
| `0x0604F0` | `Scc.B ($00FF0012).L` | fixed byte write; condition meaning unknown |
| `0x0604F6` | `BSR.W 0x0604BC` | direct target and return PC proven |
| `0x0604FA` | `Scc.B ($00FF0010).L` | fixed byte write; condition meaning unknown |
| `0x060500` | `Scc.B ($00FF0011).L` | fixed byte write; condition meaning unknown |
| `0x060506` | `Scc.B ($00FF0013).L` | fixed byte write; condition meaning unknown |
| `0x06050C` | `Scc.B ($00FF0014).L` | fixed byte write; condition meaning unknown |
| `0x060512` | `BRA.W 0x0611D6` | direct exit proven; owner/continuation unknown |
| `0x060BC4` | `MOVE.W #$0000,($00A11100).L` | exact hardware write; adapter-owned |
| `0x060BCC` | `BSR.W 0x0604BC` | direct target and return PC proven |
| `0x060BD0` | `MOVE.L (A7)+,A0` | stack read/pop and A7 effect proven |
| `0x060BD2` | `Scc.B ($00FF0012).L` | fixed byte write; condition meaning unknown |
| `0x060BD8` | `Scc.B (d16,A5)` | runtime target `FF001A`; base/alias unknown |
| `0x060BDC` | `Scc.B ($00FF0010).L` | fixed byte write; condition meaning unknown |
| `0x060BE2` | `Scc.B ($00FF0011).L` | fixed byte write; condition meaning unknown |
| `0x060BE8` | `Scc.B ($00FF0013).L` | fixed byte write; condition meaning unknown |
| `0x060BEE` | `Scc.B ($00FF0014).L` | fixed byte write; condition meaning unknown |

The remaining decoded instructions in `0x060BC4..0x060CDA` include direct
sibling calls and register-relative/table operations. Their control-flow
targets are exact, but their memory effects are not all closed; this is the
reason the ledger does not become a typed caller or subsystem contract.

## Shared-memory matrix

The matrix is an address-provenance result, not a type declaration.

| byte offset | proven access PCs | observed mode/width | closure |
|---:|---|---|---|
| `0` | `0x0604FA`, `0x060BDC` | `W1` | exact byte access, meaning unknown |
| `1` | `0x060500`, `0x060BE2` | `W1` | exact byte access, meaning unknown |
| `2` | `0x0604F0`, `0x060BD2` | `W1` | exact byte access, meaning unknown |
| `3` | `0x060506`, `0x060BE8` | `W1` | exact byte access, meaning unknown |
| `4` | `0x06050C`, `0x060BEE` | `W1` | exact byte access, meaning unknown |
| `5` | no fixed absolute access proven | — | unknown; do not fill |
| `6` | `0x0604DE` | `W1` | exact byte access, meaning unknown |

The stable exact subset is therefore `FF0010`, `FF0011`, `FF0012`, `FF0013`,
`FF0014`, and `FF0016`; offset `FF0015` is not proven. The `0x0604DE` write
is internal to RamFlag. The `0x060BD8` access is to `0x00FF001A`, outside the
requested window, and prevents treating the caller as a six-byte isolated
record. A5-relative writes at the RamFlag tail remain dynamically observed but
not fixed-address typed fields.

Typed-structure gate: BLOCKED. No field type, owner, lifetime, aliasing,
initialization protocol, or complete producer/consumer set is proven.

## Hardware boundary

Raw ROM decoding proves `0x060BC4` is `MOVE.W #$0000,$00A11100`, a Z80-control
register write immediately before `BSR.W 0x0604BC` at `0x060BCC`. Runtime
address provenance observed `0x00A11100` only at `0x060BC4` for all three
attributed `0x060BCC` entries; the RamFlag call and its proven fixed-RAM writes
follow it. This supports an adapter-owned hardware prefix and portable
RamFlag/data suffix for that exact handoff. It does not close the whole caller
region: the sibling calls and 25 unresolved memory references keep the full
hardware boundary blocked.

## Promotion and subsystem gates

The `0x0604F6` gate is not open: its call/data effects are bounded, but its
continuation at `0x0611D6` and enclosing ownership are not closed. The
`0x060BCC` gate is also not open: its direct call and hardware ordering are
proven, but sibling calls, register-relative memory and continuation are not.

The two callers do not establish a shared typed structure or a single portable
owner. `PORTABLE_ROUTINE_CLUSTERS` remains 1 (the existing RamFlag-centered
cluster); `PORTABLE_SUBSYSTEM_BOUNDARIES` remains 0. No subsystem implementation
is justified by M11.55.

## Inventory and implementation decision

| metric | M11.55 result |
|---|---:|
| authoritative native routines | 2 |
| portable mechanical primitives | 4 |
| complete routine contracts | 2 |
| partial routine/caller-region contracts | 5 (3 existing + 2 bounded caller regions) |
| portable typed structures | 0 |
| portable routine clusters | 1 |
| portable subsystem boundaries | 0 |
| hardware-blocked routines | 1 |
| continuation-blocked routines | 2 |

Conditional implementation A (portable caller) and B (exact typed shared data)
are both closed by STOP. No production implementation, third routine, hardware
model or subsystem abstraction was attempted.

## Validation

Fresh configurations from this change passed:

* Debug GPGX-enabled MinGW: full CTest `67/67`;
* Release GPGX-enabled MinGW: full CTest `67/67`;
* GNU-equivalent UCRT/MinGW configuration: full CTest `67/67`;
* `oasis_hybrid_caller_attribution` synthetic deterministic JSON test;
* `git diff --check` and the source-code file-limit gate.

No ROM, asset, binary, save-state or run-evidence file is tracked. Existing
untracked `game.srm` and historical evidence files remain untouched.

## Proposed next milestone (not executed)

M11.56 should choose one bounded falsifiable closure only: independently close
the `0x0604F0`/`0x0611D6` continuation ownership or close the
`0x060BC4..0x060CDA` sibling-call and A5-relative data contract. It must retain
the same dual-native identity gate and fail closed on unresolved memory,
hardware, indirect-flow or continuation evidence. Do not implement M11.56 in
this commit.
