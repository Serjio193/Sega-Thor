#include "tools/re_program.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

std::map<std::uint32_t, std::uint32_t> block_map(const DecodedSlice& slice) {
    std::map<std::uint32_t, std::uint32_t> result;
    for (const auto& block : slice.basic_blocks) {
        for (const auto address : block.instruction_addresses) result.emplace(address, block.start);
    }
    return result;
}

std::optional<std::uint32_t> discover_return_boundary(const DecodedSlice& slice) {
    std::set<std::uint32_t> addresses;
    for (const auto& instruction : slice.instructions) addresses.insert(instruction.address);
    bool has_return = false;
    bool complete = true;
    std::uint32_t last_return_end = 0;
    for (const auto& instruction : slice.instructions) {
        const auto next = instruction.address + static_cast<std::uint32_t>(instruction.bytes.size());
        if (instruction.flow == FlowKind::return_instruction) {
            has_return = true;
            last_return_end = std::max(last_return_end, next);
            continue;
        }
        if (instruction.flow == FlowKind::unsupported) {
            complete = false;
            continue;
        }
        bool successor = false;
        if (instruction.direct_target &&
            (instruction.flow == FlowKind::direct_branch || instruction.flow == FlowKind::direct_jump)) {
            successor = addresses.contains(*instruction.direct_target);
        }
        if (instruction.flow == FlowKind::direct_call ||
            (instruction.flow == FlowKind::direct_branch && instruction.mnemonic != "bra") ||
            instruction.flow == FlowKind::none) {
            successor = successor || addresses.contains(next);
        }
        if (!successor) complete = false;
    }
    if (!has_return || !complete || !slice.unresolved_control_flow.empty() ||
        !slice.unsupported_instruction_addresses.empty()) {
        return std::nullopt;
    }
    return last_return_end;
}

bool target_less(const FunctionTarget& left, const FunctionTarget& right) {
    return std::tie(left.entry, left.byte_budget, left.confirmed_end) <
           std::tie(right.entry, right.byte_budget, right.confirmed_end);
}

} // namespace

std::string boundary_status_name(BoundaryStatus status) {
    switch (status) {
    case BoundaryStatus::confirmed: return "confirmed";
    case BoundaryStatus::discovered_return: return "discovered_return";
    case BoundaryStatus::bounded_only: return "bounded_only";
    }
    return "unknown";
}

MultiSliceReport analyze_m68k_functions(std::span<const std::uint8_t> rom,
                                        std::span<const FunctionTarget> targets) {
    std::vector<FunctionTarget> ordered(targets.begin(), targets.end());
    std::sort(ordered.begin(), ordered.end(), target_less);
    if (std::adjacent_find(ordered.begin(), ordered.end(), [](const auto& left, const auto& right) {
            return left.entry == right.entry;
        }) != ordered.end()) {
        throw std::invalid_argument("duplicate bounded function entry");
    }
    MultiSliceReport report;
    std::set<std::uint32_t> entries;
    for (const auto& target : ordered) {
        if (target.confirmed_end && *target.confirmed_end <= target.entry) {
            throw std::invalid_argument("invalid confirmed function boundary");
        }
        const auto budget = target.confirmed_end ? *target.confirmed_end - target.entry
                                                 : target.byte_budget;
        if (budget < 2U) throw std::invalid_argument("function budget is too small");
        const auto slice = decode_m68k_slice(rom, {.entry = target.entry, .byte_budget = budget});
        AnalyzedFunction function{.entry = target.entry,
                                  .range_end = slice.range_end,
                                  .boundary = target.confirmed_end ? BoundaryStatus::confirmed
                                                                   : BoundaryStatus::bounded_only,
                                  .slice = slice};
        if (!target.confirmed_end) {
            function.boundary_end = discover_return_boundary(function.slice);
            if (function.boundary_end) function.boundary = BoundaryStatus::discovered_return;
        } else {
            function.boundary_end = target.confirmed_end;
        }
        entries.insert(function.entry);
        report.functions.push_back(std::move(function));
    }

    for (const auto& function : report.functions) {
        const auto blocks = block_map(function.slice);
        for (const auto& edge : function.slice.control_flow) {
            if (edge.kind != FlowKind::direct_call) continue;
            report.direct_call_sites.push_back({function.entry, blocks.at(edge.source), edge.source,
                                                edge.target, entries.contains(edge.target)});
        }
        for (const auto& item : function.slice.unresolved_control_flow) {
            report.unresolved_control_flow.push_back(
                {function.entry, blocks.at(item.address), item});
        }
        for (const auto& instruction : function.slice.instructions) {
            const auto block_start = blocks.at(instruction.address);
            for (const auto& reference : instruction.memory_references) {
                report.confirmed_memory_references.push_back(
                    {function.entry, function.range_end, block_start, instruction.address, reference});
            }
            for (const auto& reference : instruction.unresolved_memory_references) {
                report.unresolved_memory_references.push_back(
                    {function.entry, function.range_end, block_start, instruction.address, reference});
            }
            for (const auto& reference : instruction.unsupported_addressing) {
                report.unsupported_addressing.push_back(
                    {function.entry, function.range_end, block_start, instruction.address, reference});
            }
            if (!instruction.supported && instruction.unsupported_addressing.empty()) {
                report.unsupported_instructions.push_back(
                    {function.entry, function.range_end, block_start, instruction.address,
                     instruction.opcode});
            }
        }
    }

    std::sort(report.direct_call_sites.begin(), report.direct_call_sites.end(), [](const auto& left, const auto& right) {
        return std::tie(left.caller_entry, left.instruction_address, left.target) <
               std::tie(right.caller_entry, right.instruction_address, right.target);
    });
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::uint32_t>> grouped_calls;
    for (const auto& call : report.direct_call_sites) {
        if (call.target_analyzed) grouped_calls[{call.caller_entry, call.target}].push_back(call.instruction_address);
    }
    for (auto& [pair, sites] : grouped_calls) {
        report.function_call_edges.push_back({pair.first, pair.second, std::move(sites)});
    }
    std::sort(report.function_call_edges.begin(), report.function_call_edges.end(), [](const auto& left, const auto& right) {
        return std::tie(left.caller_entry, left.callee_entry) <
               std::tie(right.caller_entry, right.callee_entry);
    });
    return report;
}

} // namespace oasis::tools
