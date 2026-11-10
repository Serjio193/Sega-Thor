#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_mode_b_p0.hpp"
namespace oasis::recompiler::ir {
inline bool step_exp247_mb_block(PortableIrBus247& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {
  bool handled = false;
  if (!handled && pc >= 0x005CD8U && pc <= 0x007A0EU) handled = run_exp247_mb_chunk_0(bus, pc, stop_pc, next_pc);
  return handled || step_one_exp247_dispatch_ir(bus, pc, next_pc);
}
inline PortableExecutionResult finalize_exp247_res(PortableIrBus247& bus, std::uint32_t stop_pc) {
  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = bus.div_zero ? 5U : (bus.env.address_error ? 3U : 2U); }
  else if (bus.env.yielded || (bus.env.pc != stop_pc && !is_compiled_exp247_dispatch_pc(bus.env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  for (int i = 0; i < 8; ++i) { res.d[i] = bus.env.d_regs[i]; res.a[i] = bus.env.a_regs[i]; }
  res.pc = bus.env.pc; res.ccr = bus.env.read_ccr(); res.cycles = bus.env.total_cycles; res.writes = bus.writes;
  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;
}
inline PortableExecutionResult run_exp247_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus247 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {
    if (!is_compiled_exp247_dispatch_pc(env.pc)) break;
    if (!step_exp247_mb_block(bus, env.pc, stop_pc, next_pc)) break;
  }
  return finalize_exp247_res(bus, stop_pc);
}
} // namespace oasis::recompiler::ir
