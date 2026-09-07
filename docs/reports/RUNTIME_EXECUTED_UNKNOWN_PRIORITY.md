# Runtime-Executed Unknown Priority

Decision: `RUNTIME_UNKNOWN_PRIORITIZATION_HIGH_VALUE`

Canonical ROM SHA-256: `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
Total unknown PCs: **12698**
Total `RUNTIME_EXECUTED_REGION`s: **9012**

## Ranking method

score = min(2*observed,40) + min(decoded,25) + trusted incoming (10 each, cap 20) + outgoing edges (5 each, cap 10) + static corroboration 12 + candidate overlap 8 + Ghidra overlap 10 + explorer overlap 8 + nearest trusted range 8/4 - unsupported (4 each, cap 16) - tiny fragment 8 - conflict 10; repeated-hit data was unavailable, so it contributes 0.

## Top 20

| Rank | Region | PCs | Decoded | Static support | Incoming | Outgoing | Nearest trusted | Score | Reason |
|---:|---|---:|---:|---|---:|---:|---|---:|---|
| 1 | 0x000374-0x0003A0 | 23 | 100.000% | NONE | 3 | 5 | 0x0007C4-0x0007E2 | 99 | +40 23 observed PCs; +23 23 decoded PCs; +10 known outgoing edge; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range |
| 2 | 0x00E270-0x00E298 | 21 | 80.952% | NONE | 48 | 0 | 0x00E338-0x00E36E | 87 | +40 21 observed PCs; +17 17 decoded PCs; +20 trusted incoming xref; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range; -16 decoder unsupported |
| 3 | 0x060BB6-0x060BC4 | 8 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 8 | 15 | 0x0611F4-0x06121A | 80 | +16 8 observed PCs; +8 8 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 4 | 0x00E302-0x00E314 | 10 | 100.000% | NONE | 39 | 0 | 0x00E338-0x00E36E | 76 | +20 10 observed PCs; +10 10 decoded PCs; +20 trusted incoming xref; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range |
| 5 | 0x00D44A-0x00D462 | 13 | 100.000% | NONE | 1 | 5 | 0x00D3B2-0x00D406 | 75 | +26 13 observed PCs; +13 13 decoded PCs; +10 known outgoing edge; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range |
| 6 | 0x060BFE-0x060C08 | 6 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 6 | 13 | 0x0611F4-0x06121A | 74 | +12 6 observed PCs; +6 6 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 7 | 0x0082FC-0x00832C | 25 | 100.000% | NONE | 0 | 0 | 0x0083D4-0x0083F8 | 73 | +40 25 observed PCs; +25 25 decoded PCs; +8 near trusted range |
| 8 | 0x0040B2-0x0040C8 | 12 | 100.000% | NONE | 3 | 23 | 0x003820-0x003B3E | 72 | +24 12 observed PCs; +12 12 decoded PCs; +10 known outgoing edge; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range |
| 9 | 0x060FA0-0x060FA8 | 5 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 6 | 13 | 0x0611F4-0x06121A | 71 | +10 5 observed PCs; +5 5 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 10 | 0x061042-0x06104A | 5 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 6 | 13 | 0x0611F4-0x06121A | 71 | +10 5 observed PCs; +5 5 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 11 | 0x0610D8-0x0610E0 | 5 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 6 | 13 | 0x0611F4-0x06121A | 71 | +10 5 observed PCs; +5 5 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 12 | 0x0096B6-0x0096CA | 11 | 100.000% | NONE | 4 | 15 | 0x0094A2-0x0094D2 | 69 | +22 11 observed PCs; +11 11 decoded PCs; +10 trusted incoming xref; +10 known outgoing edge; +8 candidate-map overlap; +10 Ghidra overlap; +8 near trusted range; -10 data/conflict overlap |
| 13 | 0x0604D8-0x0604DE | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 5 | 11 | 0x060352-0x06042A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 14 | 0x060BF4-0x060BFA | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 15 | 0x060C0C-0x060C12 | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 16 | 0x060C22-0x060C28 | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 17 | 0x060C38-0x060C3E | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 18 | 0x060C4E-0x060C54 | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 19 | 0x060C64-0x060C6A | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |
| 20 | 0x060C7A-0x060C80 | 4 | 100.000% | RUNTIME_EXECUTED_STATIC_CORROBORATED | 4 | 11 | 0x0611F4-0x06121A | 68 | +8 4 observed PCs; +4 4 decoded PCs; +10 known outgoing edge; +12 static corroboration; +8 candidate-map overlap; +10 Ghidra overlap; +8 explorer overlap; +8 near trusted range |

## Runtime + static corroboration

Regions: **525**; showing the top 20 by the same score.

- `0x060BB6-0x060BC4` score=80 support=['0x0006042A']
- `0x060BFE-0x060C08` score=74 support=['0x0006042A']
- `0x060FA0-0x060FA8` score=71 support=['0x00060F8C', '0x0006042A']
- `0x061042-0x06104A` score=71 support=['0x00061032', '0x0006042A']
- `0x0610D8-0x0610E0` score=71 support=['0x000610C8', '0x0006042A']
- `0x0604D8-0x0604DE` score=68 support=['0x000604BC', '0x0006042A']
- `0x060BF4-0x060BFA` score=68 support=['0x0006042A']
- `0x060C0C-0x060C12` score=68 support=['0x0006042A']
- `0x060C22-0x060C28` score=68 support=['0x0006042A']
- `0x060C38-0x060C3E` score=68 support=['0x0006042A']
- `0x060C4E-0x060C54` score=68 support=['0x0006042A']
- `0x060C64-0x060C6A` score=68 support=['0x0006042A']
- `0x060C7A-0x060C80` score=68 support=['0x0006042A']
- `0x060C98-0x060C9E` score=68 support=['0x0006042A']
- `0x060CAE-0x060CB4` score=68 support=['0x0006042A']
- `0x060CC6-0x060CCC` score=68 support=['0x0006042A']
- `0x0093C0-0x0093C4` score=65 support=['0x0000938E', '0x00008F22']
- `0x00945E-0x009462` score=65 support=['0x00009460', '0x0000938E', '0x00008F22']
- `0x00947C-0x009480` score=65 support=['0x00009460', '0x00008F22']
- `0x060B64-0x060B68` score=65 support=['0x0006042A']

## Top 5 bounded static slices

Slices follow only observed local direct control flow; they do not define functions or promote trust.

### 1. 0x000374-0x0003A0

Instructions: 23; edges: 1

- branch: `0x0003A0` -> `0x000380`
### 2. 0x00E270-0x00E298

Instructions: 5; edges: 0

- stop `0x00E278`: DECODE_UNSUPPORTED
### 3. 0x060BB6-0x060BC4

Instructions: 7; edges: 1

- jump: `0x060BC2` -> `0x060B90`
- stop `0x060BC2`: direct-control-flow-leaves-region
### 4. 0x00E302-0x00E314

Instructions: 3; edges: 1

- jump: `0x00E306` -> `0x00E332`
- stop `0x00E306`: direct-control-flow-leaves-region
### 5. 0x00D44A-0x00D462

Instructions: 13; edges: 1

- branch: `0x00D462` -> `0x00D44E`

## Anchor checks

| Anchor | Exact classification | In unknown region | Nearest unknown region | Distance |
|---|---|---|---|---:|
| 0x003820 | CODE_STATIC_SUPPORTED | no | 0x00381E-0x00381E | 2 |
| 0x0062CC | CODE_STATIC_SUPPORTED | no | 0x0062E4-0x0062E4 | 24 |
| 0x009BF2 | CODE_STATIC_SUPPORTED | no | 0x009BC0-0x009BC0 | 50 |
| 0x00A8DA | NOT_OBSERVED | no | 0x00A8F0-0x00A8F0 | 22 |
| 0x00D3B2 | CODE_STATIC_SUPPORTED | no | 0x00D406-0x00D406 | 84 |
| 0x06121A | CODE_EXECUTED | no | 0x061232-0x061232 | 24 |

## Systemic pattern

4949 of 9012 regions overlap Ghidra ranges, while only 525 have corroborating trusted/static candidate evidence. The dominant gap is boundary/trust corroboration, not a proven decoder failure; no mass classifier repair is applied.

## Recommended single next target

`0x000374-0x0003A0 (score 99)`
