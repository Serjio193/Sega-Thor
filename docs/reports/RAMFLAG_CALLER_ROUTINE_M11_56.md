# M11.56 — Close the 0x604F0 caller contract

STATUS: **THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE**

BASELINE: `47c37ead7beb9a7e063588cd39a5cbf65deb41e8`.
Dominant blocker: **ENCLOSING_ROUTINE_BOUNDARY**.
Continuation classification: **ENCLOSING_ROUTINE_CONTINUATION**, implemented
by a shared restore/return epilogue. This is not a tail call to an independent
callee. Production promotion is stopped; two authoritative routines remain.

## Identity and evidence scope

Canonical user ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
Unmodified external GPGX DLL SHA-256:
`140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd`.
All observations use the unchanged 600-frame cold-reset neutral scenario.
Raw ROM, decoded slices, checkpoints, traces and binaries remain local.

The pre-edit M11.55 dual-native gate passed twice, including byte-identical
caller attribution against the retained M11.55 artifact. Post-edit Debug and
Release dual-native runs repeat the same contract. Both emulated provenance
runs have identical trace SHA-256:
`8b23fce6088956ecdc443bf432f183a6204eabe6fdf0aeb71dbe9e29087ea877`.
Capture reports one entry, complete=true, truncated=false. It contains the
natural parent prologue, predecessor path, every candidate instruction and
data access, the enclosing RTS post-state and following execution boundary.

## Boundary correction and exact CFG

The old range `[0x604F0,0x60520)` is a decoder budget, **not a routine boundary**.
The seven selected instructions occupy exactly `[0x604F0,0x60516)`. Decoding
from the parent discovers another branch entry at `0x60516` inside the old
budget; its instruction at `0x6051C` ends at `0x60522`, crossing that budget.
Consequently the old rectangular window is not even a complete set of owned
instructions. Classification of the selected seven-instruction path: **C, a
tail region of a larger routine**; the budget as a whole contains parts of two
different internal arms. The epilogue is shared.

The existing exact decoder, rooted at `0x60004` with budget `0x1200`, decodes
801 instructions / 109 blocks. Its relevant enclosing block is
`[0x604E6,0x60516)` (9 instructions), with this natural path:

```text
0x041E JSR.L 0x60004 (4EB9 0006 0004; return 0x0424)
  0x60004 BRA.W 0x6042A
  0x6042A MOVE.W SR,-(A7)
  0x6042C ORI.W #0x0700,SR
  0x60430 MOVEM.L D1-D7/A0-A6,-(A7)
  0x60434 LEA 0xFF001A,A5
  ... tested direct condition path ...
  0x60448 BEQ.W 0x604E6
  0x604E6 BSR.W 0x611EA (returns to 0x604EA)
  0x604EA BSET.B #0,0(A5)
  0x604F0 SF.B 0xFF0012
  0x604F6 BSR.W 0x604BC (returns to 0x604FA)
  0x604FA/0x60500/0x60506/0x6050C SF.B fixed RAM bytes
  0x60512 BRA.W 0x611D6
  0x611D6 MOVEQ #0,D0
  0x611D8 MOVEM.L (A7)+,D1-D7/A0-A6
  0x611DC MOVE.W (A7)+,SR
  0x611DE RTS -> 0x0424
```

All incoming/outgoing edges **in this decoded parent scope** are explicit:

* Selected seven-instruction region: incoming fallthrough `604EA -> 604F0`;
  outgoing call `604F6 -> 604BC`, return continuation `604E4 -> 604FA`, and
  branch `60512 -> 611D6`. No direct branch/call targets `604F0` or another
  instruction of the selected region in this parent CFG.
* Old budget additionally admits `60480 -> 60516`; the instruction at
  `6051C` continues beyond the budget to `60522`. There is no fallthrough
  from `60512` to `60516` because the BRA is unconditional.
* `611D6` has 15 direct incoming branches: `60512,60648,6074A,60880,609B8,
  609C2,60B4C,60CD6,60F18,60F50,60F88,61170,6117A,6119E,611AA`, plus
  fallthrough from `611D0` (SF/ST family instruction ends at `611D6`).
* `611D8` additionally admits `60A86 -> 611D8`, bypassing `MOVEQ #0,D0`.
  Its ordinary incoming edge is `611D6 -> 611D8`; its common exit is RTS.

The decoder reports zero unresolved control-flow instructions in this bounded
decode. This does **not** prove there are no arbitrary external/indirect entries
elsewhere in the ROM, nor does it close all out-of-range callees. No all-ROM
caller census or indirect-edge closure is claimed. The positive parent-frame
evidence already disproves the proposed ordinary standalone entry contract.

`604F0` is a real instruction boundary and natural internal execution point;
it has no independently established calling convention. A normal JSR to that
address would place its return where the epilogue expects saved D1, then pop
56 register bytes and SR before RTS. Calling it independently under the
ordinary RamFlag-like stack convention is therefore invalid.

## Dynamic parent ownership

Every listed event is in frame **115**, with D0-D7/A0-A7/PC/SR and 68 bytes
from A7 captured at each hook event. Counters are GPGX master cycles/refresh.

| execute PC | predecessor | A7 | SR | cycles / refresh |
|---|---|---|---|---|
| `60004` | `041E` | `FF0BEA` | `2114` | 185840 / 185896 |
| `6042A` | `60004` | `FF0BEA` | `2114` | 185910 / 185896 |
| `60430` | `6042C` | `FF0BE8` | `2714` | 186162 / 186806 |
| `60434` | `60430` | `FF0BB0` | `2714` | 187002 / 187702 |
| `604F0` | `604EA` | `FF0BB0` | `2718` | 189333 / 189599 |
| `604F6` | `604F0` | `FF0BB0` | `2718` | 189473 / 189599 |
| `604BC` | `604F6` | `FF0BAC` | `2718` | 189599 / 189599 |
| `604FA` | `604E4` | `FF0BB0` | `2718` | 190635 / 191405 |
| `60512` | `6050C` | `FF0BB0` | `2718` | 191195 / 191405 |
| `611D6` | `60512` | `FF0BB0` | `2718` | 191265 / 191405 |
| `611D8` | `611D6` | `FF0BB0` | `2714` | 191293 / 191405 |
| `611DC` | `611D8` | `FF0BE8` | `2714` | 192161 / 192301 |
| `611DE` | `611DC` | `FF0BEA` | `2114` | 192273 / 192301 |
| `0424` | `611DE` | `FF0BEE` | `2114` | 192385 / 192301 |

At `604F0`, the stack longword is `7FF0FFFF`, the saved D1 value, **not a
return address**. Let S=`FF0BB0`. `[S,S+56)` holds D1-D7/A0-A6 in that
order; `[S+56,S+58)` is saved SR=`2114`; `[S+58,S+62)` is return=`00000424`.
These values match the parent-entry registers independently in the validator.
The prologue writes the 14 registers in reverse order, low word then high
word per predecrement longword. The restore reads them forward.

At candidate entry D0-D7 are `00000000,7FF0FFFF,00000000,00000000,940F0A8C,
60000083,00000000,00000040`; A0-A6 are `00C00011,00FF0BFC,00000000,
00FF13CC,00C00004,00FF001A,00000000`. RamFlag leaves A0=`FF0022`,
A6=`FF06F2`; D/A other than A0/A6/A7 are preserved across the call. The
enclosing epilogue restores original D1-D7/A0-A6 (including D2=`0000FFFF`),
sets D0=0, restores full SR=`2114`, and returns with A7=`FF0BEE`.

## Instruction semantic and continuation ledger

`51F9` is **SF**, condition false unconditionally; its meaning is not an
unknown runtime condition. It writes zero regardless of CCR and preserves
all flags. This refines the generic `Scc` wording of M11.55.
The following classifications concern the exact represented form, supported
by ROM encodings, existing RamFlag tests, pinned GPGX instruction handlers
and the ordered natural trace. They do not establish a portable parent API.
Base cycles are 68000 cycles; observed deltas are master cycles including
the pinned refresh behavior. No lump-sum replacement was implemented.

| PC(s) | exact form / effects | base cycles | observed master delta | classification |
|---|---|---:|---:|---|
| `604F0,604FA,60500,60506,6050C` | `51F9 abs.l`: byte zero; no register/CCR/X/stack effect | 20 each | 140 each | SEMANTICS_PROVEN |
| `604F6` | `6100 FFC4`: BSR.W; A7-=4, write return `604FA`; flags preserved | 18 | 126 | SEMANTICS_PROVEN |
| `604BC` | `4DF9 00FF 0628`: A6=`FF0628`; flags preserved | 12 | 98 | SEMANTICS_PROVEN |
| `604C2` | `08EE 0004 0000`: BSET.B #4,(0,A6); read/OR/write; only Z from old bit | 20 | 140 | SEMANTICS_PROVEN |
| `604C8` | `4DF9 00FF 06F2`: A6=`FF06F2`; flags preserved | 12 | 84 | SEMANTICS_PROVEN |
| `604CE` | `08EE 0004 0000`: same bit operation at second flag | 20 | 140 | SEMANTICS_PROVEN |
| `604D4` | `41ED 0005`: A0=A5+5; flags preserved | 8 | 56 | SEMANTICS_PROVEN |
| `604D8,604DA,604DC` | `51D8`: byte zero at A0, then A0+=1; flags preserved | 12 each | 84 each | SEMANTICS_PROVEN |
| `604DE` | `51F9 00FF 0016`: byte zero; flags preserved | 20 | 140 | SEMANTICS_PROVEN |
| `604E4` | `4E75`: read return long, A7+=4; PC=`604FA`; flags preserved | 16 | 126 | SEMANTICS_PROVEN |
| `60512` | `6000 0CC2`: BRA.W to `611D6`; no stack/flags effect | 10 | 70 | SEMANTICS_PROVEN |
| `611D6` | `7000`: D0=0; Z=1,N/V/C=0,X preserved | 4 | 28 | SEMANTICS_PROVEN |
| `611D8` | `4CDF 7FFE`: 14 ordered long reads, A7+=56, extra word read, CCR/X preserved | 12+14*8 | 868 | SEMANTICS_PROVEN |
| `611DC` | `46DF`: pop full SR, A7+=2 before SR application; supervisor/interrupt/stack-bank effects need portable adapter contract | 16 | 112 | SEMANTICS_PARTIAL |
| `611DE` | `4E75`: read original return, A7+=4, PC=`424`; flags preserved | 16 | 112 | SEMANTICS_PROVEN |

Full SR restore is observed exactly for the natural S=1 -> S=1 case and
interrupt mask 7 -> 1. A general entry could cause stack-bank switching,
privilege exception, trace or interrupt service. No portable implementation
or independent continuation tests close those alternatives here. Any required
partial form keeps production promotion stopped even apart from the dominant
enclosing-boundary blocker. No `OTHER` semantic bucket is used.

## Ordered memory and aliasing

All **32 top-level data-hook accesses from `604F0` through enclosing RTS**
are checked in order, with exact width and value. Instruction fetch/prefetch
bus accesses are outside this count. Canonical bytes and execute/post snapshots
are recorded; internal prefetch latches are not independently compared by this
observer. This is GPGX evidence, not a physical bus capture.

1. W1 `FF0012=0`; BSR W4 `FF0BAC=000604FA` (predecrement A7 by 4).
2. R1/W1 `FF0628=10/10`; R1/W1 `FF06F2=10/10` (both bit 4 already set).
3. W1 `FF001F=0`, `FF0020=0`, `FF0021=0`, from A5=`FF001A` +5/+6/+7,
   with A0 postincrement after each write; W1 `FF0016=0`.
4. RTS R4 `FF0BAC=000604FA`, then A7+=4.
5. W1 `FF0010=0`, `FF0011=0`, `FF0013=0`, `FF0014=0`.
6. Fourteen R4 at `FF0BB0+4*i`, i=0..13, restoring D1-D7/A0-A6.
7. **Extra MOVEM R2 `FF0BE8=2114`**, followed by the separate SR instruction's
   R2 at that same address; SR pop advances A7 by 2.
8. Enclosing RTS R4 `FF0BEA=00000424`, then A7+=4.

The extra MOVEM read is present in the pinned `m68k_op_movem_32_er_pi`
handler and natural hook journal. Omitting it is rejected by a negative
control. All these addresses are main RAM. Code bytes are in canonical ROM.
The flag/output spans, nested return slot and saved-frame spans are disjoint
in the observed contract; the two reads of saved SR intentionally alias.
A5 is established by the parent's `LEA` at `60434`, not a free-standing
candidate argument. No general arbitrary-A5/stack safety proof is claimed.

The **parent prefix**, outside the selected portable candidate, naturally
executes `604E6 -> 611EA`: its bounded callees touch `A11100` (control read/
writes), `A00003` (byte write), and `C00011` (four writes). These are
**HARDWARE_BLOCKED** if absorbed into a proposed whole-parent portable routine.
This follows the `604F0` provenance only; the `60BCC` path was not expanded or
changed. Merely extending backward to `604E6` therefore does not produce a
hardware-free standalone contract.

## RamFlag composition and events

Exact call bytes are `6100 FFC4`; target=`604F6+2-60=604BC`, return=`604FA`.
All D/A/SR values at the BSR site map unchanged to callee entry except A7
decreasing from `FF0BB0` to `FF0BAC`. The pushed value is checked against ROM
and journal. The ten RamFlag instructions reuse the established contract:
A0/A6 are outputs; D0-D7/A1-A5 survive; N/V/C/X and upper SR survive; Z comes
from the second BSET. The subsequent SFs consume no flags. MOVEQ changes
CCR and then the enclosing saved SR supersedes it. No hidden flag predicate
or shared typed object is required or inferred.

The selected natural call has no interruption between entry and enclosing
return. The frozen dual-native scenario still has one RamFlag yield/resume
across all four calls; that is not proof of a new caller-to-callee core API,
nor proof of yielding while restoring the enclosing SR. Existing opaque
RamFlag tokens remain the only core interface. No ROM-PC composition or
duplicate RamFlag implementation was introduced into core.

## Decision gates and architecture consequence

* Entry/exit **proven as an internal parent-frame tail**, not an independent
  routine. Parent restore/return semantics close the previously unknown
  `611D6` structural ownership for this natural path.
* Exact bounded direct CFG and natural memory sequence are proven; global
  external/indirect entry exclusion and a general standalone memory contract
  are not. Full-SR/event continuation remains partial.
* Appending the four-instruction epilogue closes the observed execution exit,
  but requires an existing 58-byte parent save frame. Appending the enclosing
  entry and prefix introduces privileged SR operations and actual hardware.
* Phase 8 result: **THIRD_ROUTINE_NOT_A_STANDALONE_ROUTINE**. Phases 10-14
  (core extraction, third adapter, third-routine vectors/shadow/authoritative
  proof) do not run because the gate is closed. Optional Astra pre-production
  review is not invoked: no complete production contract exists to review.

Inventory remains 2 authoritative routines, 4 portable mechanical primitives,
2 complete routine contracts, 5 partial routine/caller-region contracts,
0 typed structures, 1 portable routine cluster, 0 portable subsystem boundaries.
The two historically continuation-blocked caller regions remain unpromoted;
for this one, the reason is now specifically parent-frame ownership rather
than an unidentified destination. No new caller pair or subsystem status is
promoted. Production understanding is below the 90% implementation threshold;
confidence in the bounded negative structural result is high.

## Accounting and validation

Both unchanged pre-edit and post-edit dual-native runs require exactly:

| category | instructions |
|---|---:|
| INTERPRETER | 6,488,699 |
| MECHANICAL_PRIMITIVE | 0 |
| GENERATED_TRANSLATED | 0 |
| NATIVE_TABLE_COPY | 34 |
| NATIVE_RAMFLAG | 40 |
| NATIVE_THIRD_ROUTINE | 0 |
| **total** | **6,488,773** |

The emulated provenance runs are 6,488,773 interpreter / zero native. Their
21 candidate-path instructions are a subset, never an extra category added
to the totals. Frozen checkpoint aggregate:
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`;
video aggregate:
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
Dual-native fallback/divergence are zero; attribution remains one `604F6`,
three `60BCC`, zero unknown. Runner `full_cpu_equivalence=false` remains
conservative: it does not load the paired reference. The paired canonical
checkpoint comparison is the evidence; no new exhaustive per-instruction
CPU/VDP/sound equivalence claim is inferred from that field.

Debug, Release and GNU-equivalent GCC/UCRT full builds and CTest passed
**68/68 each**. They include TableCopy/RamFlag, mechanical primitives,
continuation/dispatch adapters, caller attribution, checkpoint canonicalization
and the new observer. Source line-limit and diff checks passed. Native Linux
was not run locally; GCC/UCRT is the available GNU link check, not an assertion
of Ubuntu CI readiness. Validation results are also recorded in WORKLOG.
The observer regression covers
nested versus enclosing return, capture limits, missing parent ownership and
exclusion of the unrelated hardware path. The ROM-backed validator checks
the independent saved-register layout, exact return/A7/SR, natural call path,
32-access journal and repeated identity. Deliberately wrong final A7, omitted
MOVEM extra read and incomplete capture are all rejected.

## Reproduction

Existing local build configurations were rebuilt, not reinstalled:
`build-m1155-debug-gpgx`, `build-m1155-release-gpgx`, `build-m1155-gnu-msys`.
The latter uses GCC/UCRT from `C:/msys64/ucrt64/bin`; Debug/Release use
`C:/Dev/SegaThorTools/mingw64/bin`. Put the relevant compiler bin on PATH.

```powershell
$rom = 'build/reference/Beyond Oasis (USA).bin'
$lib = 'C:/Github/Genesis-Plus-GX-instrumented/genesis_plus_gx_libretro.dll'
$poc = './build-m1155-debug-gpgx/src/tools/hybrid/oasis_hybrid_poc.exe'
& $poc $lib $rom NATIVE_OVERRIDE 600 build-m1156-baseline-a '0x2D66,0x604BC'
& $poc $lib $rom NATIVE_OVERRIDE 600 build-m1156-baseline-b '0x2D66,0x604BC'
$env:OASIS_CALLER_CONTINUATION = '1'
& $poc $lib $rom EMULATED 600 build-m1156-trace-a
& $poc $lib $rom EMULATED 600 build-m1156-trace-b
Remove-Item Env:OASIS_CALLER_CONTINUATION
python src/tools/hybrid/validate_caller_continuation.py build-m1156-trace-a build-m1156-trace-b $rom
& ./build-m1155-debug-gpgx/oasis_re_slice.exe $rom build-m1156-parent.json build-m1156-parent.txt 0x60004 0x1200
& ./build-m1155-debug-gpgx/oasis_re_slice.exe $rom build-m1156-tail.json build-m1156-tail.txt 0x611D6 0x44
```

## M11.57 proposal — not executed

Close **ENCLOSING_ROUTINE_BOUNDARY** by defining and testing an explicit
parent-owned save/restore and suffix handoff contract for this one natural
`60004 -> 604F0 -> 611D6 -> 424` path. Determine whether a small portable
internal helper can own only the five SF writes plus the existing RamFlag
call while a developer-only parent adapter owns the save frame, full SR,
interrupt/stack-bank transition and hardware prefix. Independently test
entry/exit and adverse event boundaries before any implementation. Preserve
the distinction between an internal helper and a third complete routine.
If that division cannot be justified without absorbing other parent arms or
hardware semantics, record the exact boundary blocker and stop. Do not
switch to `60BCC`, introduce typed structures or widen natural coverage.
