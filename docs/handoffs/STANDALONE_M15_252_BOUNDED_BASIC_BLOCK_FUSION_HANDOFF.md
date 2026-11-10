# Standalone Track Handoff — M15.252 Bounded Basic-Block Fusion and Dispatch Amortization

> **Scope & Repository Hygiene Certification:** `C:/Github/Sega-Thor-Standalone` was kept strictly read-only and unmodified throughout M15.252. This handoff provides the standalone engineering track with verified architectural findings, basic-block fusion designs, cycle-exact timeslice boundary handling, and performance attribution models.

---

## 1. Architectural Finding: Instruction-by-Instruction Dispatch Overhead vs Bounded Basic-Block Fusion

In M15.251, profiling revealed that single-instruction hybrid stepping (`step_one_exp250_dispatch_ir`) incurred ~12.5 ns of dispatch overhead per guest instruction, resulting in `0.9724x` throughput despite 1.39x isolated code acceleration.

### The Solution: `HybridM68kFusedBus250` & `run_exp250_mode_b_all`
- Instead of returning to the Musashi / GPGX interpreter loop after every single M68000 instruction, execution stays within native C++ for the entire basic block.
- **Dynamic wrapper entries:** Reduced from 219,573 to 90,416 (**-58.82%** dispatch overhead reduction).
- **Average amortized block length:** 2.43 guest instructions per dispatch entry.
- **Isolated microbenchmark speedup:** **1.40x** (160.8 ms vs 115.3 ms across 10 trials).

---

## 2. Cycle-Exact Timeslice Boundary Yielding (`slice_yield_`)

A critical challenge discovered during multi-instruction fusion was slice expiration:
- If a basic block executes across a GPGX timeslice boundary without checking scheduler state, it may execute 1 instruction past the frame slice boundary, disturbing fine-grained cycle alignment in edge cases.
- **Solution:** In `HybridM68kFusedBus250::begin_insn`, for instructions after the first in a block:
  ```cpp
  if (insn_idx_ > 0) {
      uint32_t cur_cyc = api.read_scheduler_field(0);
      uint32_t end_cyc = api.read_scheduler_field(1);
      if (cur_cyc >= end_cyc) {
          slice_yield_ = true;
          return; // Yield to outer GPGX scheduling loop
      }
  }
  ```
- This guarantees **100% bit-exact stream parity across all 9 configurations and 6 scenarios (432 stream files)**, matching canonical checkpoints bit-for-bit while executing full basic blocks natively when cycles remain in the slice.

---

## 3. Implications for the Standalone C++20 Native Engine

1. **No Guest Stepping Boundaries in Standalone:**
   - In the pure standalone C++20 engine, there is no Musashi emulator loop. Subsystems (Camera, Physics, Entity Management, Palette Animation) can be executed as standard high-level C++ functions.
2. **Whole-Routine Fusion:**
   - Where the hybrid runner fuses instructions into basic blocks, Standalone can fuse entire routines (e.g., executing the entire 128-byte enemy entity substate loop or all 21 active entity priority sort passes in native C++).
   - This completely eliminates dispatch overhead and achieves direct native execution speed.
