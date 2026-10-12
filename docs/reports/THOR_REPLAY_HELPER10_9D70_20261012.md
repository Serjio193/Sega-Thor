# M68K helper `0x9D70` — path-specific native verification

## Result

`0x9D70` is verified for the observed direct caller `0x009144` only.

- `CPP_REPLACEMENT_VERIFIED_PATH_SPECIFIC = YES`
- `PATH_SCOPE = 0x009144 BSR.W -> 0x009148`
- `CPP_REPLACEMENT_VERIFIED_GLOBAL = NO`
- `RUNTIME_REPLACEMENT_ACTIVE = NO`

The verified cohort now contains ten path-specific helpers. This evidence does
not enable or activate a runtime route.

## Exact body and access contract

Canonical ROM SHA-256:
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

The exact decoded body is `[0x009D70,0x009D92)`, 34 bytes and 11
instructions. Its exits are RTS at `0x009D86` and `0x009D90`. A full aligned
ROM scan plus decode confirms direct BSR.W callers `0x009144` and `0x00979E`.
The selected caller has 22 full natural captures; the other has two and remains
outside this verification.

The routine reads D1.low and word `[A6+0x46]`, calculates a signed 16-bit
difference into D3.low, and clamps D0.low against that difference. A negative
difference clears D0.low and ORs `0x48` into byte `[A6+0x40]`. If the
nonnegative clamp produces zero, it performs the same byte OR. Otherwise it
returns without a RAM write. D0/D3 upper words are preserved. Other D/A
registers are preserved; CCR follows the executed 68000 word/byte operations,
including X. The only stack effect is RTS consuming the normal four-byte return
address. There are no tables, variable indexes, loops, nested calls, or
indirect transfers. The maximum routine-owned RAM span is the fixed A6-relative
field at `+0x40`; `+0x46` is read as a word.

## Replay evidence

The input corpus is
`build/gameplay-session-helper6-census-24-20261011/replay-run/auto-full-calls.jsonl`
with SHA-256
`c598c4f2ad00567f6c726bd1b26bc28658129a288241711cb5b9d8adb178c27d`.
It contains 24 calls: 22 from `0x009144` and two from `0x00979E`. All 22
selected calls follow the same path and return at `0x009D86`; D1.low and the
D3.low result each vary across all 22 calls. The captured word read is exactly
at A6+`0x46`. Native output D0, D3, SR, preserved registers, return PC, and
stack advance match all 22 selected calls. No non-stack write occurs on these
natural paths. Full input/output vectors and source hashes are in the adjacent
machine receipt.

Natural replay covers one branch shape. The canonical-ROM oracle separately
executes four ABI-local cases covering the keep, nonzero clamp, zero clamp/write,
and negative-difference/write paths. All four match the typed C++ model for all
D/A registers, SR, the conditional byte write and RTS continuation. The
validated RTS micro-oracle passes first.

## Caller-visible effects

At `0x009148`, the selected caller immediately invokes `0x009EBA`. That helper
first executes `BTST.B #2,56(A6)`, replacing NZVC before its conditional branch.
On its return path, the caller reads D0 at `0x009158`; D0 is therefore live and
is reproduced exactly. D3 is overwritten at `0x00914C` on the returning path;
on the fall-through path inside `0x009EBA`, it is overwritten at `0x009ED0`
before its subsequent use. CCR is overwritten before the caller's `BLE` at
`0x00915A`, and X is overwritten by `SUB.W` at `0x009150` before any condition
depends on it. The native model still reproduces full CCR and D3 state.

This closes only the `0x009144` path. The `0x00979E` caller, other possible
indirect entries, and cycle-level timing are not covered. No production or
developer live dispatch was added.

## Validation and authority

- Debug C++ replay-vector test: 22/22
- Debug canonical-ROM oracle: 4/4
- Release C++ replay-vector test: 22/22
- Release canonical-ROM oracle: 4/4
- Changed source files: 27, 60, 257 and 79 lines; each is below the 500-line limit.
- The dirty checkout has three unrelated modified files over 500 lines; the isolated commit snapshot passes the project source-size check for all 743 governed files.
- New bytes classified: 0; evidence authority deltas: 0.
- Totals remain ROM 3,145,728 B; runtime 440,425 B; structural 1,266,667 B;
  SOURCE_OWNED 1,487,672 B.

See `THOR_REPLAY_HELPER10_9D70_20261012.json` for the machine receipt.
