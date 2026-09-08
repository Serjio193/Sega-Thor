#pragma once

#include <filesystem>
#include <map>
#include <string_view>

namespace oasis::hybrid {

void write_interpreter_profile(const std::filesystem::path& path,
                               std::string_view mode,
                               const std::map<unsigned, unsigned>& pcs,
                               unsigned total_executions);

} // namespace oasis::hybrid
