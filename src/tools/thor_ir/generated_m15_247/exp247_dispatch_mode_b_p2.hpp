#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp247_mb_p2(Bus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x006C44U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006C44(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006D58U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006D58(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006D5E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006D64(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006F82U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006F82(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006F88(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006FB6U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006FB6(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006FBA(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006FBE(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006FC2(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00739AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00739A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00739C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073A0(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0073A2U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073A2(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073A6(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0073A8U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073A8(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073AA(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073B0(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073B4(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073B8(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073BA(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073BC(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073C0(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0073C6U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073C6(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0073CCU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073CC(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073D2(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0073D6U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0073D6(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0074FCU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0074FC(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007500(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007504(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00750AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00750A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00750E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007512(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00751A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00751E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00792AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00792A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007930(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007936(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007938U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007938(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007942(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007944(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007946(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00794AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00794A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00794C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007956(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
