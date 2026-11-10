#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_mode_b_p0.hpp"
namespace oasis::recompiler::ir {
inline bool step_exp249_mb_block(PortableIrBus249& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {
  bool handled = false;
  if (!handled && pc >= 0x00E0B8U && pc <= 0x0255C4U) handled = run_exp249_mb_chunk_0(bus, pc, stop_pc, next_pc);
  return handled || step_one_exp249_dispatch_ir(bus, pc, next_pc);
}
inline PortableExecutionResult finalize_exp249_res(PortableIrBus249& bus, std::uint32_t stop_pc) {
  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = bus.div_zero ? 5U : (bus.env.address_error ? 3U : 2U); }
  else if (bus.env.yielded || (bus.env.pc != stop_pc && !is_compiled_exp249_dispatch_pc(bus.env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  for (int i = 0; i < 8; ++i) { res.d[i] = bus.env.d_regs[i]; res.a[i] = bus.env.a_regs[i]; }
  res.pc = bus.env.pc; res.ccr = bus.env.read_ccr(); res.cycles = bus.env.total_cycles; res.writes = bus.writes;
  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;
}
inline PortableExecutionResult run_exp249_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus249 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {
    if (!is_compiled_exp249_dispatch_pc(env.pc)) break;
    if (!step_exp249_mb_block(bus, env.pc, stop_pc, next_pc)) break;
  }
  return finalize_exp249_res(bus, stop_pc);
}
} // namespace oasis::recompiler::ir
