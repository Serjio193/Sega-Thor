# STANDALONE M15.258 SAFE COMPUTATIONAL ISLANDS & HARNESS TRIAGE HANDOFF

**Milestone:** SEGA-THOR — M15.258  
**Track:** HYBRID_ROM_TO_CPP  
**Branch:** `codex/helper10-99d6-bounded-scout`  
**Baseline Git Commit:** `2d5c59a486a3f966d62bfae6b305c77dbcf17541`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  

---

## 1. Executive Summary

Milestone M15.258 accomplished two major objectives:
1. **Debug Test Harness Triage & 100% CTest Recovery Across 4 Configurations:**
   - Forensic analysis proved that production helper source code is 100% sound and frozen (`28b6617d...`, 196 active helpers).
   - Resolved test mock defects in `tests/m68k_005670_instruction_step_test.cpp`, `tests/m68k_00557a_instruction_step_test.cpp`, and `tests/m68k_008dd4_instruction_step_test.cpp`.
   - Restored 100% green pass rate across all 4 build configurations:
     * Linux Debug: 295/295 (100%)
     * Linux Release with Assertions (`-UNDEBUG`): 295/295 (100%)
     * Windows Debug (MSVC): 295/295 (100%)
     * Windows Release (MSVC): 295/295 (100%)
2. **Safe Computational Island Discovery & Safety Gate Enforcement:**
   - Dissected parent routine `0x00D406` for safe subregions:
     * Subregion 1 (`0x00D44E..0x00D464`, 4-iteration coordinate shift/add loop): entered 14 times total across the replay (560 dynamic instructions total). Net ROI < 0.001% (far below 1.5% measurement noise floor); dispatch transition overhead exceeds computational savings. Disqualified by ROI gate.
     * Subregion 2 (`0x00D47C..0x00D482`, RAM zeroing loop): 140 dynamic instructions total. Transition overhead dwarfs benefit.
     * Subregion 3 (`0x00D56C..0x00D5C2`, sprite/tile math): calls external subroutines `0x00D7B0` and `0x00D74A`, coupled with VDP DMA queue setup.
   - Evaluated top Musashi residual hotspots across all 6 scenarios:
     * `0x0032E8` (1.50M steps) & `0x002EE2` (715K steps): hardware spin-wait loops polling `$00FF1658`. Disqualified by safety gate.
     * `0x0037D2` (439K steps): VDP DMA command queue processor with interrupt suppression (`ORI.W #$0700, SR`), polling `$00FF164D`, and VDP MMIO writes. Disqualified by safety gate.
     * `0x0254EA` (518K steps): entity action state dispatcher; non-leaf routine calling 7 unmigrated subroutines (`0x00E398`, `0x00E2A2`, `0x00E268`, `0x00E2D4`, `0x00DA72`, `0x00E682`, `0x00E7E8`). Disqualified by call graph dependency gate.
     * `0x00A4C8` (255K steps): tilemap coordinate calculation; entangled CFG with non-local cross-boundary jumps into `0x00A9A0` and `0x00AD76`. Disqualified by CFG isolation gate.
     * `0x00D7C0` (228K steps): VDP command generator; contains privileged SR manipulation and direct VDP MMIO writes. Disqualified by safety gate.
     * `0x003820` (2.34M steps): multi-frame Nemesis graphics decompressor; roadmap quarantine strictly preserved.
     * `0x00C8AA` (28 instructions, 322 dynamic steps): false hotspot attribution ending in dynamic `jmp (a0)`. Disqualified by ROI gate.
   - Enforced Safety Gate: `SELECTED_KERNEL = NONE_DISQUALIFIED_BY_SAFETY_GATE`.
3. **Dedicated Safety Test Suite & Replay Qualification:**
   - Authored `tests/m15_258_computational_islands_safety_test.cpp`: 12/12 passing under ASan/UBSan.
   - Published Checkpoint Manifest: 37 checkpoints across 6 scenarios verified bit-exact (18 distinct hashes reflecting deterministic branch prefixes).
   - Replayed 78 sessions across 13 configurations (`RUN_A`..`RUN_M`) and 6 scenarios: 624/624 raw stream files bit-exact, 37 checkpoints bit-exact.
4. **180-Process Real OS Process Wall-Clock Benchmark:**
   - Executed 10 paired trials across 3 configs (`RUN_G`, `RUN_L`, `RUN_M`) and 6 scenarios (180 measured OS processes) on core-pinned CPUs:
     * Combined median wall: `RUN_G` = 99.5846 s, `RUN_L` = 101.5660 s, `RUN_M` = 101.0607 s.
     * Speedup factor `RUN_G -> RUN_M`: **0.9854x** (CI95: $\pm 7.17\%$).

---

## 2. Checkpoint Manifest (37 Checkpoints Across 6 Scenarios)

| Index | Scenario | Frame | Checkpoint SHA256 | Verified |
|---|---|---|---|---|
| 01 | scenario_c | 300 | `5941d5190fbe2d4e845cba2ddacf97d1df529ab4174780d8212af26f824b4123` | Bit-Exact |
| 02 | scenario_c | 600 | `08f1d1c320325d992509c47c9dcf21d7820e86013c61e7131d837116ccc0807a` | Bit-Exact |
| 03 | scenario_c | 900 | `0df301614063256e564b528cf30d28f4e32e3c9a9d1201966383ed5258df49b9` | Bit-Exact |
| 04 | scenario_c | 1200 | `2f8cdcf0594b17fbb0455a7de6f3f9256a9cf1075cbddda6e841be849f3659a1` | Bit-Exact |
| 05 | scenario_c | 1500 | `fb220f78d9251b92e047190424a02bc6eaf01576fe0bf45062d6925dd48a6617` | Bit-Exact |
| 06 | scenario_c | 1800 | `adfe0418dd835514a377dbf420168fdd28d0b10d3be2001248be7cfdf3239295` | Bit-Exact |
| 07 | scenario_a | 300 | `08ac74bb96164b05f23c923abf3a06ff6e9a80178a16c7765ed4db6e9ab5ac64` | Bit-Exact |
| 08 | scenario_a | 600 | `9592ef46f327af404f74691eb2259860c677dafe0757c6c8723f09d7bbcf8d24` | Bit-Exact |
| 09 | scenario_a | 900 | `68f57604e4c28e8d96b42fdfa6595089c698f311555cc1ac6bb6bf3eda4b7e8b` | Bit-Exact |
| 10 | scenario_a | 1200 | `f33205d2090dd3dac026f0123d598a6325cbad329fbb609545510d648dc9e505` | Bit-Exact |
| 11 | scenario_a | 1500 | `3a80c9f81a541849bf70f12d6a17a36b8cf109d882e39b33145172834bf72d52` | Bit-Exact |
| 12 | scenario_b | 300 | `5941d5190fbe2d4e845cba2ddacf97d1df529ab4174780d8212af26f824b4123` | Bit-Exact |
| 13 | scenario_b | 600 | `08f1d1c320325d992509c47c9dcf21d7820e86013c61e7131d837116ccc0807a` | Bit-Exact |
| 14 | longrun_3600f | 300 | `5941d5190fbe2d4e845cba2ddacf97d1df529ab4174780d8212af26f824b4123` | Bit-Exact |
| 15 | longrun_3600f | 600 | `08f1d1c320325d992509c47c9dcf21d7820e86013c61e7131d837116ccc0807a` | Bit-Exact |
| 16 | longrun_3600f | 900 | `0df301614063256e564b528cf30d28f4e32e3c9a9d1201966383ed5258df49b9` | Bit-Exact |
| 17 | longrun_3600f | 1200 | `2f8cdcf0594b17fbb0455a7de6f3f9256a9cf1075cbddda6e841be849f3659a1` | Bit-Exact |
| 18 | longrun_3600f | 1500 | `fb220f78d9251b92e047190424a02bc6eaf01576fe0bf45062d6925dd48a6617` | Bit-Exact |
| 19 | longrun_3600f | 1800 | `adfe0418dd835514a377dbf420168fdd28d0b10d3be2001248be7cfdf3239295` | Bit-Exact |
| 20 | longrun_3600f | 2100 | `e6e6c58c42710077a63043b2b2f837affa5278be22976a4f82316ed90074a24f` | Bit-Exact |
| 21 | longrun_3600f | 2400 | `83cd7babed5163fc77995a2af0ea0a46455e12cb72ce7b11084a5df7c684631d` | Bit-Exact |
| 22 | longrun_3600f | 2700 | `07ad92a2c8f78ff96e63aca25afd5a8af0812ee2164935650e5b9ab69cf88b11` | Bit-Exact |
| 23 | longrun_3600f | 3000 | `d63eed185da5fa9c87d0355ab53bcb6c560b3b7de7718cd7778bbab0a631670c` | Bit-Exact |
| 24 | longrun_3600f | 3300 | `c6c2a78d9efc48b501b11cb0042ed5897ac5da203bb4a030196604ffc89e9c25` | Bit-Exact |
| 25 | longrun_3600f | 3600 | `b399154f4baecc297eafdddf450c33866fc1f33824551594f2a7dd30cef35567` | Bit-Exact |
| 26 | longrun_alt_1800f | 300 | `08ac74bb96164b05f23c923abf3a06ff6e9a80178a16c7765ed4db6e9ab5ac64` | Bit-Exact |
| 27 | longrun_alt_1800f | 600 | `9592ef46f327af404f74691eb2259860c677dafe0757c6c8723f09d7bbcf8d24` | Bit-Exact |
| 28 | longrun_alt_1800f | 900 | `68f57604e4c28e8d96b42fdfa6595089c698f311555cc1ac6bb6bf3eda4b7e8b` | Bit-Exact |
| 29 | longrun_alt_1800f | 1200 | `f33205d2090dd3dac026f0123d598a6325cbad329fbb609545510d648dc9e505` | Bit-Exact |
| 30 | longrun_alt_1800f | 1500 | `3a80c9f81a541849bf70f12d6a17a36b8cf109d882e39b33145172834bf72d52` | Bit-Exact |
| 31 | longrun_alt_1800f | 1800 | `c03f00d97bd488ae968fc39685b3db87a74f213c895001d70f42ebbafed42dc9` | Bit-Exact |
| 32 | longrun_combat_1800f | 300 | `5941d5190fbe2d4e845cba2ddacf97d1df529ab4174780d8212af26f824b4123` | Bit-Exact |
| 33 | longrun_combat_1800f | 600 | `08f1d1c320325d992509c47c9dcf21d7820e86013c61e7131d837116ccc0807a` | Bit-Exact |
| 34 | longrun_combat_1800f | 900 | `0df301614063256e564b528cf30d28f4e32e3c9a9d1201966383ed5258df49b9` | Bit-Exact |
| 35 | longrun_combat_1800f | 1200 | `2f8cdcf0594b17fbb0455a7de6f3f9256a9cf1075cbddda6e841be849f3659a1` | Bit-Exact |
| 36 | longrun_combat_1800f | 1500 | `fb220f78d9251b92e047190424a02bc6eaf01576fe0bf45062d6925dd48a6617` | Bit-Exact |
| 37 | longrun_combat_1800f | 1800 | `adfe0418dd835514a377dbf420168fdd28d0b10d3be2001248be7cfdf3239295` | Bit-Exact |
