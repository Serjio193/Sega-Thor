#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_part2.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp256_dispatch_ir_p0(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x006B12U: return step_ir_0x006B12(bus, out_pc);
    case 0x006B14U: return step_ir_0x006B14(bus, out_pc);
    case 0x006B18U: return step_ir_0x006B18(bus, out_pc);
    case 0x006B1CU: return step_ir_0x006B1C(bus, out_pc);
    case 0x006B1EU: return step_ir_0x006B1E(bus, out_pc);
    case 0x006B20U: return step_ir_0x006B20(bus, out_pc);
    case 0x006B24U: return step_ir_0x006B24(bus, out_pc);
    case 0x006B26U: return step_ir_0x006B26(bus, out_pc);
    case 0x006B28U: return step_ir_0x006B28(bus, out_pc);
    case 0x010F48U: return step_ir_0x010F48(bus, out_pc);
    case 0x010F4CU: return step_ir_0x010F4C(bus, out_pc);
    case 0x010F50U: return step_ir_0x010F50(bus, out_pc);
    case 0x010F54U: return step_ir_0x010F54(bus, out_pc);
    case 0x010F58U: return step_ir_0x010F58(bus, out_pc);
    case 0x010F5AU: return step_ir_0x010F5A(bus, out_pc);
    case 0x010F5CU: return step_ir_0x010F5C(bus, out_pc);
    case 0x010F5EU: return step_ir_0x010F5E(bus, out_pc);
    case 0x010F60U: return step_ir_0x010F60(bus, out_pc);
    case 0x010F62U: return step_ir_0x010F62(bus, out_pc);
    case 0x010F64U: return step_ir_0x010F64(bus, out_pc);
    case 0x010F66U: return step_ir_0x010F66(bus, out_pc);
    case 0x010F68U: return step_ir_0x010F68(bus, out_pc);
    case 0x010F6AU: return step_ir_0x010F6A(bus, out_pc);
    case 0x010F6CU: return step_ir_0x010F6C(bus, out_pc);
    case 0x010F6EU: return step_ir_0x010F6E(bus, out_pc);
    case 0x010F70U: return step_ir_0x010F70(bus, out_pc);
    case 0x010F72U: return step_ir_0x010F72(bus, out_pc);
    case 0x010F74U: return step_ir_0x010F74(bus, out_pc);
    case 0x010F76U: return step_ir_0x010F76(bus, out_pc);
    case 0x010F78U: return step_ir_0x010F78(bus, out_pc);
    case 0x010F7AU: return step_ir_0x010F7A(bus, out_pc);
    case 0x010F7CU: return step_ir_0x010F7C(bus, out_pc);
    case 0x010F7EU: return step_ir_0x010F7E(bus, out_pc);
    case 0x010F82U: return step_ir_0x010F82(bus, out_pc);
    case 0x0112CCU: return step_ir_0x0112CC(bus, out_pc);
    case 0x0112D0U: return step_ir_0x0112D0(bus, out_pc);
    case 0x0112D6U: return step_ir_0x0112D6(bus, out_pc);
    case 0x0112D8U: return step_ir_0x0112D8(bus, out_pc);
    case 0x0112DCU: return step_ir_0x0112DC(bus, out_pc);
    case 0x0112DEU: return step_ir_0x0112DE(bus, out_pc);
    case 0x0112E2U: return step_ir_0x0112E2(bus, out_pc);
    case 0x0112E6U: return step_ir_0x0112E6(bus, out_pc);
    case 0x0112E8U: return step_ir_0x0112E8(bus, out_pc);
    case 0x0112ECU: return step_ir_0x0112EC(bus, out_pc);
    case 0x0141CAU: return step_ir_0x0141CA(bus, out_pc);
    case 0x0141CEU: return step_ir_0x0141CE(bus, out_pc);
    case 0x0141D0U: return step_ir_0x0141D0(bus, out_pc);
    case 0x0141D4U: return step_ir_0x0141D4(bus, out_pc);
    case 0x0141D8U: return step_ir_0x0141D8(bus, out_pc);
    case 0x0141DCU: return step_ir_0x0141DC(bus, out_pc);
    case 0x0141E0U: return step_ir_0x0141E0(bus, out_pc);
    case 0x0141E4U: return step_ir_0x0141E4(bus, out_pc);
    case 0x0141E8U: return step_ir_0x0141E8(bus, out_pc);
    case 0x0141ECU: return step_ir_0x0141EC(bus, out_pc);
    case 0x0141F0U: return step_ir_0x0141F0(bus, out_pc);
    case 0x0141F2U: return step_ir_0x0141F2(bus, out_pc);
    case 0x0141F4U: return step_ir_0x0141F4(bus, out_pc);
    case 0x0141F6U: return step_ir_0x0141F6(bus, out_pc);
    case 0x0141F8U: return step_ir_0x0141F8(bus, out_pc);
    case 0x0141FAU: return step_ir_0x0141FA(bus, out_pc);
    case 0x0141FCU: return step_ir_0x0141FC(bus, out_pc);
    case 0x0141FEU: return step_ir_0x0141FE(bus, out_pc);
    case 0x0302F4U: return step_ir_0x0302F4(bus, out_pc);
    case 0x0302FAU: return step_ir_0x0302FA(bus, out_pc);
    case 0x0302FCU: return step_ir_0x0302FC(bus, out_pc);
    case 0x030300U: return step_ir_0x030300(bus, out_pc);
    case 0x030304U: return step_ir_0x030304(bus, out_pc);
    case 0x03030AU: return step_ir_0x03030A(bus, out_pc);
    case 0x030310U: return step_ir_0x030310(bus, out_pc);
    case 0x030314U: return step_ir_0x030314(bus, out_pc);
    case 0x03031AU: return step_ir_0x03031A(bus, out_pc);
    case 0x03031EU: return step_ir_0x03031E(bus, out_pc);
    case 0x030322U: return step_ir_0x030322(bus, out_pc);
    default: return false;
  }
}
inline bool is_compiled_exp256_dispatch_pc_p0(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x006B12U:
    case 0x006B14U:
    case 0x006B18U:
    case 0x006B1CU:
    case 0x006B1EU:
    case 0x006B20U:
    case 0x006B24U:
    case 0x006B26U:
    case 0x006B28U:
    case 0x010F48U:
    case 0x010F4CU:
    case 0x010F50U:
    case 0x010F54U:
    case 0x010F58U:
    case 0x010F5AU:
    case 0x010F5CU:
    case 0x010F5EU:
    case 0x010F60U:
    case 0x010F62U:
    case 0x010F64U:
    case 0x010F66U:
    case 0x010F68U:
    case 0x010F6AU:
    case 0x010F6CU:
    case 0x010F6EU:
    case 0x010F70U:
    case 0x010F72U:
    case 0x010F74U:
    case 0x010F76U:
    case 0x010F78U:
    case 0x010F7AU:
    case 0x010F7CU:
    case 0x010F7EU:
    case 0x010F82U:
    case 0x0112CCU:
    case 0x0112D0U:
    case 0x0112D6U:
    case 0x0112D8U:
    case 0x0112DCU:
    case 0x0112DEU:
    case 0x0112E2U:
    case 0x0112E6U:
    case 0x0112E8U:
    case 0x0112ECU:
    case 0x0141CAU:
    case 0x0141CEU:
    case 0x0141D0U:
    case 0x0141D4U:
    case 0x0141D8U:
    case 0x0141DCU:
    case 0x0141E0U:
    case 0x0141E4U:
    case 0x0141E8U:
    case 0x0141ECU:
    case 0x0141F0U:
    case 0x0141F2U:
    case 0x0141F4U:
    case 0x0141F6U:
    case 0x0141F8U:
    case 0x0141FAU:
    case 0x0141FCU:
    case 0x0141FEU:
    case 0x0302F4U:
    case 0x0302FAU:
    case 0x0302FCU:
    case 0x030300U:
    case 0x030304U:
    case 0x03030AU:
    case 0x030310U:
    case 0x030314U:
    case 0x03031AU:
    case 0x03031EU:
    case 0x030322U:
      return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
