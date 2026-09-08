# M11.37 Controlled Dynamic Coverage Expansion

## Result

`CONTROLLED_DYNAMIC_COVERAGE_EXPANSION_PROVEN`

This is a bounded developer-only hybrid result, not ROM-byte coverage, a
whole-ROM translation, a production CPU model or a runtime JIT. The unchanged
cold-reset neutral scenario used the canonical USA ROM and the M11.36 external
GPGX identity:

| identity | value |
| --- | --- |
| ROM SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` |
| GPGX source commit | `d60d079934977aa6973e220d123533387159f66e` |
| GPGX DLL SHA-256 | `9b345293c239805cbfe22bb3c582e7d42164a701ba2c50e1934e1a8ef80b2ec8` |
| scenario | cold reset, neutral input, 600 frames |

## Before/after metrics

| metric | EMULATED before | BASIC_BLOCK_NATIVE after |
| --- | ---: | ---: |
| guest instruction executions | 6,488,773 | 6,488,773 |
| translated instruction executions | 0 | 388,314 |
| interpreter instruction executions | 6,488,773 | 6,100,459 |
| translated instruction share | 0% | 5.9844% |
| unique observed interpreter PCs | 2,188 | 2,161 |
| unique registered basic-block entries | 6 | 22 |
| natural block entries | n/a | 388,308 |
| shadow comparisons | n/a | 388,308 / 388,308 |
| divergence count | n/a | 0 |
| interpreter fallback entries | n/a | 12 |
| original starts inside translated blocks | n/a | 0 |
| hardware-visible accesses | n/a | 0 |
| video frames | 600 | 600 |

The before and after serialized checkpoint hash is
`b8e1e07908d75e9c8b21f3ed661352a7005c51dcf3120e2502cc0f73788a4bd4` and the
video sequence hash is
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`.
The six historical entries were preserved. Sixteen new entries were promoted.

## Candidate queue and gates

The single bounded discovery produced 2,188 unique PCs. Exact slicing and the
existing mechanical generator considered the following 61 generator-eligible
entry PCs. `Bcc` and `DBcc` rows have direct control flow; `TST` rows have an
absolute-long RAM or hardware reference. `SEMANTIC-FORM` means the instruction
family was independently covered by the M11.34/M11.35 semantic harness. A
candidate marked `CAP` was not runtime-tested after the bounded queue was
filled; it is not promoted or trusted.

| entry | observed count | decoded form / control flow | status |
| --- | ---: | --- | --- |
| `0x00026C` | 16,384 | `DBF D6 -> 0x00026A` | CAP; SEMANTIC-FORM |
| `0x0003A0` | 98,288 | `DBF D2 -> 0x000380` | PROMOTED |
| `0x0003F2` | 4,565 | `DBF D0 -> 0x0003F0` | PROMOTED |
| `0x002230` | 9,940 | `DBF D0 -> 0x002230` | PROMOTED |
| `0x002C18` | 2,784 | `BNE.S -> 0x002C12` | PROMOTED |
| `0x002CD4` | 1,248 | `DBF D2 -> 0x002CD2` | CAP; SEMANTIC-FORM |
| `0x0030BE` | 4,074 | `BNE.S -> 0x0030B6` | PROMOTED |
| `0x003186` | 3,302 | `BNE.S -> 0x00317E` | PROMOTED |
| `0x003250` | 3,300 | `BNE.S -> 0x003248` | PROMOTED |
| `0x0032E4` | 1,814 | `BNE.S -> 0x0032DC` | CAP; SEMANTIC-FORM |
| `0x0032EE` | 1,121,997 | `TST.W RAM 0xFF1658; BNE.S -> 0x0032EE` | REJECTED; interrupt interleaving |
| `0x0032F4` | 1,121,997 | `BNE.S -> 0x0032EE` | CAP; overlaps rejected range |
| `0x003818` | 4,333 | `BNE.S -> 0x003810` | PROMOTED |
| `0x003864` | 1,256 | `DBF D0 -> 0x003862` | CAP; SEMANTIC-FORM |
| `0x00387E` | 996 | `DBF D0 -> 0x00387C` | CAP; SEMANTIC-FORM |
| `0x0038A0` | 7,124 | `DBF D0 -> 0x00389E` | PROMOTED |
| `0x0038DC` | 2,597 | `BMI.W -> 0x003A26` | CAP; SEMANTIC-FORM |
| `0x0038E2` | 2,597 | `BCC.W -> 0x003986` | CAP; SEMANTIC-FORM |
| `0x0038E8` | 1,364 | `BMI.W -> 0x003A34` | CAP; SEMANTIC-FORM |
| `0x0038EE` | 1,364 | `BCC.W -> 0x00398C` | CAP; SEMANTIC-FORM |
| `0x003996` | 1,355 | `BPL.W -> 0x003ACE` | CAP; SEMANTIC-FORM |
| `0x00399C` | 1,355 | `BCS.W -> 0x003A06` | CAP; SEMANTIC-FORM |
| `0x003A0E` | 7,613 | `DBF D2 -> 0x003A0C` | PROMOTED |
| `0x00D994` | 3,567 | `DBF D3 -> 0x00D990` | PROMOTED |
| `0x03A758` | 3,959 | `BNE.S -> 0x03A750` | PROMOTED |
| `0x03A78A` | 2,095 | `BNE.S -> 0x03A782` | CAP; SEMANTIC-FORM |
| `0x03A7AE` | 50,474 | `TST.W RAM 0xFF1654` | REJECTED; prior IR/prefetch mismatch |
| `0x03A7B4` | 50,474 | `BNE.W -> 0x03A7AE` | CAP; overlaps rejected range |
| `0x03A864` | 132,187 | `BNE.W -> 0x03A85E` | CAP; adjacent historical range |
| `0x03A892` | 26,887 | `BNE.W -> 0x03A8A4` | CAP; adjacent historical range |
| `0x03A8AC` | 26,887 | `BNE.W -> 0x03A8BA` | PROMOTED |
| `0x03A8C0` | 26,887 | `BNE.W -> 0x03A88C` | CAP; adjacent historical range |
| `0x03A9AC` | 248,291 | `TST.W RAM 0xFFAFAE; BNE.S -> 0x03A9BC` | REJECTED; interrupt interleaving |
| `0x03A9B2` | 248,291 | `BNE.S -> 0x03A9BC` | CAP; overlaps rejected range |
| `0x03A9B4` | 248,290 | `TST.B RAM 0xFF0BFD; BNE.S -> 0x03A9CA` | REJECTED; interrupt interleaving |
| `0x03A9BA` | 248,290 | `BNE.S -> 0x03A9CA` | CAP; overlaps rejected range |
| `0x03A9CA` | 248,290 | `TST.W RAM 0xFF1654; BNE.W -> 0x03A9AC` | REJECTED; interrupt interleaving |
| `0x03A9D0` | 248,290 | `BNE.W -> 0x03A9AC` | CAP; overlaps rejected range |
| `0x03B27C` | 990 | `BNE.W -> 0x03B274` | CAP; SEMANTIC-FORM |
| `0x060312` | 11,489 | `DBF D0 -> 0x060310` | PROMOTED |
| `0x060BA0` | 914 | `BNE.W -> 0x060B98` | CAP; hardware-adjacent |
| `0x060BA4` | 914 | `TST.B hardware 0xA00003` | REJECTED; hardware-visible |
| `0x060BAA` | 914 | `BEQ.W -> 0x060BC4` | CAP; hardware-adjacent |
| `0x061268` | 1,890 | `DBF D0 -> 0x061266` | CAP; SEMANTIC-FORM |
| `0x061360` | 8,192 | `DBF D0 -> 0x06135E` | PROMOTED |
| `0x0613B8` | 1,400 | `BEQ.W -> 0x0613F6` | CAP; SEMANTIC-FORM |
| `0x061938` | 2,916 | `BNE.W -> 0x061946` | PROMOTED |
| `0x061942` | 2,430 | `BNE.W -> 0x061954` | CAP; SEMANTIC-FORM |
| `0x061950` | 1,361 | `BEQ.W -> 0x06195A` | CAP; SEMANTIC-FORM |
| `0x06196C` | 875 | `BNE.W -> 0x0619B6` | CAP; SEMANTIC-FORM |
| `0x0619BA` | 875 | `BNE.W -> 0x061AC2` | CAP; SEMANTIC-FORM |
| `0x061B98` | 875 | `BNE.W -> 0x061EE0` | CAP; SEMANTIC-FORM |
| `0x061BBC` | 875 | `BNE.W -> 0x061C74` | CAP; SEMANTIC-FORM |
| `0x061BC4` | 875 | `BEQ.W -> 0x061C56` | CAP; SEMANTIC-FORM |
| `0x061C86` | 875 | `BEQ.W -> 0x061CC8` | CAP; SEMANTIC-FORM |
| `0x061E4E` | 875 | `BEQ.W -> 0x061EDE` | CAP; SEMANTIC-FORM |
| `0x0623B0` | 1,944 | `BNE.W -> 0x0623BE` | CAP; SEMANTIC-FORM |
| `0x0623BA` | 1,458 | `BNE.W -> 0x0623E6` | CAP; SEMANTIC-FORM |
| `0x0623C8` | 1,011 | `BEQ.W -> 0x0623D0` | CAP; SEMANTIC-FORM |
| `0x0623D4` | 1,011 | `BEQ.W -> 0x0623DC` | CAP; SEMANTIC-FORM |
| `0x0623E2` | 1,011 | `BEQ.W -> 0x0623E8` | CAP; SEMANTIC-FORM |

The four rejected multi-instruction candidates were shadow-tested and failed
only because an interrupt handler can interleave inside the selected range;
they remain interpreter-only. The selected set contains no such range. The
hardware candidate was rejected before promotion because its read is observable
outside ordinary ROM/work-RAM semantics. No rejected or capped candidate is
trusted by the runtime registry.

## Validation evidence

The independent semantic harness, generator/provenance tests and generated
block tests passed. Full GPGX shadow then passed `388308/388308` comparisons
with zero divergence and full CPU equivalence. Native promotion passed the
same 600-frame run with exact checkpoint/video hashes, 12 interpreter fallback
entries, zero starts inside translated blocks and zero hardware-visible
accesses. All run directories are local ignored evidence only; no ROM, asset,
emulator binary, generated run evidence or `game.srm` is tracked.
