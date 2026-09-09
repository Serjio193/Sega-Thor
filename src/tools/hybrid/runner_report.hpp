#pragma once

#include "tools/hybrid/basic_block.hpp"
#include "tools/hybrid/mechanical_primitive.hpp"
#include "tools/hybrid/replacement.hpp"

#include <ostream>

namespace oasis::hybrid {

void write_runner_report_details(std::ostream& report, const Registry* registry,
                                 const BasicBlockRegistry* blocks,
                                 const MechanicalPrimitiveRegistry* primitives);

} // namespace oasis::hybrid
