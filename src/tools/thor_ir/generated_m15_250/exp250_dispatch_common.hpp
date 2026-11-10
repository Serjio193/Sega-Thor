#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"

namespace oasis::recompiler::ir {

struct PortableIrBus250 : PortableIrBus249 {
  explicit PortableIrBus250(PortableGuestEnvironment& e) noexcept : PortableIrBus249{e} {}
};

} // namespace oasis::recompiler::ir
