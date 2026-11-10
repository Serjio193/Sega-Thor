#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp247_mb_p3(Bus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x00795AU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00795A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007960(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007962U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007962(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007968U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007968(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x00796CU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x00796C(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007970U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007970(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0079EEU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0079EE(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0079F4(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0079FAU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0079FA(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x0079FCU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x0079FC(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007A00U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007A00(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007A04(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007A06U: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007A06(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007A0A(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    case 0x007A0EU: {
      if (bus.env().pc == stop_pc) { out_pc = bus.env().pc; return true; }
      if (!step_ir_0x007A0E(bus, out_pc)) return false;
      if (bus.env().yielded || bus.faulted()) return true;
      return true;
    }
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
