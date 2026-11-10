#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"
namespace oasis::recompiler::ir {
inline bool run_exp_mb_chunk_2(PortableIrBus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {
  switch (pc) {
  case 0x006F2CU: {
    if (!step_ir_0x006F2C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006F32(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006F38(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007112U: {
    if (!step_ir_0x007112(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007118(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00711CU: {
    if (!step_ir_0x00711C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007120U: {
    if (!step_ir_0x007120(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007124U: {
    if (!step_ir_0x007124(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00712A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00712CU: {
    if (!step_ir_0x00712C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007132(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007134U: {
    if (!step_ir_0x007134(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00713A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00713CU: {
    if (!step_ir_0x00713C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007142(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007148U: {
    if (!step_ir_0x007148(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00714C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00714EU: {
    if (!step_ir_0x00714E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007152U: {
    if (!step_ir_0x007152(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007158(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00715AU: {
    if (!step_ir_0x00715A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00715CU: {
    if (!step_ir_0x00715C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00715EU: {
    if (!step_ir_0x00715E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007164(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007168U: {
    if (!step_ir_0x007168(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00716C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007170U: {
    if (!step_ir_0x007170(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007174(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007178(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00717CU: {
    if (!step_ir_0x00717C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00717EU: {
    if (!step_ir_0x00717E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007184(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007188(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00718C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00718EU: {
    if (!step_ir_0x00718E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007194(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071DAU: {
    if (!step_ir_0x0071DA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0071E0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071E4U: {
    if (!step_ir_0x0071E4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0071E8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071EAU: {
    if (!step_ir_0x0071EA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071EEU: {
    if (!step_ir_0x0071EE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0071F4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071F6U: {
    if (!step_ir_0x0071F6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071F8U: {
    if (!step_ir_0x0071F8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0071FAU: {
    if (!step_ir_0x0071FA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007200(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007204U: {
    if (!step_ir_0x007204(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007208(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00720CU: {
    if (!step_ir_0x00720C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007210(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007214(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007218(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00721C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007220U: {
    if (!step_ir_0x007220(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007224(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007226U: {
    if (!step_ir_0x007226(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00722C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00722EU: {
    if (!step_ir_0x00722E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007234(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007238(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00723C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00723EU: {
    if (!step_ir_0x00723E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007242U: {
    if (!step_ir_0x007242(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007248(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00724E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007252U: {
    if (!step_ir_0x007252(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007256U: {
    if (!step_ir_0x007256(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00725C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007260U: {
    if (!step_ir_0x007260(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007656U: {
    if (!step_ir_0x007656(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00765E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007660U: {
    if (!step_ir_0x007660(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007666U: {
    if (!step_ir_0x007666(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00766AU: {
    if (!step_ir_0x00766A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007670U: {
    if (!step_ir_0x007670(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007674U: {
    if (!step_ir_0x007674(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00767AU: {
    if (!step_ir_0x00767A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00767EU: {
    if (!step_ir_0x00767E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007684U: {
    if (!step_ir_0x007684(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007688U: {
    if (!step_ir_0x007688(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00768EU: {
    if (!step_ir_0x00768E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007692U: {
    if (!step_ir_0x007692(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007698(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00769E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076A4U: {
    if (!step_ir_0x0076A4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076AA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076ACU: {
    if (!step_ir_0x0076AC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076B2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076B4U: {
    if (!step_ir_0x0076B4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076BAU: {
    if (!step_ir_0x0076BA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076C0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076C2U: {
    if (!step_ir_0x0076C2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076C4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076CA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076D0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076D6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076D8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076DCU: {
    if (!step_ir_0x0076DC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076E2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076E6U: {
    if (!step_ir_0x0076E6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076E8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076EA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076EE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076F2U: {
    if (!step_ir_0x0076F2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076F6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0076F8U: {
    if (!step_ir_0x0076F8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0076FE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007700U: {
    if (!step_ir_0x007700(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007706(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00770AU: {
    if (!step_ir_0x00770A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00770CU: {
    if (!step_ir_0x00770C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007712(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007718(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007720(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007726(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00772E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007734(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00773A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  default: return false; }
}
} // namespace oasis::recompiler::ir
