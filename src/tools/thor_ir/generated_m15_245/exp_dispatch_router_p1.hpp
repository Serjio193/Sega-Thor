#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part5.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part6.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part7.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part8.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part9.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part10.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part11.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part12.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part13.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part14.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part15.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part16.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp_chunk_1(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x006762U: return step_ir_0x006762(bus, out_pc);
  case 0x006764U: return step_ir_0x006764(bus, out_pc);
  case 0x00676CU: return step_ir_0x00676C(bus, out_pc);
  case 0x00676EU: return step_ir_0x00676E(bus, out_pc);
  case 0x006774U: return step_ir_0x006774(bus, out_pc);
  case 0x00677AU: return step_ir_0x00677A(bus, out_pc);
  case 0x006780U: return step_ir_0x006780(bus, out_pc);
  case 0x006782U: return step_ir_0x006782(bus, out_pc);
  case 0x006786U: return step_ir_0x006786(bus, out_pc);
  case 0x00678AU: return step_ir_0x00678A(bus, out_pc);
  case 0x00678EU: return step_ir_0x00678E(bus, out_pc);
  case 0x006790U: return step_ir_0x006790(bus, out_pc);
  case 0x006796U: return step_ir_0x006796(bus, out_pc);
  case 0x00679AU: return step_ir_0x00679A(bus, out_pc);
  case 0x0067A0U: return step_ir_0x0067A0(bus, out_pc);
  case 0x0067A4U: return step_ir_0x0067A4(bus, out_pc);
  case 0x0067A6U: return step_ir_0x0067A6(bus, out_pc);
  case 0x0067A8U: return step_ir_0x0067A8(bus, out_pc);
  case 0x0067AAU: return step_ir_0x0067AA(bus, out_pc);
  case 0x0067ACU: return step_ir_0x0067AC(bus, out_pc);
  case 0x0067AEU: return step_ir_0x0067AE(bus, out_pc);
  case 0x0067B0U: return step_ir_0x0067B0(bus, out_pc);
  case 0x0067B2U: return step_ir_0x0067B2(bus, out_pc);
  case 0x0067B4U: return step_ir_0x0067B4(bus, out_pc);
  case 0x0067B8U: return step_ir_0x0067B8(bus, out_pc);
  case 0x0067BAU: return step_ir_0x0067BA(bus, out_pc);
  case 0x0067BCU: return step_ir_0x0067BC(bus, out_pc);
  case 0x0067BEU: return step_ir_0x0067BE(bus, out_pc);
  case 0x0067C0U: return step_ir_0x0067C0(bus, out_pc);
  case 0x0067C2U: return step_ir_0x0067C2(bus, out_pc);
  case 0x0067C4U: return step_ir_0x0067C4(bus, out_pc);
  case 0x0067C6U: return step_ir_0x0067C6(bus, out_pc);
  case 0x0067C8U: return step_ir_0x0067C8(bus, out_pc);
  case 0x0067CCU: return step_ir_0x0067CC(bus, out_pc);
  case 0x0067D0U: return step_ir_0x0067D0(bus, out_pc);
  case 0x0067D4U: return step_ir_0x0067D4(bus, out_pc);
  case 0x0067D8U: return step_ir_0x0067D8(bus, out_pc);
  case 0x0067DCU: return step_ir_0x0067DC(bus, out_pc);
  case 0x006E28U: return step_ir_0x006E28(bus, out_pc);
  case 0x006E4AU: return step_ir_0x006E4A(bus, out_pc);
  case 0x006E50U: return step_ir_0x006E50(bus, out_pc);
  case 0x006E52U: return step_ir_0x006E52(bus, out_pc);
  case 0x006E56U: return step_ir_0x006E56(bus, out_pc);
  case 0x006E5CU: return step_ir_0x006E5C(bus, out_pc);
  case 0x006E5EU: return step_ir_0x006E5E(bus, out_pc);
  case 0x006E64U: return step_ir_0x006E64(bus, out_pc);
  case 0x006E68U: return step_ir_0x006E68(bus, out_pc);
  case 0x006E6CU: return step_ir_0x006E6C(bus, out_pc);
  case 0x006E72U: return step_ir_0x006E72(bus, out_pc);
  case 0x006E78U: return step_ir_0x006E78(bus, out_pc);
  case 0x006E7AU: return step_ir_0x006E7A(bus, out_pc);
  case 0x006E82U: return step_ir_0x006E82(bus, out_pc);
  case 0x006E84U: return step_ir_0x006E84(bus, out_pc);
  case 0x006EDAU: return step_ir_0x006EDA(bus, out_pc);
  case 0x006EE0U: return step_ir_0x006EE0(bus, out_pc);
  case 0x006EE4U: return step_ir_0x006EE4(bus, out_pc);
  case 0x006EE6U: return step_ir_0x006EE6(bus, out_pc);
  case 0x006EE8U: return step_ir_0x006EE8(bus, out_pc);
  case 0x006EEEU: return step_ir_0x006EEE(bus, out_pc);
  case 0x006EF0U: return step_ir_0x006EF0(bus, out_pc);
  case 0x006EF4U: return step_ir_0x006EF4(bus, out_pc);
  case 0x006EF8U: return step_ir_0x006EF8(bus, out_pc);
  case 0x006EFCU: return step_ir_0x006EFC(bus, out_pc);
  case 0x006F00U: return step_ir_0x006F00(bus, out_pc);
  case 0x006F06U: return step_ir_0x006F06(bus, out_pc);
  case 0x006F08U: return step_ir_0x006F08(bus, out_pc);
  case 0x006F0AU: return step_ir_0x006F0A(bus, out_pc);
  case 0x006F0EU: return step_ir_0x006F0E(bus, out_pc);
  case 0x006F12U: return step_ir_0x006F12(bus, out_pc);
  case 0x006F14U: return step_ir_0x006F14(bus, out_pc);
  case 0x006F18U: return step_ir_0x006F18(bus, out_pc);
  case 0x006F1CU: return step_ir_0x006F1C(bus, out_pc);
  case 0x006F1EU: return step_ir_0x006F1E(bus, out_pc);
  case 0x006F24U: return step_ir_0x006F24(bus, out_pc);
  case 0x006F26U: return step_ir_0x006F26(bus, out_pc);
  case 0x006F28U: return step_ir_0x006F28(bus, out_pc);
  case 0x006F2CU: return step_ir_0x006F2C(bus, out_pc);
  case 0x006F32U: return step_ir_0x006F32(bus, out_pc);
  case 0x006F38U: return step_ir_0x006F38(bus, out_pc);
  case 0x007112U: return step_ir_0x007112(bus, out_pc);
  case 0x007118U: return step_ir_0x007118(bus, out_pc);
  case 0x00711CU: return step_ir_0x00711C(bus, out_pc);
  case 0x007120U: return step_ir_0x007120(bus, out_pc);
  case 0x007124U: return step_ir_0x007124(bus, out_pc);
  case 0x00712AU: return step_ir_0x00712A(bus, out_pc);
  case 0x00712CU: return step_ir_0x00712C(bus, out_pc);
  case 0x007132U: return step_ir_0x007132(bus, out_pc);
  case 0x007134U: return step_ir_0x007134(bus, out_pc);
  case 0x00713AU: return step_ir_0x00713A(bus, out_pc);
  case 0x00713CU: return step_ir_0x00713C(bus, out_pc);
  case 0x007142U: return step_ir_0x007142(bus, out_pc);
  case 0x007148U: return step_ir_0x007148(bus, out_pc);
  case 0x00714CU: return step_ir_0x00714C(bus, out_pc);
  case 0x00714EU: return step_ir_0x00714E(bus, out_pc);
  case 0x007152U: return step_ir_0x007152(bus, out_pc);
  case 0x007158U: return step_ir_0x007158(bus, out_pc);
  case 0x00715AU: return step_ir_0x00715A(bus, out_pc);
  case 0x00715CU: return step_ir_0x00715C(bus, out_pc);
  case 0x00715EU: return step_ir_0x00715E(bus, out_pc);
  case 0x007164U: return step_ir_0x007164(bus, out_pc);
  case 0x007168U: return step_ir_0x007168(bus, out_pc);
  case 0x00716CU: return step_ir_0x00716C(bus, out_pc);
  case 0x007170U: return step_ir_0x007170(bus, out_pc);
  case 0x007174U: return step_ir_0x007174(bus, out_pc);
  case 0x007178U: return step_ir_0x007178(bus, out_pc);
  case 0x00717CU: return step_ir_0x00717C(bus, out_pc);
  case 0x00717EU: return step_ir_0x00717E(bus, out_pc);
  case 0x007184U: return step_ir_0x007184(bus, out_pc);
  case 0x007188U: return step_ir_0x007188(bus, out_pc);
  case 0x00718CU: return step_ir_0x00718C(bus, out_pc);
  case 0x00718EU: return step_ir_0x00718E(bus, out_pc);
  case 0x007194U: return step_ir_0x007194(bus, out_pc);
  case 0x0071DAU: return step_ir_0x0071DA(bus, out_pc);
  case 0x0071E0U: return step_ir_0x0071E0(bus, out_pc);
  case 0x0071E4U: return step_ir_0x0071E4(bus, out_pc);
  case 0x0071E8U: return step_ir_0x0071E8(bus, out_pc);
  case 0x0071EAU: return step_ir_0x0071EA(bus, out_pc);
  case 0x0071EEU: return step_ir_0x0071EE(bus, out_pc);
  case 0x0071F4U: return step_ir_0x0071F4(bus, out_pc);
  case 0x0071F6U: return step_ir_0x0071F6(bus, out_pc);
  case 0x0071F8U: return step_ir_0x0071F8(bus, out_pc);
  case 0x0071FAU: return step_ir_0x0071FA(bus, out_pc);
  case 0x007200U: return step_ir_0x007200(bus, out_pc);
  case 0x007204U: return step_ir_0x007204(bus, out_pc);
  case 0x007208U: return step_ir_0x007208(bus, out_pc);
  case 0x00720CU: return step_ir_0x00720C(bus, out_pc);
  case 0x007210U: return step_ir_0x007210(bus, out_pc);
  case 0x007214U: return step_ir_0x007214(bus, out_pc);
  case 0x007218U: return step_ir_0x007218(bus, out_pc);
  case 0x00721CU: return step_ir_0x00721C(bus, out_pc);
  case 0x007220U: return step_ir_0x007220(bus, out_pc);
  case 0x007224U: return step_ir_0x007224(bus, out_pc);
  case 0x007226U: return step_ir_0x007226(bus, out_pc);
  case 0x00722CU: return step_ir_0x00722C(bus, out_pc);
  case 0x00722EU: return step_ir_0x00722E(bus, out_pc);
  case 0x007234U: return step_ir_0x007234(bus, out_pc);
  case 0x007238U: return step_ir_0x007238(bus, out_pc);
  case 0x00723CU: return step_ir_0x00723C(bus, out_pc);
  case 0x00723EU: return step_ir_0x00723E(bus, out_pc);
  case 0x007242U: return step_ir_0x007242(bus, out_pc);
  case 0x007248U: return step_ir_0x007248(bus, out_pc);
  case 0x00724EU: return step_ir_0x00724E(bus, out_pc);
  case 0x007252U: return step_ir_0x007252(bus, out_pc);
  case 0x007256U: return step_ir_0x007256(bus, out_pc);
  case 0x00725CU: return step_ir_0x00725C(bus, out_pc);
  case 0x007260U: return step_ir_0x007260(bus, out_pc);
  case 0x007656U: return step_ir_0x007656(bus, out_pc);
  case 0x00765EU: return step_ir_0x00765E(bus, out_pc);
  case 0x007660U: return step_ir_0x007660(bus, out_pc);
  case 0x007666U: return step_ir_0x007666(bus, out_pc);
  case 0x00766AU: return step_ir_0x00766A(bus, out_pc);
  case 0x007670U: return step_ir_0x007670(bus, out_pc);
  case 0x007674U: return step_ir_0x007674(bus, out_pc);
  case 0x00767AU: return step_ir_0x00767A(bus, out_pc);
  case 0x00767EU: return step_ir_0x00767E(bus, out_pc);
  case 0x007684U: return step_ir_0x007684(bus, out_pc);
  case 0x007688U: return step_ir_0x007688(bus, out_pc);
  case 0x00768EU: return step_ir_0x00768E(bus, out_pc);
  case 0x007692U: return step_ir_0x007692(bus, out_pc);
  case 0x007698U: return step_ir_0x007698(bus, out_pc);
  case 0x00769EU: return step_ir_0x00769E(bus, out_pc);
  case 0x0076A4U: return step_ir_0x0076A4(bus, out_pc);
  case 0x0076AAU: return step_ir_0x0076AA(bus, out_pc);
  case 0x0076ACU: return step_ir_0x0076AC(bus, out_pc);
  case 0x0076B2U: return step_ir_0x0076B2(bus, out_pc);
  case 0x0076B4U: return step_ir_0x0076B4(bus, out_pc);
  case 0x0076BAU: return step_ir_0x0076BA(bus, out_pc);
  case 0x0076C0U: return step_ir_0x0076C0(bus, out_pc);
  case 0x0076C2U: return step_ir_0x0076C2(bus, out_pc);
  case 0x0076C4U: return step_ir_0x0076C4(bus, out_pc);
  case 0x0076CAU: return step_ir_0x0076CA(bus, out_pc);
  case 0x0076D0U: return step_ir_0x0076D0(bus, out_pc);
  case 0x0076D6U: return step_ir_0x0076D6(bus, out_pc);
  case 0x0076D8U: return step_ir_0x0076D8(bus, out_pc);
  case 0x0076DCU: return step_ir_0x0076DC(bus, out_pc);
  case 0x0076E2U: return step_ir_0x0076E2(bus, out_pc);
  case 0x0076E6U: return step_ir_0x0076E6(bus, out_pc);
  case 0x0076E8U: return step_ir_0x0076E8(bus, out_pc);
  case 0x0076EAU: return step_ir_0x0076EA(bus, out_pc);
  case 0x0076EEU: return step_ir_0x0076EE(bus, out_pc);
  default: return false; }
}
} // namespace oasis::recompiler::ir
