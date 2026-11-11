#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"

namespace oasis::recompiler::ir {

struct PortableIrBus256 : PortableIrBus250 {
  explicit PortableIrBus256(PortableGuestEnvironment& e) noexcept : PortableIrBus250{e} {}
};

} // namespace oasis::recompiler::ir
