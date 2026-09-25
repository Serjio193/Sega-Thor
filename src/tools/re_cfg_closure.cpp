#include "tools/re_cfg_closure.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace oasis::tools {
namespace {
bool contains(std::span<const std::uint32_t> targets, std::uint32_t address) {
    return std::find(targets.begin(), targets.end(), address) != targets.end();
}

void blocker(CfgClosure& result, const std::string& value) {
    if (std::find(result.blockers.begin(), result.blockers.end(), value) == result.blockers.end())
        result.blockers.push_back(value);
}
} // namespace

CfgClosure decode_closed_cfg(
    std::span<const std::uint8_t> rom, std::uint32_t range_start,
    std::uint32_t range_end, std::span<const std::uint32_t> exact_seeds,
    std::span<const std::uint32_t> exact_known_targets) {
    CfgClosure result{};
    if (range_start >= range_end || range_end > rom.size() || (range_start & 1U) ||
        (range_end & 1U) || exact_seeds.empty()) {
        result.blockers.push_back("ENTRY_BOUNDARY_UNPROVEN");
        return result;
    }
    std::map<std::uint32_t, DecodedInstruction> decoded;
    std::set<std::uint32_t> pending(exact_seeds.begin(), exact_seeds.end());
    for (const auto seed : exact_seeds) {
        if ((seed & 1U) || seed < range_start || seed >= range_end)
            blocker(result, "ENTRY_BOUNDARY_UNPROVEN");
    }
    while (!pending.empty()) {
        const auto seed = *pending.begin();
        pending.erase(pending.begin());
        if (seed < range_start || seed >= range_end || decoded.contains(seed)) continue;
        DecodeOptions options{};
        options.entry = seed;
        options.byte_budget = range_end - seed;
        options.instruction_budget = (range_end - seed) / 2U;
        const auto slice = decode_m68k_slice(rom, options);
        for (const auto& instruction : slice.instructions) {
            for (const auto& [address, existing] : decoded) {
                const auto existing_end = address + static_cast<std::uint32_t>(existing.bytes.size());
                const auto instruction_end = instruction.address +
                    static_cast<std::uint32_t>(instruction.bytes.size());
                if (address < instruction_end && instruction.address < existing_end &&
                    (address != instruction.address || existing.bytes != instruction.bytes))
                    blocker(result, "OVERLAPPING_DECODE");
            }
            const auto [position, inserted] = decoded.emplace(instruction.address, instruction);
            if (!inserted && position->second.bytes != instruction.bytes)
                blocker(result, "OVERLAPPING_DECODE");
            if (!instruction.supported || !instruction.exact)
                blocker(result, "UNSUPPORTED_OPCODE");
        }
        for (const auto& edge : slice.control_flow) {
            if (edge.target >= range_start && edge.target < range_end &&
                !decoded.contains(edge.target)) pending.insert(edge.target);
        }
        for (const auto& flow : slice.unresolved_control_flow) {
            if (flow.kind == FlowKind::indirect_jump)
                blocker(result, "UNRESOLVED_INDIRECT_TARGET");
        }
    }
    result.seeds.assign(exact_seeds.begin(), exact_seeds.end());
    std::sort(result.seeds.begin(), result.seeds.end());
    result.seeds.erase(std::unique(result.seeds.begin(), result.seeds.end()), result.seeds.end());
    for (const auto& [address, instruction] : decoded) {
        result.decoded_bytes += instruction.bytes.size();
        const auto next = address + static_cast<std::uint32_t>(instruction.bytes.size());
        auto validate_target = [&](std::uint32_t target) {
            if (target >= range_start && target < range_end) {
                if (!decoded.contains(target)) blocker(result, "MID_INSTRUCTION_TARGET");
            } else if (!contains(exact_known_targets, target)) {
                blocker(result, "CFG_ESCAPES_UNKNOWN_WITHOUT_EXACT_TARGET");
            }
        };
        const auto flow = instruction.flow;
        if (flow == FlowKind::unsupported || !instruction.supported) continue;
        if (flow == FlowKind::return_instruction || instruction.mnemonic == "stop" ||
            instruction.mnemonic == "trap") continue;
        if (flow == FlowKind::direct_jump || flow == FlowKind::direct_branch ||
            flow == FlowKind::direct_call) {
            if (!instruction.direct_target) blocker(result, "UNSUPPORTED_OPCODE");
            else validate_target(*instruction.direct_target);
        }
        const bool has_fallthrough = flow == FlowKind::none || flow == FlowKind::direct_call ||
            flow == FlowKind::indirect_call ||
            (flow == FlowKind::direct_branch && instruction.mnemonic != "bra");
        if (has_fallthrough) validate_target(next);
        if (flow == FlowKind::indirect_jump) blocker(result, "UNRESOLVED_INDIRECT_TARGET");
    }
    std::sort(result.blockers.begin(), result.blockers.end());
    result.closed = result.blockers.empty() && !decoded.empty();
    std::map<std::uint32_t, std::uint32_t> parent;
    for (const auto& [address, instruction] : decoded) {
        (void)instruction;
        parent.emplace(address, address);
    }
    auto root = [&](std::uint32_t address) {
        auto current = address;
        while (parent.at(current) != current) current = parent.at(current);
        return current;
    };
    auto join = [&](std::uint32_t left, std::uint32_t right) {
        if (parent.contains(left) && parent.contains(right)) parent[root(right)] = root(left);
    };
    for (const auto& [address, instruction] : decoded) {
        if (instruction.direct_target) join(address, *instruction.direct_target);
        const auto next = address + static_cast<std::uint32_t>(instruction.bytes.size());
        const auto flow = instruction.flow;
        if (flow == FlowKind::none || flow == FlowKind::direct_call ||
            flow == FlowKind::indirect_call ||
            (flow == FlowKind::direct_branch && instruction.mnemonic != "bra"))
            join(address, next);
    }
    std::set<std::uint32_t> roots;
    for (const auto& [address, instruction] : decoded) {
        (void)instruction;
        roots.insert(root(address));
    }
    result.connected_components = roots.size();
    for (auto& [address, instruction] : decoded) {
        (void)address;
        result.instructions.push_back(std::move(instruction));
    }
    return result;
}

} // namespace oasis::tools
