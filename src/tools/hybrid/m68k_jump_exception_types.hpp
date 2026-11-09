#ifndef OASIS_TOOLS_HYBRID_M68K_JUMP_EXCEPTION_TYPES_HPP_
#define OASIS_TOOLS_HYBRID_M68K_JUMP_EXCEPTION_TYPES_HPP_

#include <cstdint>

namespace oasis::hybrid {

inline constexpr std::uint32_t kM68kBusAddressMask = 0x00FFFFFFU;
inline constexpr std::uint32_t kCanonicalRomSize   = 0x00400000U; // 4MB
inline constexpr std::uint32_t kWorkRamStart       = 0x00FF0000U;
inline constexpr std::uint32_t kWorkRamEnd         = 0x01000000U; // 64KB

enum class JumpTargetClassification : std::uint8_t {
    KnownCompiledTarget,         // Target is within our compiled/recompiled clusters
    ValidUncompiledTarget,       // Target is valid ROM/RAM code address, but not compiled -> safe yield to interpreter
    OriginalM68000Exception,     // Target triggers authentic CPU exception (Address Error if odd, Bus Error if unmapped)
    UnsupportedBusMapping,       // Access to invalid hardware bus space
    HybridSafetyContractFault    // Internal recompiler contract violation (e.g. re-entry while active, corrupted SP)
};

enum class M68kExceptionVector : std::uint8_t {
    None = 0,
    BusError = 2,
    AddressError = 3,
    IllegalInstruction = 4,
    ZeroDivide = 5
};

struct JumpResolutionOutcome {
    JumpTargetClassification classification{JumpTargetClassification::HybridSafetyContractFault};
    M68kExceptionVector exception_vector{M68kExceptionVector::None};
    std::uint32_t resolved_pc{0};
    bool yield_to_interpreter{false};
};

JumpResolutionOutcome classify_jump_target(std::uint32_t target_pc, std::uint32_t rom_size = kCanonicalRomSize) noexcept;

} // namespace oasis::hybrid

#endif // OASIS_TOOLS_HYBRID_M68K_JUMP_EXCEPTION_TYPES_HPP_
