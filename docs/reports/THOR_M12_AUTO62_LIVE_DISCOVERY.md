# THOR M12 — AUTO62 full-layout repair and live discovery

## Gate and scope

- Baseline: `a5e6dacd7b4c8c40a8be5fb769848057e2cf071a`
- Campaign: `M12-AUTO62`
- Canonical ROM SHA256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- Exact QuickSave1 SHA256: `7fde47833ce70a1df34e75d95c84ed87afc8470d228af87967c6bd9dd38b3970`
- Scope: ASM/layout repair and developer-only Evidence Engine discovery; no C++,
  M13, ROM, or extracted-asset commit.

## Phase A — inherited full-layout repair

The failure was in the materializer, not in the child-table ownership. The
generated `sub_00B79A.asm` contained redundant same-value aliases for labels
defined by other slices: `loc_00B856`, `loc_00B912`, `loc_00E2D4`, and
`loc_00E7FC`. The materializer now removes only an alias whose numeric value is
identical to a label defined in the assembled layout; mismatched aliases are
preserved fail-closed. A regression test covers both cases.

The AUTO62 materialization assembled with the existing local
`vasmm68k_mot.exe` and matched the canonical ROM exactly:

- size: `3,145,728`
- CRC32: `C4728225`
- SHA256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- assembler status: `PASS`, `assembler_and_full_rom_match`
- SOURCE_OWNED: `1,475,600 / 3,145,728` before and after; delta `0`

## Phase B — real BizHawk capture

The bounded collector used BizHawk 2.11.1, loaded the canonical ROM, loaded
QuickSave1, verified frame `2117`, settled to frame `2120`, and ran the exact
directional window `quicksave1-directional-window-v1` for 120 scripted frames.
The raw receipt ended complete at frame `2240`; `EMULATOR_EXECUTED=YES`.

The harness's 30-second launcher wait expired while broad bus callbacks were
still slowing EmuHawk. The EmuHawk process remained alive, later wrote
`result=PASS`, and produced the complete raw capture. This is recorded as a
launcher timeout/performance limitation, not an emulator or state-load failure.

Persisted comparison inputs were the two read-only SQLite sidecars plus the
prior GPGX executed-PC set:

- SQLite: 27 EXEC, 24 READ, 20 WRITE addresses across the queried sidecars
- prior GPGX executed PCs: `14,732`
- watch-plan Lua SHA256: `9bcabdc88a99f968010add8ab770dbde57c2ba6403c8664e53ce752ee41523e4`
- raw capture SHA256: `bb0dc669f5120af9523833bff3241f2657c71d6b90038f661cc892e0a5e9a48c`

### Novelty result

| Observation | Count | Interpretation |
|---|---:|---|
| new execution PCs | 0 | no execution novelty beyond the prior GPGX set |
| ROM-read novelty outside owned manifest ranges | 108 | genuine new raw ROM-activity witnesses relative to the queried sidecars |
| RAM-write novelty absent from queried sidecars | 512 | genuine bounded writer witnesses; collector cap reached |
| already-owned manifest structure activity | 83 | known structure activity, not new ownership |
| unique new edges | 620 | persisted `(source PC, target address, kind)` edges |
| new roots proven | 0 | not established |
| new consumers proven | 0 | not established |
| domain expansions proven | 0 | not established |
| conflicts | 0 | none emitted |

The answer to “was genuinely new game evidence found?” is **yes**, relative to
the persisted SQLite observations and prior GPGX execution set: the real
BizHawk run produced 108 unknown-range ROM-read witnesses and 512 previously
unpersisted RAM-write witnesses. This is new raw evidence, not yet a new
typed routine, root, consumer, or ownership claim.

## Selected investigation and follow-up

The deterministic selector chose `INV-AUTO62-02D7B024766C880B`:

`PC=0x2872 -> ROM=0x0CBC20`, in unknown range `0x0C0000..0x11F9FF`.

The existing bounded `oasis_re_slice` was run at `0x2872` with a `0x300`-byte
budget. It decoded 13 supported instructions in one basic block, with no
unsupported or unresolved control flow, but no absolute ROM reference. The
observed target remains register-based. Follow-up status is therefore
`STATIC_SLICE_COMPLETE_INCONCLUSIVE`; no new root or consumer is claimed.

Persistent artifacts remain local under
`build/thor-evidence/live-discovery/auto62-live-a/`:

- `capture.raw.jsonl`
- `discovery_report.json` (schema v2)
- `static-followup-02872/report.json`

## Final status

Phase A is a byte-exact full-layout assembly pass. Phase B found genuine new
runtime observations, but the first selected candidate did not close a causal
static contract. SOURCE_OWNED remains `1,475,600 / 3,145,728 = 46.9080607096%`;
no promotion is authorized by this campaign. Final Git/CI identity is recorded
in the publication gate after validation.
