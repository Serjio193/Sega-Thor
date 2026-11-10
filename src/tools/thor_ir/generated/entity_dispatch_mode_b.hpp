#pragma once
#include "tools/thor_ir/generated/entity_dispatch_router.hpp"
namespace oasis::recompiler::ir {
inline PortableExecutionResult run_entity_dispatch_mode_b_ir(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;
  std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 4096) {
    if (!is_compiled_entity_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }
    switch (env.pc) {
    case 0x0059A8U: {
      if (!step_ir_0x0059A8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x0059AE(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x0059B2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x0059B6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x0059B8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C1AU: {
      if (!step_ir_0x005C1A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C1C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C1E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C20U: {
      if (!step_ir_0x005C20(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C24(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C2C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C34(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C3A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C3E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C42(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C46(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C4C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C52(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C56(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C5A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C5E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C62(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C68(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C6E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C74(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C7AU: {
      if (!step_ir_0x005C7A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C80(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C82U: {
      if (!step_ir_0x005C82(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C88(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C8AU: {
      if (!step_ir_0x005C8A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C92(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005C94U: {
      if (!step_ir_0x005C94(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C9A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005C9C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CA0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CA2U: {
      if (!step_ir_0x005CA2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CA6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CAC(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CB2U: {
      if (!step_ir_0x005CB2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CB4U: {
      if (!step_ir_0x005CB4(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CBA(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CC0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CC6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CC8U: {
      if (!step_ir_0x005CC8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CCC(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CD0U: {
      if (!step_ir_0x005CD0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CD6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CDC(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CE0U: {
      if (!step_ir_0x005CE0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CE6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CEA(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CEE(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CF2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CF6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005CF8U: {
      if (!step_ir_0x005CF8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005CFE(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D04(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D06U: {
      if (!step_ir_0x005D06(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D0A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D10(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D16U: {
      if (!step_ir_0x005D16(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D1C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D1EU: {
      if (!step_ir_0x005D1E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D22(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D28(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D2EU: {
      if (!step_ir_0x005D2E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D32(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D38(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D3AU: {
      if (!step_ir_0x005D3A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D3CU: {
      if (!step_ir_0x005D3C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D40(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D44(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D46U: {
      if (!step_ir_0x005D46(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D4AU: {
      if (!step_ir_0x005D4A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D4E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D52(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D56(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D5A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D5CU: {
      if (!step_ir_0x005D5C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D60(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D66(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D6C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D72(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D74U: {
      if (!step_ir_0x005D74(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D78(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D7AU: {
      if (!step_ir_0x005D7A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D7E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D82(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D88(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D8E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D90U: {
      if (!step_ir_0x005D90(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D96U: {
      if (!step_ir_0x005D96(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005D9C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005D9EU: {
      if (!step_ir_0x005D9E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DA4U: {
      if (!step_ir_0x005DA4(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DA6U: {
      if (!step_ir_0x005DA6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DAA(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DB0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DB6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DB8U: {
      if (!step_ir_0x005DB8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DBE(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DC0U: {
      if (!step_ir_0x005DC0(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DC6(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DC8U: {
      if (!step_ir_0x005DC8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DCE(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DD2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DD4U: {
      if (!step_ir_0x005DD4(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DD8(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x005DDC(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x005DE2U: {
      if (!step_ir_0x005DE2(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x00780EU: {
      if (!step_ir_0x00780E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007810(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007814(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007818(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x00781AU: {
      if (!step_ir_0x00781A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x00781C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007820(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x007822U: {
      if (!step_ir_0x007822(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007824(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007828(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x00782AU: {
      if (!step_ir_0x00782A(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x00782CU: {
      if (!step_ir_0x00782C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007830(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007832(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x007836U: {
      if (!step_ir_0x007836(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x00783C(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x007842(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x007844U: {
      if (!step_ir_0x007844(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    case 0x007846U: {
      if (!step_ir_0x007846(bus, next_pc)) break; if (next_pc == stop_pc) break;
      if (!step_ir_0x00784E(bus, next_pc)) break; if (next_pc == stop_pc) break;
      break; }
    default: if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break; break; }
  }
  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }
  else if (env.yielded || (env.pc != stop_pc && !is_compiled_entity_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;
  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;
}
} // namespace oasis::recompiler::ir
