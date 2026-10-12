# Helper #10 candidate scout — M68K `0x99D6` (2026-10-12)

## Result

`0x99D6` is frozen as a compact, deterministic byte lookup whose pointer extent is unresolved. No native model was implemented. The address span observed in replay is not treated as a payload boundary.

## Exact boundary and caller census

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`. Exact decoded range `[0x0099D6,0x0099F0)`, 26 bytes, 10 instructions, single RTS at `0x0099EE`. No nested calls, indirect control, or stores. The ROM-wide aligned BSR.W census finds `0x009920` and `0x009932`; both are decoded callsites. The deterministic replay corpus has 24 full calls from `0x009932`, returning to `0x009936`; only that path is analyzed. Indirect entries remain open.

## Read and pointer arithmetic

The helper reads the long pointer `[A0+0]`, shift word `[A0+8]`, and exactly one byte at `A1 + sign_extend_16(D4.w)`. Its effective index is the low word of `((int16(D2.w) ASR 3) << (shift & 63)) + (int16(D1.w) ASR 3)`. Across 24 captures, the formula reproduces all 24 observed cell addresses: base `0x00FF5FAC`, shift 6, indices `0x0550..0x0814`, addresses `0x00FF64FC..0x00FF67C0`, and observed cell byte `0x01`. Each call performs one indexed byte read plus the two descriptor reads and RTS stack read. There is no sentinel, count, clamp, or bounds predicate.

The caller at `0x009932` receives D3 and immediately masks it with `ANDI.B #0x90`, then compares and branches. That proves D3 is live and the resulting NZVC drives the branch. It does not prove a coordinate range or allocation size. A0 points at descriptor `0x00FF1766` in this corpus, but the loaded backing pointer is `0x00FF5FAC`; this capture does not link that RAM allocation to an exact decoded screen-root window or its extent.

## ABI and disposition

Inputs are D1.w, D2.w and the A0 descriptor fields. Outputs include D3.b, D4.w and A1. A0 is preserved; RTS restores A7. No stores occur. D3 liveness is exact; A1/D4 liveness after the caller's conditional continuation remains unresolved. More importantly, the routine accepts a computed signed word offset without validating it, and neither the chosen call edge nor these captures establish the full valid input domain.

Disposition: `POINTER_EXTENT_UNRESOLVED`; no native model or differential test. Do not infer a 64x64 extent from the shift value, observed addresses, or a different setup's grid contract. Freeze this candidate and rerank. Next candidate: `0xF788` (24 natural calls, two paths, one replay-observed caller `0x008B2A`; boundary still unknown).

No ROM bytes classified; authority deltas 0. Totals remain ROM 3,145,728 B; runtime 440,425 B; structural 1,266,667 B; SOURCE_OWNED 1,487,672 B. Machine receipt: `THOR_REPLAY_HELPER10_99D6_BOUNDARY_20261012.json`.
