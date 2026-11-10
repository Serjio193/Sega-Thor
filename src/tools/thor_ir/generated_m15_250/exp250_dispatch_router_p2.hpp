#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part5.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp250_dispatch_ir_p2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x02E8ECU: return step_ir_0x02E8EC(bus, out_pc);
    case 0x02E8EEU: return step_ir_0x02E8EE(bus, out_pc);
    case 0x02E8F4U: return step_ir_0x02E8F4(bus, out_pc);
    case 0x02E8F6U: return step_ir_0x02E8F6(bus, out_pc);
    case 0x02E8FCU: return step_ir_0x02E8FC(bus, out_pc);
    case 0x02E8FEU: return step_ir_0x02E8FE(bus, out_pc);
    case 0x02E902U: return step_ir_0x02E902(bus, out_pc);
    case 0x02E904U: return step_ir_0x02E904(bus, out_pc);
    case 0x02E908U: return step_ir_0x02E908(bus, out_pc);
    case 0x02E90CU: return step_ir_0x02E90C(bus, out_pc);
    case 0x02EC18U: return step_ir_0x02EC18(bus, out_pc);
    case 0x02EC20U: return step_ir_0x02EC20(bus, out_pc);
    case 0x02EC24U: return step_ir_0x02EC24(bus, out_pc);
    case 0x02EC2CU: return step_ir_0x02EC2C(bus, out_pc);
    case 0x02EC30U: return step_ir_0x02EC30(bus, out_pc);
    case 0x02EC38U: return step_ir_0x02EC38(bus, out_pc);
    case 0x02EC3CU: return step_ir_0x02EC3C(bus, out_pc);
    case 0x02EC40U: return step_ir_0x02EC40(bus, out_pc);
    case 0x02EC48U: return step_ir_0x02EC48(bus, out_pc);
    case 0x02EC4CU: return step_ir_0x02EC4C(bus, out_pc);
    case 0x02EC54U: return step_ir_0x02EC54(bus, out_pc);
    default: return false;
  }
}
inline bool is_compiled_exp250_dispatch_pc_p2(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x02E8ECU:
    case 0x02E8EEU:
    case 0x02E8F4U:
    case 0x02E8F6U:
    case 0x02E8FCU:
    case 0x02E8FEU:
    case 0x02E902U:
    case 0x02E904U:
    case 0x02E908U:
    case 0x02E90CU:
    case 0x02EC18U:
    case 0x02EC20U:
    case 0x02EC24U:
    case 0x02EC2CU:
    case 0x02EC30U:
    case 0x02EC38U:
    case 0x02EC3CU:
    case 0x02EC40U:
    case 0x02EC48U:
    case 0x02EC4CU:
    case 0x02EC54U:
      return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
