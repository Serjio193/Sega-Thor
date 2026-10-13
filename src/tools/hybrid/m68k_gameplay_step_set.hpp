#pragma once

#include <string_view>

namespace oasis::hybrid { class ExternalLibrary; }

namespace oasis::hybrid {

void install_m68k_gameplay_step_set(ExternalLibrary& core,
                                    std::string_view enabled_helpers,
                                    void (*observe)(unsigned, unsigned),
                                    void (*fault)(unsigned));

} // namespace oasis::hybrid
