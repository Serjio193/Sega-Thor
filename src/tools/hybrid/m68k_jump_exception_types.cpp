#include "tools/hybrid/m68k_jump_exception_types.hpp"

namespace oasis::hybrid {

JumpResolutionOutcome classify_jump_target(std::uint32_t target_pc, std::uint32_t rom_size) noexcept {
    const std::uint32_t pc24 = target_pc & kM68kBusAddressMask;

    // 1. Odd address generates M68000 Address Error Exception (Vector 3)
    if ((pc24 & 1U) != 0U) {
        return {
            JumpTargetClassification::OriginalM68000Exception,
            M68kExceptionVector::AddressError,
            pc24,
            false
        };
    }

    // 2. Unmapped address space outside ROM and RAM generates Bus Error Exception (Vector 2)
    // Mega Drive memory map: ROM is 0x000000..rom_size, RAM is 0xFF0000..0xFFFFFF
    if (pc24 >= rom_size && pc24 < kWorkRamStart) {
        // Hardware I/O space 0xA00000..0xA1FFFF, VDP 0xC00000..0xC0001F cannot execute instructions
        if (pc24 >= 0x00A00000U && pc24 < 0x00D00000U) {
            return {
                JumpTargetClassification::UnsupportedBusMapping,
                M68kExceptionVector::BusError,
                pc24,
                false
            };
        }
        return {
            JumpTargetClassification::OriginalM68000Exception,
            M68kExceptionVector::BusError,
            pc24,
            false
        };
    }

    // 3. Known compiled targets (experimental slots & production entries)
    switch (pc24) {
    case 0x00E682U: // Slot 239
    case 0x015AE2U: // Slot 240
    case 0x00C9B2U: // Slot 241
    case 0x00D2B4U: // Slot 237
    case 0x0059A8U: // Slot 238
    case 0x006E7AU: // Slot 234
    case 0x00D266U: // Slot 230
    case 0x00D168U: // Slot 229
    case 0x0023CCU: // Slot 228
    case 0x002512U: // Slot 227
    case 0x002FDAU: // Slot 226
    case 0x00A9A0U: // Slot 224
    case 0x00AD76U: // Slot 223
    case 0x02575CU: // Slot 222
        return {
            JumpTargetClassification::KnownCompiledTarget,
            M68kExceptionVector::None,
            pc24,
            false
        };
    default:
        break;
    }

    // 4. Valid uncompiled target inside valid ROM or Work RAM
    // Safe continuation: yield to interpreter at resolved target PC
    return {
        JumpTargetClassification::ValidUncompiledTarget,
        M68kExceptionVector::None,
        pc24,
        true
    };
}

} // namespace oasis::hybrid
