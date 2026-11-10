#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p0.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p1.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p2.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p3.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b_p4.hpp"
namespace oasis::recompiler::ir {
inline PortableExecutionResult run_exp_dispatch_mode_b_ir(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {
    if (!is_compiled_exp_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }
    bool handled = false;
    if (env.pc >= 0x005E78U && env.pc <= 0x0066C2U) handled = run_exp_mb_chunk_0(bus, env.pc, stop_pc, next_pc);
    if (env.pc >= 0x0066CEU && env.pc <= 0x006F28U) handled = run_exp_mb_chunk_1(bus, env.pc, stop_pc, next_pc);
    if (env.pc >= 0x006F2CU && env.pc <= 0x00770CU) handled = run_exp_mb_chunk_2(bus, env.pc, stop_pc, next_pc);
    if (env.pc >= 0x007740U && env.pc <= 0x007E70U) handled = run_exp_mb_chunk_3(bus, env.pc, stop_pc, next_pc);
    if (env.pc >= 0x007E78U && env.pc <= 0x007E96U) handled = run_exp_mb_chunk_4(bus, env.pc, stop_pc, next_pc);
    if (!handled) step_one_exp_dispatch_ir(bus, env.pc, next_pc);
  }
  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }
  else if (env.yielded || (env.pc != stop_pc && !is_compiled_exp_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;
}
} // namespace oasis::recompiler::ir
