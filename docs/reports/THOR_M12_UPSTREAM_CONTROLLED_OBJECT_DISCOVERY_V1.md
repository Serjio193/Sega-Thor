# M12 Upstream Controlled Object Discovery V1 — STOP

Date: 2026-09-22
Target: `ram-FF13CC-pc-00A372`, base `0xFF13CC`, candidate stride `8`
Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`

## Result

`FF13CC` is reclassified as `SAT_SHADOW_BUFFER`. The accepted exact entity-to-
SAT relation remains valid as a renderer/data-transfer relation, but no
upstream gameplay RAM object, gameplay X/Y field, or stable gameplay-object
stride is proven. The historical candidate artifact is retained unchanged;
the active semantic classification removes this address from gameplay entity
interpretation.

```text
FF13CC_STRUCTURE_CLASS              = SAT_SHADOW_BUFFER
SAT_SHADOW_LAYOUT_MATCH             = YES
SAT_SHADOW_BUFFER                   = YES
GAMEPLAY_ENTITY_CANDIDATE_STATUS    = REMOVED_FROM_ACTIVE_CANDIDATE_SET
UPSTREAM_RAM_STRUCTURE_CANDIDATES   = []
UPSTREAM_RENDER_RECORD_CANDIDATES   = [ROM:0x00A438, ROM:0x00A480]
EXACT_UPSTREAM_STRIDES              = [8] (ROM render-record tables only)
GAMEPLAY_X_SOURCE                   = UNRESOLVED
GAMEPLAY_Y_SOURCE                   = UNRESOLVED
ENTITY_ROLE                         = SAT_SHADOW_BUFFER
PLAYER_LABEL_PROVEN                 = NO
```

## Proven producer and shadow layout

The bounded static producer report proves routine `0x00A342..0x00A438`, with
callers `0x00A19C` and `0x00A6A0`. It initializes `A5` from `0xFF13CC` plus
the signed offset in `0xFF188C`, selects one of two nine-record ROM tables,
and emits a fixed record grammar. Both roots have nine records of eight bytes:

| source | records | stride | fields | selector |
| --- | ---: | ---: | --- | --- |
| ROM `0x00A438..0x00A480` | 9 | 8 | long `+0`, word `+4`, word `+6` | `FF1858 == 0` |
| ROM `0x00A480..0x00A4C8` | 9 | 8 | long `+0`, word `+4`, word `+6` | `FF1858 != 0` |

The exact selected-candidate store is `0x00A372: MOVE.L D2,(A5)+`. Its
static dataflow is:

```text
ROM record long +0
  -> A0 postincrement read at 0x00A36C
  -> D2
  -> low-byte replacement at 0x00A370 using D5.B + 1
  -> 0x00A372 MOVE.L D2,(A5)+
  -> FF13CC when FF188C is zero
```

The adjacent word-producing path is also fixed-table based: `0x00A374` reads
the next ROM word into D5, `0x00A376` reads the following ROM word into D2,
and `0x00A378` adds D3 before the documented word output. The current stock
observer reports post-store/next-fetch PCs for these neighboring writes, so it
does not provide an authoritative native instruction identity for each
`FF13D0`/`FF13D2` write. That ambiguity is not promoted to causality.

The only RAM inputs proven in this producer family are control/selector state:
`FF188A`, `FF188C`, `FF1858`, `FF185A`, `FF184F`, `FF1856`, `FF1854`,
`FF1855`, `FF1892`, and `FF1996`. None is proven to be gameplay X, gameplay Y,
or an upstream object record.

## SAT and DMA continuation

The accepted prior artifacts prove:

```text
FF13CC source RAM
  -> exact 184-byte 68K_BUS DMA
  -> VRAM destination 0xD000
  -> SAT entries 0..22 in the accepted DMA state
  -> selected historical candidate links entries 0..4
```

The candidate-specific exact links are `sat_entries=[0,1,2,3,4]` for
`ram-FF13CC-pc-00A372`; this is an exact shadow-to-SAT link, not proof that
the shadow record is a gameplay object. The DMA evidence is from the preserved
post-run corpus and is not a new Worker capture.

## Upstream search and failure classification

No preserved evidence shows a RAM address being read as an object record and
then transformed into the `A5` source fields. Therefore there is no exact
gameplay X/Y chain:

```text
gameplay X -> transform -> FF13CC SAT X -> SAT entry X = UNRESOLVED
gameplay Y -> transform -> FF13CC SAT Y -> SAT entry Y = UNRESOLVED
```

The current blockers are:

| edge | result | reason |
| --- | --- | --- |
| upstream RAM source -> producer | missing | producer reads the selected ROM roots; RAM reads are selector/counter state only |
| producer field -> exact neighbor write identity | incomplete | stock observer exposes post-store/next-fetch callbacks, not W3 native instruction identity |
| upstream object stride/lifetime | missing | no gameplay RAM record source or activation boundary is captured |
| input -> upstream object | missing | targeted run showed no input-dependent mutation of the selected shadow fields |

## Required next capture

Only a targeted W3 V2 native capture is justified. It must preserve the
accepted identity tuple `(run_id, epoch, frame, stream_sequence,
instruction_sequence, PC, address, value, width, domain)` and bounded register
snapshots around `0x00A342..0x00A37C` and the producer callers. It must include
the `A0` source reads, writes/readbacks of `FF188A`, `FF188C`, `FF1858`, and
the deliberate directional/action transitions from run `1790106191`, extended
long enough to observe one selected-slot activation through deactivation or
reuse. Lua-local frame or callback identity is insufficient.

## Acceptance

```text
UPSTREAM_CONTROLLED_OBJECT_DISCOVERY_V1 = STOP_UPSTREAM_GAMEPLAY_SOURCE_UNPROVEN
SOURCE_OWNED_BEFORE                    = 1487672
SOURCE_OWNED_AFTER                     = 1487672
SOURCE_OWNED_DELTA                     = 0
COMMIT_CREATED                         = NO
PUSH_PERFORMED                         = NO
```

Evidence inputs:

- ignored static producer report:
  `build/thor-evidence/upstream-controlled-object-discovery-static.json`
  (SHA-256 `7C5742ADA1B8D498F8556025C62A1798D87E73C38B2E7723FD870D774F0F4B9F`);
- preserved exact entity/SAT links:
  `build/thor-evidence/live-worker-control/campaign-desktop-20260922-083246-302/post-run-gameplay-v1/postrun_entity_sat_links.json`;
- preserved exact DMA report:
  `build/thor-evidence/live-worker-control/campaign-desktop-20260922-083246-302/post-run-gameplay-v1/postrun_vdp_dma.json`;
- targeted run `1790106191`, frames `2117..2172`, which did not contain the
  required W3 native identity or a mutation of the selected shadow fields.
