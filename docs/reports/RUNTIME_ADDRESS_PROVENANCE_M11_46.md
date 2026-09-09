# M11.46 — Runtime Address Provenance and Memory-Class Resolution

Result: `BOUNDED_RUNTIME_ADDRESS_PROVENANCE_PROVEN`.

## Gate identity

The M11.45 `BASIC_BLOCK_NATIVE` baseline was rerun twice after the observer
change. Both runs matched checkpoint aggregate
`251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d`, video
`5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58`,
6,488,773 total guest instructions, 6,199,718 translated instructions,
289,055 interpreter instructions, 580 registered ranges, 149,059 boundary
yields, 288 interrupted resumptions, zero fallback entries, zero hardware
visible accesses and zero original starts inside translated ranges.

The new `BASIC_BLOCK_ADDRESS_PROVENANCE` mode uses the same native block
registry and existing GPGX callback path. It observes `HOOK_M68K_E`,
`HOOK_M68K_R/W` and `HOOK_M68K_POST` only; it does not read or write machine
state. The external bridge defines R/W as top-level data accesses, while
immediate instruction reads use the separate immediate-read path.

Three independent 600-frame processes (two Debug and one Release) produced
identical observer output SHA-256
`CAE29E15E0CBC51D3726BE7DD55814A81DF3B783906D6B47884E4169ECC01420`.
Each recorded 289,055 instruction entries and zero unclosed entries. The
native metrics and checkpoint/video identities above were unchanged in all
three processes.

## Remainder selection and closure

The final M11.45 ledger contains 635 `UNKNOWN_WITH_EVIDENCE` PCs whose blocker
is unresolved register-based addressing. Their dynamic counts sum exactly to
149,678. Every one of those 635 PCs appeared in the observer output with the
same dynamic count. The observer recorded address, width, direction, ordered
bus sequence and before/after A0–A7 transition aggregates for each PC.

| observed address result | PCs | executions | % of total 6,488,773 | % of selected remainder |
| --- | ---: | ---: | ---: | ---: |
| `SAFE_MEMORY_OBSERVED` | 532 | 112,490 | 1.7336% | 75.1547% |
| `HARDWARE_REACHABLE` | 65 | 14,085 | 0.2171% | 9.4102% |
| `MIXED` | 20 | 21,272 | 0.3278% | 14.2118% |
| `UNRESOLVED_EXACT_EVIDENCE` | 18 | 1,831 | 0.0282% | 1.2233% |
| **TOTAL** | **635** | **149,678** | **2.3067%** | **100.0000%** |

`SAFE_MEMORY_OBSERVED` means every observed data-bus address was ROM, main
RAM, or an unmapped address. `HARDWARE_REACHABLE` means only hardware classes
were observed. `MIXED` means both sets occurred for the same executed PC.
The address classes are `ROM`, `MAIN_RAM`, `VDP`, `Z80_RAM`, `Z80_CONTROL`,
`YM2612`, `PSG`, `IO`, `CART_SRAM`, `OTHER_HARDWARE`, `UNMAPPED` and
`UNKNOWN`; no observed address was assigned a semantic gameplay name.

The 18 unresolved rows are exact-evidence cases where the decoder identifies
register-based address computation but the instruction has no data-bus access
to observe. They are predominantly LEA forms, including `0x061482` and
`0x0614A8` at 875 executions each. Their PC/count/opcode/decoded form and
register-transition evidence remain in the M11.45 ledger plus the untracked
local observer outputs; they are not placed in a generic bucket.

## Required high-payoff candidates

| PC | observed executions | dominant observed bus sequence | resolved result | promotion eligibility |
| --- | ---: | --- | --- | --- |
| `0x00026A` | 16,384 | `W2:0x00FF0002; W2:0x00FF0000` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x060310` | 11,489 | `R1:0x00FF077C; W1:0x00A00017` | `MIXED` / `MAIN_RAM + Z80_RAM` | `HARDWARE_BLOCKED` |
| `0x06135E` | 8,192 | `R1:0x00062E38; W1:0x00A00000` | `MIXED` / `ROM + Z80_RAM` | `HARDWARE_BLOCKED` |
| `0x003A0C` | 7,613 | `R1:0x00FF322A; W1:0x00FF322E` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x00389E` | 7,124 | `R1:0x00FF3617; W1:0x00FF361F` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x0003F0` | 4,565 | `W2:0x00FF0BFE` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x00D990` | 3,567 | `W2:0x00C00000` | `HARDWARE_REACHABLE` / `VDP` | `HARDWARE_BLOCKED` |
| `0x002C12` | 2,784 | `R2:0x00C00004` | `HARDWARE_REACHABLE` / `VDP` | `HARDWARE_BLOCKED` |
| `0x06193C` | 2,430 | `R1:0x00FF001A` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x061954` | 2,041 | `W1:0x00FF0781` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x061266` | 1,890 | `W1:0x00FF001A` | `SAFE_MEMORY_OBSERVED` / `MAIN_RAM` | `SEMANTICS_STILL_REQUIRED` |
| `0x060BA4` | 914 | `R1:0x00A00003` | `HARDWARE_REACHABLE` / `Z80_RAM` | `HARDWARE_BLOCKED` |

The historical `0x03A7AE` rejection remains preserved as M11.35 evidence and
is still obsolete after the M11.36+ bridge, M11.38 boundary contract and
M11.43 canonicalization. It is already promoted in M11.45 and therefore does
not appear in the remaining unknown set. No bus contract was broadened for
`0x060BA4` or any other hardware-reachable row.

## Decision

Runtime address and memory class are proven for a bounded subset: 147,847 of
149,678 selected unknown executions (98.7767%). The remaining 1,831 executions
are retained as `UNKNOWN_WITH_EVIDENCE` because the existing callback path
cannot observe a data-bus address for address-computation-only instructions.
No native gameplay block was promoted, no generator support was changed, and
the 95% dynamic gate was not redefined or forced. This is an address-provenance
result only; semantic, hardware, and exact effective-address proof remain
separate gates.

No ROM, asset, emulator binary, generated run evidence or `game.srm` is part
of the repository change.
