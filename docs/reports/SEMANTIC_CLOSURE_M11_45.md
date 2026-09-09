# M11.45 — Bounded Semantic Closure Toward 95% Dynamic Coverage

Result: `REMAINING_ATTRIBUTION_AND_DYNAMIC_COVERAGE_95_PROVEN`.

## Gate identity

The committed M11.44 baseline `b1c624072e1341bbe26a569841d325152cf3bd7b`
was run twice without code changes. Both runs matched checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`, ROM/DLL
identity, 6,488,773 total instructions, 5,927,805 translated, 560,968
interpreter, 29 registered ranges, 142,813 yields, 288 interrupted resumptions
and zero original starts inside translated ranges.

## Selection and exact verification

The complete M11.44 final ledger was ranked by dynamic count. The selected
candidate set contains 551 decoder-owned single-instruction ranges and has
271,913 dynamic executions. It includes only rows with exact IR, proven static
memory class, and either existing bounded semantic proof or a new exact helper.
Register-based/other memory, hardware-visible, indirect-CFG, decoder and unknown
runtime-address rows were excluded. The pre-shadow eligible upper bound was
`>=236,530` executions; the selected set remains above that bound after each
shadow veto.

Independent vectors cover the exact selected combinations for BTST immediate
absolute-long/data-register, MOVEQ, MOVE.W data-to-data, ADD/SUB.W data-to-data,
ADDQ.W, SUBQ.B/W, ANDI.B/W, OR.W, ADDI.B, SUBI.B, BCLR.L data-register, MOVEA.L,
and the existing exact helper forms. Tests verify result, registers, SR/CCR/X,
effective address behavior where applicable, memory width/order and bit-index
modulo. The generator accepts only these exact operation/size/addressing-mode
forms and fails closed otherwise.

Three candidates were vetoed and remain interpreter fallback:

| candidate | exact shadow mismatch | decision |
| --- | --- | --- |
| `0x0038E0 LSR.W #1,D7` | timing `actual +14 cycles`, refresh equal | fail closed |
| `0x06115A ROR.W #8,D0` | timing `actual +112 cycles`, refresh equal | fail closed |
| `0x0038AA CMPI.B #$60,D1` | SR X divergence: actual clear, predicted preserved | fail closed |

The historical M11.35 rejection at `0x03A7AE` (`actual IR/prefetch 0x4E73`,
expected `0x4A79`) remains preserved in M11.44 evidence and is obsolete after
the M11.36+ bridge, M11.38 boundary contract and M11.43 canonicalization. The
current M11.45 shadow includes the already-proven generated
`0x03A7AE–0x03A7B8` block with no divergence.

## Shadow and native proof

The full selected registry passed `6,199,718/6,199,718` per-instruction shadow
comparisons with zero divergence across CPU/PC/SR/IR/prefetch/RAM/bus/cycles/
refresh/event/interrupt/continuation state. The unchanged external GPGX
600-frame native scenario completed with the canonical ROM and pinned DLL:

| metric | M11.44 baseline | M11.45 final |
| --- | ---: | ---: |
| total guest instructions | 6,488,773 | 6,488,773 |
| translated instructions | 5,927,805 | 6,199,718 |
| interpreter instructions | 560,968 | 289,055 |
| translated share | 91.3548% | 95.5453% |
| registered ranges | 29 | 580 |
| natural translated entries | 2,624,284 | 2,896,197 |
| average instructions/translated entry | 2.2581 | 2.1406 |
| boundary yields | 142,813 | 149,059 |
| interrupted resumptions | 288 | 288 |
| hardware-visible fallback count | 0 | 0 |
| unexpected fallback entries | 0 | 0 |
| starts inside translated ranges | 0 | 0 |

Checkpoint, video, CPU/RAM/VDP/sound, interrupt-visible behavior and continuation
identity matched. No hardware emulation was broadened; `0x060BA4` remains
`HARDWARE_VISIBLE_BLOCKED` in the final ledger.

## Final Pareto

The exhaustive final ledger is in
`REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md`; its rows sum exactly to 289,055.
The required categories are:

| category | executions | % of total 6,488,773 | % of remaining 289,055 |
| --- | ---: | ---: | ---: |
| semantic | 106,022 | 1.6339% | 36.6788% |
| runtime/prefetch | 0 | 0.0000% | 0.0000% |
| hardware-visible | 914 | 0.0141% | 0.3162% |
| indirect CFG | 267 | 0.0041% | 0.0924% |
| decoder | 6,196 | 0.0955% | 2.1435% |
| cold/low-payoff | 25,978 | 0.4004% | 8.9872% |
| unknown-with-evidence | 149,678 | 2.3067% | 51.7818% |
| **TOTAL** | **289,055** | **4.4547%** | **100.0000%** |

Runtime/prefetch is zero as a terminal class: M11.43 canonicalization and the
M11.44 bridge repair removed the prior representation blocker. The remaining
unknown rows retain exact register-based/runtime-address evidence and are not
silently relabeled. The hottest remaining PCs are `0x03A8A4` (26,887,
CMPI.W absolute-long), `0x00026A` (16,384, MOVE.L predecrement), `0x060310`
(11,489, MOVE.B postincrement-to-postincrement), `0x06135E` (8,192),
`0x003A0C` (7,613), `0x00389E` (7,124), `0x0003F0` (4,565), `0x003810`
(4,333), `0x0030B6` (4,074), `0x03A750` (3,959), `0x00D990` (3,567) and
`0x00317E` (3,302).

No ROM, asset, emulator binary, generated run evidence or `game.srm` is tracked.
