#pragma once
#include "tools/thor_ir/generated/entity_dispatch_part0.hpp"
#include "tools/thor_ir/generated/entity_dispatch_part1.hpp"
#include "tools/thor_ir/generated/entity_dispatch_part2.hpp"
#include "tools/thor_ir/generated/entity_dispatch_part3.hpp"
#include "tools/thor_ir/generated/entity_dispatch_part4.hpp"
namespace oasis::recompiler::ir {
inline bool is_compiled_entity_dispatch_pc(std::uint32_t pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x0059A8U: case 0x0059AEU: case 0x0059B2U: case 0x0059B6U: case 0x0059B8U: case 0x005C1AU: case 0x005C1CU: case 0x005C1EU: return true;
  case 0x005C20U: case 0x005C24U: case 0x005C2CU: case 0x005C34U: case 0x005C3AU: case 0x005C3EU: case 0x005C42U: case 0x005C46U: return true;
  case 0x005C4CU: case 0x005C52U: case 0x005C56U: case 0x005C5AU: case 0x005C5EU: case 0x005C62U: case 0x005C68U: case 0x005C6EU: return true;
  case 0x005C74U: case 0x005C7AU: case 0x005C80U: case 0x005C82U: case 0x005C88U: case 0x005C8AU: case 0x005C92U: case 0x005C94U: return true;
  case 0x005C9AU: case 0x005C9CU: case 0x005CA0U: case 0x005CA2U: case 0x005CA6U: case 0x005CACU: case 0x005CB2U: case 0x005CB4U: return true;
  case 0x005CBAU: case 0x005CC0U: case 0x005CC6U: case 0x005CC8U: case 0x005CCCU: case 0x005CD0U: case 0x005CD6U: case 0x005CDCU: return true;
  case 0x005CE0U: case 0x005CE6U: case 0x005CEAU: case 0x005CEEU: case 0x005CF2U: case 0x005CF6U: case 0x005CF8U: case 0x005CFEU: return true;
  case 0x005D04U: case 0x005D06U: case 0x005D0AU: case 0x005D10U: case 0x005D16U: case 0x005D1CU: case 0x005D1EU: case 0x005D22U: return true;
  case 0x005D28U: case 0x005D2EU: case 0x005D32U: case 0x005D38U: case 0x005D3AU: case 0x005D3CU: case 0x005D40U: case 0x005D44U: return true;
  case 0x005D46U: case 0x005D4AU: case 0x005D4EU: case 0x005D52U: case 0x005D56U: case 0x005D5AU: case 0x005D5CU: case 0x005D60U: return true;
  case 0x005D66U: case 0x005D6CU: case 0x005D72U: case 0x005D74U: case 0x005D78U: case 0x005D7AU: case 0x005D7EU: case 0x005D82U: return true;
  case 0x005D88U: case 0x005D8EU: case 0x005D90U: case 0x005D96U: case 0x005D9CU: case 0x005D9EU: case 0x005DA4U: case 0x005DA6U: return true;
  case 0x005DAAU: case 0x005DB0U: case 0x005DB6U: case 0x005DB8U: case 0x005DBEU: case 0x005DC0U: case 0x005DC6U: case 0x005DC8U: return true;
  case 0x005DCEU: case 0x005DD2U: case 0x005DD4U: case 0x005DD8U: case 0x005DDCU: case 0x005DE2U: case 0x00780EU: case 0x007810U: return true;
  case 0x007814U: case 0x007818U: case 0x00781AU: case 0x00781CU: case 0x007820U: case 0x007822U: case 0x007824U: case 0x007828U: return true;
  case 0x00782AU: case 0x00782CU: case 0x007830U: case 0x007832U: case 0x007836U: case 0x00783CU: case 0x007842U: case 0x007844U: return true;
  case 0x007846U: case 0x00784EU: return true;
  default: return false; }
}
template <typename Bus> inline bool step_one_entity_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x0059A8U: return step_ir_0x0059A8(bus, out_pc);
  case 0x0059AEU: return step_ir_0x0059AE(bus, out_pc);
  case 0x0059B2U: return step_ir_0x0059B2(bus, out_pc);
  case 0x0059B6U: return step_ir_0x0059B6(bus, out_pc);
  case 0x0059B8U: return step_ir_0x0059B8(bus, out_pc);
  case 0x005C1AU: return step_ir_0x005C1A(bus, out_pc);
  case 0x005C1CU: return step_ir_0x005C1C(bus, out_pc);
  case 0x005C1EU: return step_ir_0x005C1E(bus, out_pc);
  case 0x005C20U: return step_ir_0x005C20(bus, out_pc);
  case 0x005C24U: return step_ir_0x005C24(bus, out_pc);
  case 0x005C2CU: return step_ir_0x005C2C(bus, out_pc);
  case 0x005C34U: return step_ir_0x005C34(bus, out_pc);
  case 0x005C3AU: return step_ir_0x005C3A(bus, out_pc);
  case 0x005C3EU: return step_ir_0x005C3E(bus, out_pc);
  case 0x005C42U: return step_ir_0x005C42(bus, out_pc);
  case 0x005C46U: return step_ir_0x005C46(bus, out_pc);
  case 0x005C4CU: return step_ir_0x005C4C(bus, out_pc);
  case 0x005C52U: return step_ir_0x005C52(bus, out_pc);
  case 0x005C56U: return step_ir_0x005C56(bus, out_pc);
  case 0x005C5AU: return step_ir_0x005C5A(bus, out_pc);
  case 0x005C5EU: return step_ir_0x005C5E(bus, out_pc);
  case 0x005C62U: return step_ir_0x005C62(bus, out_pc);
  case 0x005C68U: return step_ir_0x005C68(bus, out_pc);
  case 0x005C6EU: return step_ir_0x005C6E(bus, out_pc);
  case 0x005C74U: return step_ir_0x005C74(bus, out_pc);
  case 0x005C7AU: return step_ir_0x005C7A(bus, out_pc);
  case 0x005C80U: return step_ir_0x005C80(bus, out_pc);
  case 0x005C82U: return step_ir_0x005C82(bus, out_pc);
  case 0x005C88U: return step_ir_0x005C88(bus, out_pc);
  case 0x005C8AU: return step_ir_0x005C8A(bus, out_pc);
  case 0x005C92U: return step_ir_0x005C92(bus, out_pc);
  case 0x005C94U: return step_ir_0x005C94(bus, out_pc);
  case 0x005C9AU: return step_ir_0x005C9A(bus, out_pc);
  case 0x005C9CU: return step_ir_0x005C9C(bus, out_pc);
  case 0x005CA0U: return step_ir_0x005CA0(bus, out_pc);
  case 0x005CA2U: return step_ir_0x005CA2(bus, out_pc);
  case 0x005CA6U: return step_ir_0x005CA6(bus, out_pc);
  case 0x005CACU: return step_ir_0x005CAC(bus, out_pc);
  case 0x005CB2U: return step_ir_0x005CB2(bus, out_pc);
  case 0x005CB4U: return step_ir_0x005CB4(bus, out_pc);
  case 0x005CBAU: return step_ir_0x005CBA(bus, out_pc);
  case 0x005CC0U: return step_ir_0x005CC0(bus, out_pc);
  case 0x005CC6U: return step_ir_0x005CC6(bus, out_pc);
  case 0x005CC8U: return step_ir_0x005CC8(bus, out_pc);
  case 0x005CCCU: return step_ir_0x005CCC(bus, out_pc);
  case 0x005CD0U: return step_ir_0x005CD0(bus, out_pc);
  case 0x005CD6U: return step_ir_0x005CD6(bus, out_pc);
  case 0x005CDCU: return step_ir_0x005CDC(bus, out_pc);
  case 0x005CE0U: return step_ir_0x005CE0(bus, out_pc);
  case 0x005CE6U: return step_ir_0x005CE6(bus, out_pc);
  case 0x005CEAU: return step_ir_0x005CEA(bus, out_pc);
  case 0x005CEEU: return step_ir_0x005CEE(bus, out_pc);
  case 0x005CF2U: return step_ir_0x005CF2(bus, out_pc);
  case 0x005CF6U: return step_ir_0x005CF6(bus, out_pc);
  case 0x005CF8U: return step_ir_0x005CF8(bus, out_pc);
  case 0x005CFEU: return step_ir_0x005CFE(bus, out_pc);
  case 0x005D04U: return step_ir_0x005D04(bus, out_pc);
  case 0x005D06U: return step_ir_0x005D06(bus, out_pc);
  case 0x005D0AU: return step_ir_0x005D0A(bus, out_pc);
  case 0x005D10U: return step_ir_0x005D10(bus, out_pc);
  case 0x005D16U: return step_ir_0x005D16(bus, out_pc);
  case 0x005D1CU: return step_ir_0x005D1C(bus, out_pc);
  case 0x005D1EU: return step_ir_0x005D1E(bus, out_pc);
  case 0x005D22U: return step_ir_0x005D22(bus, out_pc);
  case 0x005D28U: return step_ir_0x005D28(bus, out_pc);
  case 0x005D2EU: return step_ir_0x005D2E(bus, out_pc);
  case 0x005D32U: return step_ir_0x005D32(bus, out_pc);
  case 0x005D38U: return step_ir_0x005D38(bus, out_pc);
  case 0x005D3AU: return step_ir_0x005D3A(bus, out_pc);
  case 0x005D3CU: return step_ir_0x005D3C(bus, out_pc);
  case 0x005D40U: return step_ir_0x005D40(bus, out_pc);
  case 0x005D44U: return step_ir_0x005D44(bus, out_pc);
  case 0x005D46U: return step_ir_0x005D46(bus, out_pc);
  case 0x005D4AU: return step_ir_0x005D4A(bus, out_pc);
  case 0x005D4EU: return step_ir_0x005D4E(bus, out_pc);
  case 0x005D52U: return step_ir_0x005D52(bus, out_pc);
  case 0x005D56U: return step_ir_0x005D56(bus, out_pc);
  case 0x005D5AU: return step_ir_0x005D5A(bus, out_pc);
  case 0x005D5CU: return step_ir_0x005D5C(bus, out_pc);
  case 0x005D60U: return step_ir_0x005D60(bus, out_pc);
  case 0x005D66U: return step_ir_0x005D66(bus, out_pc);
  case 0x005D6CU: return step_ir_0x005D6C(bus, out_pc);
  case 0x005D72U: return step_ir_0x005D72(bus, out_pc);
  case 0x005D74U: return step_ir_0x005D74(bus, out_pc);
  case 0x005D78U: return step_ir_0x005D78(bus, out_pc);
  case 0x005D7AU: return step_ir_0x005D7A(bus, out_pc);
  case 0x005D7EU: return step_ir_0x005D7E(bus, out_pc);
  case 0x005D82U: return step_ir_0x005D82(bus, out_pc);
  case 0x005D88U: return step_ir_0x005D88(bus, out_pc);
  case 0x005D8EU: return step_ir_0x005D8E(bus, out_pc);
  case 0x005D90U: return step_ir_0x005D90(bus, out_pc);
  case 0x005D96U: return step_ir_0x005D96(bus, out_pc);
  case 0x005D9CU: return step_ir_0x005D9C(bus, out_pc);
  case 0x005D9EU: return step_ir_0x005D9E(bus, out_pc);
  case 0x005DA4U: return step_ir_0x005DA4(bus, out_pc);
  case 0x005DA6U: return step_ir_0x005DA6(bus, out_pc);
  case 0x005DAAU: return step_ir_0x005DAA(bus, out_pc);
  case 0x005DB0U: return step_ir_0x005DB0(bus, out_pc);
  case 0x005DB6U: return step_ir_0x005DB6(bus, out_pc);
  case 0x005DB8U: return step_ir_0x005DB8(bus, out_pc);
  case 0x005DBEU: return step_ir_0x005DBE(bus, out_pc);
  case 0x005DC0U: return step_ir_0x005DC0(bus, out_pc);
  case 0x005DC6U: return step_ir_0x005DC6(bus, out_pc);
  case 0x005DC8U: return step_ir_0x005DC8(bus, out_pc);
  case 0x005DCEU: return step_ir_0x005DCE(bus, out_pc);
  case 0x005DD2U: return step_ir_0x005DD2(bus, out_pc);
  case 0x005DD4U: return step_ir_0x005DD4(bus, out_pc);
  case 0x005DD8U: return step_ir_0x005DD8(bus, out_pc);
  case 0x005DDCU: return step_ir_0x005DDC(bus, out_pc);
  case 0x005DE2U: return step_ir_0x005DE2(bus, out_pc);
  case 0x00780EU: return step_ir_0x00780E(bus, out_pc);
  case 0x007810U: return step_ir_0x007810(bus, out_pc);
  case 0x007814U: return step_ir_0x007814(bus, out_pc);
  case 0x007818U: return step_ir_0x007818(bus, out_pc);
  case 0x00781AU: return step_ir_0x00781A(bus, out_pc);
  case 0x00781CU: return step_ir_0x00781C(bus, out_pc);
  case 0x007820U: return step_ir_0x007820(bus, out_pc);
  case 0x007822U: return step_ir_0x007822(bus, out_pc);
  case 0x007824U: return step_ir_0x007824(bus, out_pc);
  case 0x007828U: return step_ir_0x007828(bus, out_pc);
  case 0x00782AU: return step_ir_0x00782A(bus, out_pc);
  case 0x00782CU: return step_ir_0x00782C(bus, out_pc);
  case 0x007830U: return step_ir_0x007830(bus, out_pc);
  case 0x007832U: return step_ir_0x007832(bus, out_pc);
  case 0x007836U: return step_ir_0x007836(bus, out_pc);
  case 0x00783CU: return step_ir_0x00783C(bus, out_pc);
  case 0x007842U: return step_ir_0x007842(bus, out_pc);
  case 0x007844U: return step_ir_0x007844(bus, out_pc);
  case 0x007846U: return step_ir_0x007846(bus, out_pc);
  case 0x00784EU: return step_ir_0x00784E(bus, out_pc);
  default: return false; }
}
} // namespace oasis::recompiler::ir
