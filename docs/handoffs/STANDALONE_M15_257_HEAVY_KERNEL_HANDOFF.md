# STANDALONE M15.257 HEAVY-KERNEL FORENSICS & SAFETY QUARANTINE HANDOFF

**Milestone:** SEGA-THOR — M15.257  
**Track:** HYBRID_ROM_TO_CPP  
**Branch:** `codex/helper10-99d6-bounded-scout`  
**Baseline Git Commit:** `b14306aeb2fa9d35c563c2366db3ed94d95d5b7a`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  

---

## 1. Executive Summary

Milestone M15.257 audited the M15.256 host CPU profiler, eliminated measurement overhead via calibrated invariant TSC timing (-0.73% vs +27.33%), and performed exhaustive disassembly and forensic analysis on candidate heavy kernels at ROM addresses `0x00C8AA` and `0x00D406`.

1. **`0x00C8AA` Disproved as False Hotspot Attribution:**
   `0x00C8AA` was revealed to be a 28-instruction dispatch trampoline (98 bytes) exiting via `JMP (A0)` into room map header pointer tables. Across 11,192 frames, it executed only 14 times, taking 322 dynamic steps and 0.35 ms host CPU. Compiling it is virtually zero ROI.
2. **`0x00D406` Disqualified by Phase 4 Safety Gate:**
   `0x00D406` is a heavy computational engine (2.08M instructions, 2.49s host CPU across 14 scene transitions), but it explicitly calls quarantined Nemesis decompression routine `0x003820`, suppresses interrupts via `ORI.W #$0700, SR`, spin-polls on `$FF164D`, and writes directly to VDP control/data ports. Lowering it into portable C++20 violates the Phase 4 Safety Gate.
3. **Safety Gate Decision:**
   `SELECTED_KERNEL = NONE_DISQUALIFIED_BY_SAFETY_GATE`. Compilation was halted per Phase 4 instructions to prevent hardware corruption. Mode `RUN_M` (`m15257m`) was registered to enforce safety quarantine while running verified M15.256 portable coverage.
4. **Qualification & Benchmarking:**
   - 13 configurations (`RUN_A`..`RUN_M`) qualified across 6 scenarios (78 sessions, 624/624 streams bit-exact, 37 checkpoints bit-exact).
   - 180 independent OS processes benchmarked on core-pinned CPUs: Speedup `RUN_G -> RUN_M` is **0.9945x** (CI95: $\pm 3.77\%$), confirming performance parity within noise floor.

---

## 2. Profiling Overhead Audit & Resolution

| Metric | M15.256 Profiler | M15.257 Invariant TSC Profiler |
|---|---|---|
| Timing Method | POSIX `clock_gettime(CLOCK_PROCESS_CPUTIME_ID)` | Hardware `__rdtsc()` calibrated to monotonic clock |
| Timer Overhead | ~40 ns per sample | ~4.5 ns per event |
| 6-Scenario Baseline Wall | 110.24 s | 111.46 s |
| 6-Scenario Instrumented Wall | 140.37 s | 110.65 s |
| Measured Overhead | **+27.33%** | **-0.73%** (within host OS noise floor) |

### Authoritative Workload Distribution (6 Scenarios, 11,192 Frames)
- **Musashi M68000 Interpreter:** 64.79%
- **Production Native Helpers (196 helpers):** 15.42%
- **Portable Hotpaths (M15.244..M15.256):** 0.86%
- **Runner Overhead & Platform Dispatch:** 17.71%
- **Audio & Z80 Processing:** 1.12%

---

## 3. Heavy-Kernel Forensic Details

### A. Candidate `0x00C8AA` (`map_load_dispatch`)
- **ROM Extent:** `0x00C8AA` — `0x00C90C` (98 bytes, 28 instructions).
- **Exit Instruction:** `JMP (A0)` (indirect jump to target resolved from table at `0x00C92C`).
- **Dynamic Usage:** 14 entries, 322 instructions, 0.354 ms host CPU.
- **Verdict:** Overcount artifact. Do not compile in isolation.

### B. Candidate `0x00D406` (`vdp_nemesis_scene_streamer`)
- **ROM Extent:** `0x00D406` — `0x00D700` / `0x00D7C0` (954 bytes, 273 instructions).
- **Dynamic Usage:** 14 entries, 2,085,176 instructions, 2,497.25 ms host CPU.
- **Fatal Safety Gate Violations:**
  - Calls quarantined Nemesis decompression routine `0x003820` (`0x00D4EE`, `0x00D54A`, `0x00D650`).
  - Manipulates CPU status register: `ORI.W #$0700, SR` (disables interrupts), `ANDI.W #$F9FF, SR` (restores interrupts).
  - Hardware polling: `BTST #0, ($FF164D).L` / `BNE.S ...`.
  - Direct VDP MMIO writes: `$00C00004` (control), `$00C00000` (data).
- **Verdict:** Disqualified by Phase 4 Safety Gate. Preserved in Musashi under hardware quarantine.

---

## 4. Replay Qualification & Real Benchmark Summary

### Thirteen-Configuration Replay Qualification (78 Sessions)
- **Streams Verified:** 624/624 streams bit-exact (8/8 streams per session across `run_a`..`run_m`).
- **Checkpoints Verified:** 37/37 checkpoints bit-exact against baseline `run_a`.
- **Contract Faults:** 0 across all runs.

### 180-Process Real OS Benchmark (Core-Pinned Workers)
- **Combined Median Wall:** `RUN_G` = 100.3801 s, `RUN_L` = 102.3206 s, `RUN_M` = 100.9308 s.
- **Combined Median CPU:** `RUN_G` = 98.9028 s, `RUN_L` = 100.4509 s, `RUN_M` = 100.1942 s.
- **Speedup `RUN_G -> RUN_M`:** 0.9945x (CI95: $\pm 3.77\%$, paired wall saved median: 0.2564 s, paired CPU saved median: -0.1178 s).

---

## 5. Roadmap to Milestone M15.258

1. **Deconstruct Room Initialization Subsystem:**
   Investigate the subroutines indirectly targeted by `JMP (A0)` from `0x00C8AA` to locate pure mathematical or data transformation routines isolated from hardware MMIO.
2. **Decompose Nemesis Streaming Pipeline:**
   Analyze whether software buffer preparation in `0x00D406` can be cleanly decoupled from the actual VDP MMIO transfer and interrupt masking stages.
3. **Maintain Zero-Tolerance Safety Gate:**
   Never compromise deterministic synchronization, interrupt pacing, or hardware quarantines for static instruction metrics.
