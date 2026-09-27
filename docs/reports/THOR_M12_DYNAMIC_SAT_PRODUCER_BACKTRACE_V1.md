# M12 Dynamic SAT Producer Backtrace V1

Date: 2026-09-23
Result: `PASS_DYNAMIC_SAT_PRODUCER_BACKTRACE_V1`

## Static writer decode

The runtime callback PC is one instruction after the bus store in this
BizHawk/GPGX observation. The exact mapping is:

| Callback PC | Actual instruction | Destination | Source | Width | SAT meaning |
|---|---|---|---|---:|---|
| `0x00B754` | `0x00B752 MOVE.W D6,(A1)` | `A1+0` | `D6` | 2 | Y |
| `0x00B768` | `0x00B764 MOVE.W D7,2(A1)` | `A1+2` | `D7` | 2 | size/link |
| `0x00B772` | `0x00B76E MOVE.W D6,4(A1)` | `A1+4` | `D6` | 2 | X |
| `0x00B77E` | `0x00B77A MOVE.W D6,6(A1)` | `A1+6` | `D6` | 2 | tile/attribute |

The X path is exact:

```text
0x00B768: MOVE.W 2(A0),D6       source read
0x00B76C: ADD.W D3,D6           word transform
0x00B76E: MOVE.W D6,4(A1)       SAT X store
```

## Exact witness: frame 1500 -> 1501

```text
FRAME              = [1500, 1501]
STREAM/ORDER ID    = frame-boundary order in accepted capture
SAT_ENTRY          = 0
SAT_FIELD          = X
SHADOW_ADDRESS     = 0xFF13D0
OLD_VALUE          = 0x0423
NEW_VALUE          = 0x0429
WRITE_PC           = 0x00B772 (callback), actual 0x00B76E
WRITE_WIDTH        = 2 bytes
```

Runtime register/source evidence at frame 1500:

```text
A0                 = 0x0017435A
SOURCE_READ_PC     = 0x00B768
SOURCE_ADDRESS     = 0x0017435C
SOURCE_VALUE       = 0x0029
D3                 = 0x0400
DERIVED            = 0x0029 + 0x0400 = 0x0429
```

The source address is inside the canonical ROM. This is `DERIVED_EXACT` ROM
provenance, not a gameplay RAM/object proof. The upstream definition of A0 is
outside this bounded local capture and remains `UNRESOLVED`.

## Multi-frame and controller results

The analyzer closes 23 exact transitions for SAT entry 0 X-field publication,
well above the required three. The deliberate input sequence is retained only
as correlation: UP has a nearby `720 -> 721` transition and ACTION has the
selected `1500 -> 1501` transition; RIGHT, LEFT, and DOWN have no nearby
transition for this selected field. Correlation is not treated as causality.

Artifacts are emitted under ignored
`build/m12-dynamic-sat-producer-v1-run2/report/`:

- `postrun_dynamic_sat_producer_analysis.json`
- `postrun_dynamic_sat_backtrace.json`
- `postrun_dynamic_sat_source_candidates.json`
- `postrun_dynamic_sat_producer_receipt.json`

`SOURCE_OWNED_BEFORE = 1,487,672`, `SOURCE_OWNED_AFTER = 1,487,672`, delta 0.
`PLAYER_LABEL_PROVEN = NO`; `COMMIT_CREATED = NO`; `PUSH_PERFORMED = NO`.
