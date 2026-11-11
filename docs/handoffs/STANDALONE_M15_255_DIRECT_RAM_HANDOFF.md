# STANDALONE HANDOFF: M15.255 DIRECT WORK RAM FAST-PATH & MEMORY FORENSICS

**Milestone:** SEGA-THOR — M15.255
**Track:** HYBRID_ROM_TO_CPP
**Date:** 2026-11-11
**Baseline Commit:** `b63703e4b7ffd887b36a72f5f91af79bca7eca46`
**Standalone Repository:** `C:/Github/Sega-Thor-Standalone` (STRICTLY READ-ONLY / UNTOUCHED)
**Target:** Direct Work RAM fast-path qualification, physical memory interleaving rules, dynamic profiling, and virtual bus bottleneck forensics.

---

## 1. Executive Summary & Architectural Verdict

M15.255 investigated whether virtual memory bus callbacks are the primary bottleneck preventing portable compiled guest code from delivering integrated gameplay speedup.

### Key Conclusions:
1. **Virtual Bus Overhead is Negligible in Integrated Gameplay:**
   - Isolated microbenchmarks confirm direct RAM host pointers are **1.79x faster** than bus callbacks (0.66 ns vs 1.17 ns per operation).
   - However, in 60 FPS gameplay, total memory operations across M15.250 hotpaths account for only **~0.05 ms per frame** out of a 16.6 ms budget.
   - 10 real OS process benchmark trials of RUN_J (multi-block fusion with virtual bus) vs RUN_K (multi-block fusion with direct Work RAM) showed a delta of -0.0248s (-0.16%), well within the 1.5% measurement noise threshold.
   - **Verdict:** Virtual bus callbacks are **NOT** the bottleneck. The dominant bottleneck is the residual uncompiled Musashi routines (68+ million guest instructions per gameplay session). Acceleration requires expanding static decompilation coverage.

2. **GPGX Physical Memory Architecture Discovery:**
   - Under x86/x64 little-endian hosts (`LSB_FIRST`), GPGX Work RAM is byte-interleaved.
   - Byte accesses require an XOR mask: `base[addr ^ 1]`.
   - 16-bit word dereferences match host little-endian order: `*(uint16_t*)(base + (addr & 0xFFFF))`.
   - 32-bit longwords require two sequential 16-bit word dereferences: `(read_16(addr) << 16) | read_16(addr + 2)`. Direct 32-bit host loads swap the high and low words!

---

## 2. Pinned GPGX Memory Layout & Direct-RAM Adapter Contract

### Canonical Address Invariants
- Genesis Work RAM spans `0xFF0000..0xFFFFFF` (64 KB).
- Genesis RAM mirrors span `0xE00000..0xFEFFFF`. Profiling confirmed Beyond Oasis performs **0 accesses** to mirror addresses during gameplay.
- Any access outside `0xFF0000..0xFFFFFF` (ROM, VDP, Z80, MMIO) is rejected by the direct RAM fast-path and routes via the original bus callback.

### Adapter Implementation (`HybridM68kDirectRamBus250`)
Located in `src/tools/thor_ir/exp250_direct_ram_adapter.hpp`:
```cpp
// Direct RAM Read Byte (XOR interleaved for LSB_FIRST)
uint8_t read_byte(uint32_t addr) {
    if ((addr & 0xFF0000U) == 0xFF0000U && ram_base_ != nullptr) {
        return ram_base_[(addr & 0xFFFFU) ^ 1U];
    }
    return api_.read_byte(ctx_, addr);
}

// Direct RAM Read Word (native 16-bit little endian)
uint16_t read_word(uint32_t addr) {
    if ((addr & 0xFF0000U) == 0xFF0000U && ram_base_ != nullptr && (addr & 1U) == 0) {
        uint16_t val;
        std::memcpy(&val, ram_base_ + (addr & 0xFFFFU), sizeof(uint16_t));
        return val;
    }
    return api_.read_word(ctx_, addr);
}
```

---

## 3. Dynamic Memory Access Profiling Across Scenarios

Dynamic accounting across all 6 test scenarios (15,000+ total frames) revealed:
- **Total Dynamic Hotpath Memory Ops:** 36,187 operations.
- **Fast-Path Eligibility:** 99.8% of hotpath memory accesses target canonical Work RAM.
- **Read / Write Breakdown:**
  - 16-bit Word Reads: 64.2%
  - 16-bit Word Writes: 22.8%
  - 8-bit Byte Reads: 9.1%
  - 32-bit Long Operations: 3.9%
- **MMIO / VDP in Hotpaths:** 0 accesses (all I/O handled outside compiled clusters).

---

## 4. Benchmark Results (10 Real OS Process Trials)

Scenario A (1,592 frames):
- **RUN_G (Interpreter Baseline):** Median 15.0307s (min 14.9380s, max 16.8126s)
- **RUN_I (Basic-Block Fusion):** Median 15.0526s (min 14.9303s, max 15.2956s)
- **RUN_J (Multi-Block Fusion):** Median 15.1076s (min 14.9945s, max 15.3095s)
- **RUN_K (Multi-Block + Direct RAM):** Median 15.1324s (min 14.9727s, max 15.6634s)
- **Speedup (RUN_J -> RUN_K):** 0.9984x (Delta: -0.0248s; 0.16% variance)

---

## 5. Qualification & Invariant Verification

- **Production Helper Freeze:** Active production source hash matches baseline exactly (`28b6617d...`). Active helpers: 196 (`17396fda...`).
- **Bit-Exact Replay Streams:** 66/66 sessions, 528/528 raw streams bit-exact across RUN_A through RUN_K.
- **Checkpoints:** 31/31 checkpoints bit-exact.
- **Regression Tests:** 296/296 Windows CTest passed; 295/295 Linux CTest passed (with assertions enabled).
- **Differential & Mutation Tests:** 4/4 direct-RAM tests passed under ASan/UBSan.
- **Receipt Verifier:** Verified with 0 rejections (`combined-clusters-qualification-m15-255-20261111.json`).
