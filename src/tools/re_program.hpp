#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class BoundaryStatus { confirmed, discovered_return, bounded_only };

struct FunctionTarget {
    std::uint32_t entry{};
    std::size_t byte_budget{};
    std::optional<std::uint32_t> confirmed_end;
};

struct AnalyzedFunction {
    std::uint32_t entry{};
    std::uint32_t range_end{};
    BoundaryStatus boundary{BoundaryStatus::bounded_only};
    std::optional<std::uint32_t> boundary_end;
    DecodedSlice slice;
};

struct BoundMemoryReference {
    std::uint32_t function_entry{};
    std::uint32_t slice_range_end{};
    std::uint32_t block_start{};
    std::uint32_t instruction_address{};
    MemoryReference reference;
};

struct BoundUnresolvedMemoryReference {
    std::uint32_t function_entry{};
    std::uint32_t slice_range_end{};
    std::uint32_t block_start{};
    std::uint32_t instruction_address{};
    UnresolvedMemoryReference reference;
};

struct BoundUnsupportedAddressing {
    std::uint32_t function_entry{};
    std::uint32_t slice_range_end{};
    std::uint32_t block_start{};
    std::uint32_t instruction_address{};
    UnsupportedAddressing reference;
};

struct BoundUnsupportedInstruction {
    std::uint32_t function_entry{};
    std::uint32_t slice_range_end{};
    std::uint32_t block_start{};
    std::uint32_t instruction_address{};
    std::uint16_t opcode{};
};

struct DirectCallSite {
    std::uint32_t caller_entry{};
    std::uint32_t block_start{};
    std::uint32_t instruction_address{};
    std::uint32_t target{};
    bool target_analyzed{};
};

struct FunctionCallEdge {
    std::uint32_t caller_entry{};
    std::uint32_t callee_entry{};
    std::vector<std::uint32_t> call_sites;
};

struct BoundUnresolvedControlFlow {
    std::uint32_t function_entry{};
    std::uint32_t block_start{};
    UnresolvedControlFlow flow;
};

struct MultiSliceReport {
    std::vector<AnalyzedFunction> functions;
    std::vector<DirectCallSite> direct_call_sites;
    std::vector<FunctionCallEdge> function_call_edges;
    std::vector<BoundUnresolvedControlFlow> unresolved_control_flow;
    std::vector<BoundMemoryReference> confirmed_memory_references;
    std::vector<BoundUnresolvedMemoryReference> unresolved_memory_references;
    std::vector<BoundUnsupportedAddressing> unsupported_addressing;
    std::vector<BoundUnsupportedInstruction> unsupported_instructions;
};

[[nodiscard]] MultiSliceReport analyze_m68k_functions(
    std::span<const std::uint8_t> rom, std::span<const FunctionTarget> targets);

[[nodiscard]] std::string boundary_status_name(BoundaryStatus status);
[[nodiscard]] std::string program_to_json(const MultiSliceReport& report);
[[nodiscard]] std::string program_to_text(const MultiSliceReport& report);

} // namespace oasis::tools
