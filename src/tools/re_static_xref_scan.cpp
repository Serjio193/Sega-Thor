#include "tools/re_static_xref_scan.hpp"

#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

bool overlaps(StaticXrefSpan left, StaticXrefSpan right) {
    return left.start < right.end && right.start < left.end;
}

bool falls_through(const DecodedInstruction& instruction) {
    if (instruction.flow == FlowKind::none ||
        instruction.flow == FlowKind::direct_call ||
        instruction.flow == FlowKind::indirect_call) return true;
    return instruction.flow == FlowKind::direct_branch &&
        instruction.branch_condition_code &&
        *instruction.branch_condition_code != 0U;
}

void add_candidate(StaticXrefScanResult& result,
                   const DecodedInstruction& instruction,
                   StaticXrefSpan source, std::uint32_t target,
                   std::span<const StaticXrefSpan> components, bool fallthrough) {
    for (const auto component : components) {
        if (target < component.start || target >= component.end) continue;
        result.candidates.push_back({instruction.address,
            instruction.address + static_cast<std::uint32_t>(instruction.bytes.size()),
            instruction.opcode, instruction.mnemonic, instruction.flow,
            instruction.branch_condition_code.value_or(0xFFU), target,
            source, component, target == component.start, fallthrough});
    }
}

} // namespace

std::string static_xref_kind(const StaticXrefCandidate& candidate) {
    if (candidate.fallthrough) return "VERIFIED_FALLTHROUGH";
    if (candidate.flow == FlowKind::direct_call) return "DIRECT_CALL_TARGET";
    if (candidate.flow == FlowKind::direct_branch) return "DIRECT_BRANCH_TARGET";
    if (candidate.flow == FlowKind::direct_jump) return "DIRECT_JUMP_TARGET";
    return "VERIFIED_FALLTHROUGH";
}

StaticXrefScanResult scan_static_xrefs(
    std::span<const std::uint8_t> rom,
    std::span<const StaticXrefSpan> verified_asm,
    std::span<const StaticXrefSpan> target_components) {
    StaticXrefScanResult result;
    std::vector<StaticXrefSpan> sources(verified_asm.begin(), verified_asm.end());
    std::vector<StaticXrefSpan> targets(target_components.begin(), target_components.end());
    std::sort(sources.begin(), sources.end(), [](auto left, auto right) {
        return std::tie(left.start, left.end) < std::tie(right.start, right.end);
    });
    std::sort(targets.begin(), targets.end(), [](auto left, auto right) {
        return std::tie(left.start, left.end) < std::tie(right.start, right.end);
    });
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (sources[i].start >= sources[i].end || sources[i].end > rom.size() ||
            (sources[i].start & 1U) != 0U ||
            (i != 0U && overlaps(sources[i - 1U], sources[i])))
            throw std::invalid_argument("invalid or overlapping verified ASM spans");
    }
    for (std::size_t i = 0; i < targets.size(); ++i) {
        if (targets[i].start >= targets[i].end || targets[i].end > rom.size() ||
            (targets[i].start & 1U) != 0U ||
            (i != 0U && overlaps(targets[i - 1U], targets[i])))
            throw std::invalid_argument("invalid or overlapping target components");
    }

    for (const auto source : sources) {
        auto pc = source.start;
        StaticXrefRangeResult range{source, source.start, 0U, {}};
        while (pc < source.end) {
            DecodeOptions options{};
            options.entry = pc;
            options.byte_budget = source.end - pc;
            options.instruction_budget = 1U;
            const auto decoded = decode_m68k_slice(rom, options);
            if (decoded.instructions.size() != 1U) {
                range.stop_reason = "NO_EXACT_INSTRUCTION";
                break;
            }
            const auto& instruction = decoded.instructions.front();
            if (!instruction.supported || instruction.address != pc ||
                instruction.bytes.empty() ||
                instruction.bytes.size() > source.end - pc) {
                range.stop_reason = "UNSUPPORTED_OR_TRUNCATED_INSTRUCTION";
                break;
            }
            const auto next = pc + static_cast<std::uint32_t>(instruction.bytes.size());
            if (instruction.direct_target &&
                (instruction.flow == FlowKind::direct_call ||
                 instruction.flow == FlowKind::direct_branch ||
                 instruction.flow == FlowKind::direct_jump)) {
                add_candidate(result, instruction, source,
                              *instruction.direct_target, targets, false);
            }
            if (falls_through(instruction))
                add_candidate(result, instruction, source, next, targets, true);
            ++range.instruction_count;
            ++result.decoded_instruction_count;
            result.decoded_byte_count += instruction.bytes.size();
            pc = next;
            range.decoded_end = pc;
        }
        if (range.stop_reason.empty()) range.stop_reason = "COMPLETE";
        result.ranges.push_back(std::move(range));
    }
    std::sort(result.candidates.begin(), result.candidates.end(),
        [](const auto& left, const auto& right) {
            return std::tie(left.target_component.start, left.target_pc,
                left.caller_pc, left.flow) < std::tie(right.target_component.start,
                right.target_pc, right.caller_pc, right.flow);
        });
    result.candidates.erase(std::unique(result.candidates.begin(), result.candidates.end(),
        [](const auto& left, const auto& right) {
            return left.caller_pc == right.caller_pc && left.target_pc == right.target_pc &&
                left.target_component == right.target_component && left.flow == right.flow &&
                left.fallthrough == right.fallthrough;
        }), result.candidates.end());
    return result;
}

} // namespace oasis::tools
