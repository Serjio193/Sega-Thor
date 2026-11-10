#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"

namespace oasis::recompiler::ir {

struct PortableIrBus247 : PortableIrBus246 {
  explicit PortableIrBus247(PortableGuestEnvironment& e) noexcept : PortableIrBus246{e} {}
};

inline int m15247_calc_mulu_cycles(std::uint16_t src) noexcept {
  int mc = 38 * 7;
  while (src != 0U) { if ((src & 1U) != 0U) mc += 14; src >>= 1U; }
  return mc;
}

} // namespace oasis::recompiler::ir
