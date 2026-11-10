#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b.hpp"
namespace oasis::recompiler::ir {
inline PortableExecutionResult run_exp_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {
    if (!is_compiled_exp_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }
    if (!step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;
  }
  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }
  else if (env.yielded || (env.pc != stop_pc && !is_compiled_exp_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;
}
inline PortableExecutionResult run_exp_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  return run_exp_dispatch_mode_b_ir(env, entry_pc, stop_pc);
}
} // namespace oasis::recompiler::ir
