# M15.245 Read-Only Standalone Handoff: High-Frequency Entity Handlers, Input Combat Gate (`0x007DC8`), and Caller Continuation Topology

**Status:** Read-only engineering reference produced by `C:/Github/Sega-Thor` (Milestone M15.245).
**Standalone Worktree Policy:** `C:/Github/Sega-Thor-Standalone` remains strictly untouched (`b21e4c59817b52d4732377a08cc8a1b5a9629d74`).
**ROM & Oracle Evidence:** `Beyond Oasis (USA).md` (`3,145,728` bytes, SHA256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`) and pinned GPGX core (`ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`).

---

## 1. Executive Summary & Architectural Guidance

1. **Disjoint Production Separation Preserved:** All `196` active production helpers (`ACTIVE_SET_SHA256 = 17396fdaf9c566d36ca8317d40bb334f0f3467c73f1aebe0e26a36f8d4951867`) remain untouched, `ORACLE_OWNER_APPROVAL = PENDING`, and `0x003820` remains quarantined. High-traffic entity targets owned in production (`0x006516` = `546` entries / `61,998` steps across C/A/B, `0x0062E4` = `252` / `24,235`, `0x00663E`, `0x006FE4`, `0x00701A`, `0x00705A`) were explicitly excluded (`0` overlap).
2. **Real Portable Gameplay Scaling:** Adding `m15245p` (`508` new M68000 instructions, `230` CFG blocks, `397` edges; combined M15.244+M15.245 = `638` instructions, `273` blocks) increased verified disjoint portable gameplay displacement across Scenarios C (`1,800f`), A (`1,592f`), and B (`600f`) from `791` steps (M15.244) to **`60,307` total portable steps** (`791` M15.244 + **`59,516` new M15.245 steps**: `C = 28,912`, `A = 20,782`, `B = 9,822`), with **`24/24` bit-exact A/B/C stream parity**, `0` fallbacks, `0` exceptions, and `0` contract faults.
3. **Standalone C++20 Modeling Rule:** Implement these mechanics as native entity component updates and ring-buffered input queries, **never** M68000 registers, SR/CCR bits, or `$FF1934` saved-stack pointer rewinds.

---

## 2. Caller Topology (`0x005670 -> 0x0059B8`) & Why `0x0059A8` Was Bypassed

Observed in `data/thor-evidence/rom_005670_and_00e682_continuation_proof_m15_245.json`:
- Across Scenarios C (`861` entries, `61,165` steps), A (`772` entries, `52,455` steps), and B (`299` entries, `21,261` steps), **`100%`** (`1,932 / 1,932`) of `0x005670` entries arrive from caller `0x005586` (return site `0x00558A`).
- `1,908` of those calls exit `0x005670` through `0x0059B8 JMP (A0)` directly to the jump-table handler entry PC (while `24` return early via `0x00558A`: `C=3, A=19, B=2`).
- Because production helper `m68k_005670` owns `0x005670..0x0059B8` and stops cleanly after setting `pc = target_pc` at `0x0059B8`, standalone M15.245 entry hooks catch execution immediately at each unowned handler entry PC (`0x0061F6`, `0x007112`, `0x007656`, `0x007DC8`, etc.).
- Note on `0x007112` (index `11`): static disassembly confirms production helper `m68k_0070f8` terminates at `0x007110 RTS`; `0x007112` has **`0` production overlap** (`135` authentic entries across C/A/B).

---

## 3. Newly Compiled M15.245 Entry Roots (`508` Instructions Across `16` Roots)

| Entry PC | Canonical Role / Table Index | Observed Live Entries (C / A / B) | GPGX Dynamic Steps (C / A / B) | Compiled Edges / Exits |
| :--- | :--- | :--- | :--- | :--- |
| `0x007DC8` | `helper_7dc8_input_combat_gate` (shared input/combat BSR callee) | Called by `0x0061F6`, `0x007112`, etc. | `24,231` C (`0` overlap) | `0x007E44 RTS`, `0x007E96..0x007E9C` SP-unwind `RTS`, `0x007F82`, `0x007FB6`, `0x00822C` |
| `0x0061F6` | `state_0_walk_move` (`idx = 0`) | `113 / 151 / 21` (`285`) | `11,825 / 17,809 / 2,665` | Calls `0x007DC8`, `0x060004`, `0x0085E2`; tail `0x0062CC` |
| `0x007656` | `state_5_attack_scan` (`idx = 5`) | `28 / 28 / 0` (`56`) | `7,002 / 8,824 / 0` | Calls `0x0086B4`, `0x008778`, `0x008814`, `0x008910`, `0x008950`, `0x00E2D4`, `0x060004` |
| `0x007112` | `state_11_jump_air` (`idx = 11`) | `72 / 39 / 24` (`135`) | `3,912 / 2,123 / 1,302` | Calls `0x007DC8`, `0x0085E2`; returns `RTS` |
| `0x0066AA` | `state_28_action_6aa` (`idx = 28`) | `24 / 72 / 24` (`120`) | `2,392 / 7,352 / 2,392` | Calls `0x00BCBA`; returns `RTS` |
| `0x007852` | `state_4_combat_scan` (`idx = 4`) | `42 / 43 / 0` (`85`) | `3,333 / 3,569 / 0` | Calls `0x00BCBA`; returns `RTS` |
| `0x0071DA` | `state_7_action_1da` (`idx = 7`) | `19 / 0 / 19` (`38`) | `2,669 / 0 / 2,669` | Calls `0x009D00`; returns `RTS` |
| `0x006738` | `state_58_action_738` (`idx = 58`) | `0 / 35 / 0` (`35`) | `0 / 6,968 / 0` | Calls `0x0064C4`, `0x00BCBA`; returns `RTS` |
| `0x0078F8` | `state_27_action_8f8` (`idx = 27`) | `12 / 0 / 0` (`12`) | `1,548 / 0 / 0` | Calls `0x00BCBA`; returns `RTS` |
| `0x006EDA` | `state_19_action_eda` (`idx = 19`) | `0 / 10 / 0` (`10`) | `0 / 1,141 / 0` | Calls `0x0085E2`; returns `RTS` |
| `0x006464` | `state_53_action_464` (`idx = 53`) | `10 / 5 / 5` (`20`) | `218 / 103 / 109` | Shared action handler; returns `RTS` |
| `0x006338` | `state_24_action_338` (`idx = 24`) | `2 / 1 / 1` (`4`) | `208 / 104 / 104` | Calls `0x0063DA`; returns `RTS` |
| `0x005E78`, `0x006E28`, `0x006E4A`, `0x006E7A` | Shared stagger/recovery tails & callee (`0x006E7A`) | Shared callees/tails | Covered inside handler windows | `0x009C40`, `0x006E28`, `RTS` |

*(Note: Dynamic step totals reported in `entity_handler_hotspot_manifest_m15_245.json` reflect inclusive handler + downstream callee execution per GPGX profiler; `m15245p` disjoint self steps inside the `508` compiled instructions are `C = 28,912`, `A = 20,782`, `B = 9,822`.)*

---

## 4. Verified Entity (`A6`) Fields, Input Ring Buffer (`$FF1668`), and Nonlocal Stack Proof

### Controller Input Ring Buffer Scan (`0x007DC8..0x007EC2`)
- Reads Work RAM global `$FF1983` (disable/lock byte); returns early if non-zero (`0x007DCE`).
- Checks current entity state `$04(A6) == 0` (`walk/idle`, `0x007DD2`) and bit `7` of `$6F(A6)` (`0x007DDC`); sets `$FF1986` (`0x007DE6`) on idle ground state.
- Scans the `64`-entry word ring buffer starting at **`$FF1668`** (`LEA $FF1668, A0` at `0x007DEC`), using index **`$FF16E8`** (`D5`) and buffered count **`$FF16EA`** (`D4`):
  - Decrements `D4`, exits if negative (`0x007E00`), clamps `D4 <= 4` (`CMPI #4, D4` -> scan at most `5` recent samples via `DBRA`).
  - Accumulates `D0 |= *(uint16_t*)(0xFF1668 + D5)` while advancing `D5 = (D5 + 2) & 0x7E`.
- Evaluates accumulated input mask `D0` and global `$FF165E`:
  - `BTST #6, D0 -> 0x007E46` (attack/special branch); `BTST #4, D0 -> 0x007F82`; `BTST #4, $FF165E -> 0x007FB6`; `BTST #5, D0 -> 0x00822C`.
- Inside `0x007E46`: guards `$FF1985` (sets `= 1`), `4(A6) != 4`, `$FF1AA4 == 0`, and `142(A6) != 0`; then sets **`42(A6) = 20`** (`+0x2A`), **`4(A6) = 20`** (`2 * 10`, switching to state index `10`), clears `$FF1990` (`SF.B`), triggers SFX `$900359` via `0x060004`, and executes **`0x007E96 MOVEA.L $FF1934, A7; 0x007E9C RTS`**, unwinding past its immediate `BSR` caller directly to grandparent return `$00558A` (`+8` byte `A7` delta).

### Additional Observed Entity (`A6`) Offsets & Globals in M15.245
- **`+0x02`..`+0x04` (`2(A6)`, `4(A6)`):** Frame countdown `.w` and jump-table byte offset `2 * index` `.w`.
- **`+0x10` (`16(A6)`), `+0x72`/`+0x76` (`114`/`118(A6)`):** 32-bit height/Z accumulator and X/Y world coordinates `.l` (`ADD.L`/`MOVE.L` with velocity updates from `+0x4E`, `+0x52`, `+0x56`).
- **`+0x2A` (`42(A6)`), `+0x2E`/`+0x30` (`46`/`48(A6)`):** Sub-action/animation selector `.w` (`CMPI`/`CLR`) and facing/direction code `.w`.
- **`+0x36..+0x39` (`54..57(A6)`), `+0x40..+0x41` (`64..65(A6)`), `+0x67`/`+0x6F`/`+0x70` (`103`/`111`/`112(A6)`):** Action status, wall-clip (`OR.B`), and input/lock flags (`BSET`/`BCLR`/`BTST`/`TST`).
- **`+0x8E` (`142(A6)`), `+0x9E` (`158(A6)`), `+0xAA` (`170(A6)`):** Combat readiness `.w` (`0x007E70`), stagger reset `.w`, and secondary pointer `.l` (`MOVEA.L 170(A6), Ax`).
- **Globals:** `$FF0DB2`, `$FF165E..$FF16F0` (controller & ring state), `$FF17B8`/`$FF17BB`, `$FF1934` (saved base SP), `$FF1940`, `$FF1971`..`$FF1995`, `$FF1AA4`, `$FF1B60`.

### Nonlocal Yield & Negative Contract Coverage
- `0x00E682..0x00E6A8` continues to show live nonlocal stack unwinds (`C=4, A=43, B=2`, `49` total across C/A/B, `a7_delta=+8`), mirrored in `0x007E96 MOVEA.L $FF1934, A7` and `0x005C82`.
- All `28` M15.245 reference fixtures (`18 AUTHENTIC + 10 CONTROLLED`), Phase 5 nonlocal unwind (`+6` mutations), and `9` Phase 7 negative/contract assertions pass `100%` in Debug and Release across GPGX Oracle, Mode A, and fused Mode B (`200/508` fixture insns; `442/508` live C+A+B insns [`87.0%`]; `457/508` combined [`90.0%`]).

---

## 5. Frozen Trace Hashes & Remaining Unverified Table Indices

- **Expanded IR & Source Map:** `data/thor-evidence/entity_dispatch_cfg_source_map_m15_245.json` (`508` new insns / `638` combined, `230` new blocks / `273` combined, `397` edges, `generated_ir_sha256 = 1ea72912c21af03847ef411366cabf61ad25dfbb2bc66fc0e9ad0b2621a6f690`, `generated_cpp_umbrella_sha256 = e0d168ab69f67a787b9c47be10cae8cd84e8123654e46d08db755a158c2c945e`).
- **Isolated Runner (`SHA256 = 3461acf8e3b6715790350609ce57d22a3538aab92dea271abd492ac2e332bf81`, `3,380,704` bytes) — `24/24` Bit-Exact `full-frame-state.jsonl` Hashes:**
  - `scenario_c`: `188f329c8919400406c72c783c7ada5992905490d465a6e7463a885e4017d550` (1,800f)
  - `scenario_a`: `499ac47714012e61a02f93aed4a32e016cc6fad8cb9407f3ce4c004bfb331040` (1,592f)
  - `scenario_b`: `f2e3fcdbd51475562e3cb123acd505aa02e4f4970c3daa18cdca9eebe639d0c8` (600f)
- **Remaining Unverified Table Entries:** `34 / 59` jump-table indices (`[3, 8..10, 18, 20..22, 25..26, 30..31, 33..48, 51, 54..56]` plus unreached static branches in `51 / 508` M15.245 instructions) were never entered in Scenarios C, A, or B and remain unverified pending dedicated gameplay state captures.
