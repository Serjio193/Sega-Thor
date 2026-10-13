#pragma once

#include <filesystem>
#include <string_view>

namespace oasis::hybrid { class ExternalLibrary; }

namespace oasis::hybrid {

void install_m68k_gameplay_step_set(ExternalLibrary& core,
                                    std::string_view enabled_helpers,
                                    void (*observe)(unsigned, unsigned),
                                    void (*fault)(unsigned),
                                    unsigned (*transfer_source_pc)());
void write_m68k_gameplay_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
