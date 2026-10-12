#pragma once

#include "tools/hybrid/gameplay_session_cpu.hpp"
#include "tools/hybrid/grid_query_native_model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oasis::hybrid::session {

struct CallMemoryEvent {
    unsigned type{}, pc{}, address{}, width{}, value{};
};

struct CallCapture {
    bool active{};
    bool saw_rts{}, saw_return{}, saw_continuation{};
    unsigned source_opcode{}, entry_opcode{}, stack_return{};
    CpuSnapshot source{}, entry{}, rts{}, returned{}, continuation{};
    std::optional<grid_query::Observation> native_grid;
    std::optional<bool> native_d5_match;
    std::vector<CallMemoryEvent> events;
    std::vector<unsigned> pcs;
};

[[nodiscard]] std::string serialize_call_capture(
    const CallCapture& call, Peek peek_memory,
    const std::vector<std::uint8_t>& before_ram);

} // namespace oasis::hybrid::session
