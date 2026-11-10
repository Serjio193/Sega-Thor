#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp247_mb_p1(Bus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x005F8AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x005F8A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x005F8E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0060B2U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060B2(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0060B4U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060B4(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060B8(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0060BAU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060BA(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060BE(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0060C0U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060C0(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060C4(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0060C6U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0060C6(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0066C4U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0066C4(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006770U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006770(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006844U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006844(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006848(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00684C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006850(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006854(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006858U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006858(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00685E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006862U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006862(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006868(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00686C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00686EU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00686E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006874(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006908U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006908(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00690C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006912(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006918(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00691E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006922(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006928(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00692E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006934(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00693AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00693A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x006C3AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006C3A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x006C40(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
