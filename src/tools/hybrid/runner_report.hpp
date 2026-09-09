#pragma once

#include "tools/hybrid/basic_block.hpp"
#include "tools/hybrid/contract.hpp"
#include "tools/hybrid/mechanical_primitive.hpp"
#include "tools/hybrid/replacement.hpp"

#include <ostream>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace oasis::hybrid {

void write_runner_report_details(std::ostream& report, const Registry* registry,
                                 const BasicBlockRegistry* blocks,
                                 const MechanicalPrimitiveRegistry* primitives);
void write_native_routine_accounting(std::ostream& report, const ReplacementMetrics& metrics,
                                     unsigned generated_translated,
                                     unsigned mechanical_primitive,
                                     unsigned interpreter);
std::string hash_text(std::string_view value);
void write_interpreter_profile_report(const std::filesystem::path& path, std::string_view mode,
                                      const std::map<unsigned, unsigned>& pcs,
                                      unsigned instructions);
bool body_was_skipped(bool block_mode, unsigned overrides, unsigned original_inside,
                      const Registry* registry, unsigned body_instructions);
bool full_cpu_identity(bool completed, unsigned divergences, bool block_mode, Mode mode);
unsigned guest_instruction_total(unsigned interpreter, unsigned translated,
                                 unsigned mechanical, unsigned routine,
                                 unsigned second_routine);

} // namespace oasis::hybrid
