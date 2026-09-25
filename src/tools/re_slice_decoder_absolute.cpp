#include "tools/re_slice_decoder.hpp"

#include <cstdint>

namespace oasis::tools {

void set_absolute_word_control_target(DecodedInstruction& instruction,
    std::span<const std::uint8_t> rom, std::uint32_t pc, std::uint32_t range_end) {
    if (pc + 4U > range_end || pc + 4U > rom.size()) return;
    const auto word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom[pc + 2U]) << 8U) | rom[pc + 3U]);
    const auto signed_target = static_cast<std::int32_t>(static_cast<std::int16_t>(word));
    instruction.direct_target = static_cast<std::uint32_t>(signed_target) & 0x00FFFFFFU;
    instruction.flow = instruction.mnemonic == "jsr" ? FlowKind::direct_call : FlowKind::direct_jump;
}

} // namespace oasis::tools
