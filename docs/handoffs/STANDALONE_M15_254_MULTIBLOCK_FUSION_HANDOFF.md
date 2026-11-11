# STANDALONE M15.254 MULTI-BLOCK FUSION HANDOFF

## 1. Milestone Summary
- **Track:** `HYBRID_ROM_TO_CPP`
- **Milestone:** `M15.254`
- **Focus:** Real Benchmark Evidence, CTest Failure Triage, and Controlled Multi-Block Fusion Prototype (`m15250m` / `RUN_J`).
- **Production Helpers:** strictly 196 active helpers (`17396fdaf9c566d36ca8317d40bb334f0f3467c73f1aebe0e26a36f8d4951867`).
- **Canonical ROM SHA256:** `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
- **Pinned GPGX Core SHA256:** `ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`.

## 2. Benchmark Provenance Audit
- `tools/benchmark_m15_253_integrated.py` synthetic variance distribution projections (1.0042x, 0.9765x) retracted.
- Real OS process timings captured via `tools/benchmark_m15_254_real.py` and `tools/run_m15_254_multiblock_ab.py`:
  * `RUN_G = 109.06s`
  * `RUN_H = 111.04s`
  * `RUN_I = 110.84s`
  * `RUN_J = 111.26s`
  * Aggregate speedups: `H -> I = 1.0018x`, `G -> I = 0.9839x`, `I -> J = 0.9962x`.
  * Performance Gate Verdict: Speedups lie within the ~1.5% host measurement noise window.

## 3. Controlled Multi-Block Fusion Prototype
- Implemented in `src/tools/thor_ir/exp250_fused_hybrid_adapter.hpp`:
  * Connected CFG edges: Camera Tracker (`0x00A4C8`: `BB_09 -> BB_10 -> BB_11`) and Priority Sort (`0x00A550`: `BB_15 -> BB_16`).
  * Safety checks: `slice_yield_` (cycle budget & pending interrupt) halts chaining before starting a successor block.
  * Replay metrics across 6 scenarios:
    - `MULTI_BLOCK_ENTRIES = 110`
    - `MULTI_BLOCK_CHAINED_BLOCKS = 220` (2.0 blocks/entry)
    - `INTERNAL_CFG_EDGES_TRAVERSED = 126`
    - `GUEST_STEPS = 219,573`
- Bit-exact gameplay: 10 configurations x 6 scenarios = 60 sessions, 480 streams, 31 checkpoints bit-exact.

## 4. CTest Failure Triage
- Linux Release CTest: **294/294 PASS 100% in 57.75s** (0 failures).
- Windows CTest:
  * Missing test binary (`oasis_m68k_00ceea_cmpa_regression`) triaged to untracked capture block `CMakeLists_captures.cmake`.
  * 7 helper unit tests (`00a196`, `00a342`, `00c980`, `00c998`, `008dd4`, `00557a`, `005670`) pass in Release builds (`-DNDEBUG`), but fail in Debug builds due to mock harness state re-entrance assumptions.

## 5. Next Strategic Focus
- Focus on native memory access optimization (direct RAM pointer access for portable routines to bypass virtual function bus call overhead) to achieve net acceleration over RUN_G.
