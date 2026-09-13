# THOR M12 — Autonomous ASM Reconstruction After Evidence Engine Baseline

## AUTO63 — focused novelty investigation (2026-09-13)

AUTO63 consumed the strongest persisted AUTO62 novelty candidate, `PC=0xAF22`,
and generated a register-specific request instead of repeating the broad
capture. Static evidence identified `0xAF06 MOVEA.L 0x1A(A6),A0` and the
`0xAF20 MOVE.W (A0),D6` consumer. One exact BizHawk 2.11.1 focused capture
observed the definition transition and 56 consumer reads, but it found
`D3=512`, no direct RAM-source callback, and two `+8` gaps in the 18 unique
addresses. The causal edge and exact six-byte enumeration therefore remain
unproven; promotion was correctly blocked and SOURCE_OWNED stayed
`1,475,600 / 3,145,728 = 46.9080607096%`.

The machine-readable request, capture and fail-closed result are under the
ignored `build/thor-evidence/auto63-followup/auto63-af22-a/` directory. Full
details are in `docs/reports/THOR_M12_AUTO63_INVESTIGATION.md`. The next
question is upstream A6 provenance plus the explanation for the observed
`D3=512`/`+8` gaps; no typed data claim is made.

## Campaign gate

- Baseline SHA: `4ab17d4a7854dab20661c03706d11f1eddf470d8`
- Final SHA: publication commit (exact SHA recorded in the final gate response)
- Campaign: `M12-AUTO61`
- Scope: canonical Beyond Oasis ROM reconstruction only; no C++/M13 work, no
  ROM or extracted asset committed
- Stop target: continue toward 90% ownership, but stop this campaign at the
  first fixed-point or verification blocker
- Current ownership: `1,475,600 / 3,145,728 = 46.9080607096%`

## Evidence and selected method

The next ranked frontier was the selector child-table region adjacent to the
already documented descriptor table. The prior `0x03BDA6`/`0x03BDD8` frontier
was not repeated: static and runtime evidence had already failed to close its
unresolved extent. Generic Carver/format discovery was also at its documented
fixed point. The selected method was the existing selector/descriptor grammar,
which independently establishes a finite selector domain (`0..6`), a common
8-byte record width, and one `0xFFFF` sentinel terminating each child stream.

The complete physical table is `0x03B95C..0x03BA46` (234 bytes). Its first two
bytes, `0x03B95C..0x03B95E`, alias an already-owned descriptor record. The safe
new ownership interval is therefore exactly:

`0x03B95E..0x03BA46` — 232 bytes, `STRUCTURED_DATA_CONFIRMED`.

No code conflict was found. The following interval remains UNKNOWN:

`0x03BA46..0x03BD86`.

## Fixed-point and verification record

- Contract parser: selectors `0..6`, all seven sentinel boundaries, record
  width 8, and physical extent validated.
- Manifest transaction: no gaps, overlaps, or conflicting ownership; only the
  UNKNOWN interval containing the safe promotion was split.
- Canonical materialization: exact size 3,145,728; CRC32 `C4728225`; SHA256
  `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- GitHub CI run `34750447524` for publication commit
  `e1e27e084d7beb600fdb47c2df0c0fb3462b4788` passed Build and Test.
- Exact assembler invocation was attempted. Full-layout assembly failed on
  inherited duplicate labels `loc_00B856`, `loc_00B912`, `loc_00E2D4`, and
  `loc_00E7FC`. This is recorded as `BLOCKED_INHERITED_FULL_LAYOUT`; it is
  not evidence against the child-table contract and is not reported as an ASM
  round-trip pass.

## Campaign delta

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| SOURCE_OWNED_BYTES | 1,475,368 | 1,475,600 | +232 |
| SOURCE_OWNED_PERCENT | 46.9006856283% | 46.9080607096% | +0.0073750813 pp |

## Next bounded direction

Do not promote `0x03BA46..0x03BD86` from this grammar alone. The next campaign
must obtain an independent evidence class for that successor region or remain
fail-closed. The unresolved `0x03BDD8` path and the inherited full-layout
duplicate-label blocker remain explicit blockers. The 90% review checkpoint
has not been reached.
