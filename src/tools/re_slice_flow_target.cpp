#include "tools/re_slice_flow_target.hpp"

namespace oasis::tools {
namespace {

std::uint32_t read32(std::span<const std::uint8_t> rom, std::size_t offset) {
    const auto high = (static_cast<std::uint32_t>(rom[offset]) << 8U) | rom[offset + 1U];
    const auto low = (static_cast<std::uint32_t>(rom[offset + 2U]) << 8U) | rom[offset + 3U];
    return (high << 16U) | low;
}

MemoryKind memory_kind(std::uint32_t address) {
    if (address < 0x00400000U) return MemoryKind::rom;
    if (address >= 0x00FF0000U && address <= 0x00FFFFFFU) return MemoryKind::ram;
    return MemoryKind::other;
}

} // namespace

bool set_absolute_long_control_target(std::span<const std::uint8_t> rom,
                                      std::uint32_t pc,
                                      std::uint32_t range_end, bool is_call,
                                      DecodedInstruction& instruction) {
    if (pc + 6U > rom.size() || pc + 6U > range_end) return false;
    instruction.direct_target = read32(rom, pc + 2U);
    instruction.flow = is_call ? FlowKind::direct_call : FlowKind::direct_jump;
    instruction.memory_references.push_back(
        {*instruction.direct_target, 0U, memory_kind(*instruction.direct_target),
         MemoryAccess::address});
    DecodedOperand operand{};
    operand.kind = OperandKind::absolute_long;
    operand.width_bytes = 4U;
    operand.extension_bytes = 4U;
    operand.value = *instruction.direct_target;
    operand.extension_address = pc + 2U;
    instruction.effective_operands.push_back(operand);
    return true;
}

bool set_direct_target_from_effective_address(DecodedInstruction& instruction,
                                              unsigned mode, unsigned reg,
                                              bool is_call) {
    if (mode != 7U || (reg != 0U && reg != 2U) ||
        instruction.effective_operands.empty()) {
        return false;
    }

    const auto& operand = instruction.effective_operands.back();
    const auto target = reg == 0U
        ? static_cast<std::uint32_t>(
              static_cast<std::int32_t>(static_cast<std::int16_t>(operand.value)) &
              0x00FFFFFF)
        : static_cast<std::uint32_t>(
              (operand.extension_address + operand.displacement) & 0x00FFFFFFU);
    instruction.direct_target = target;
    instruction.flow = is_call ? FlowKind::direct_call : FlowKind::direct_jump;
    return true;
}

} // namespace oasis::tools
