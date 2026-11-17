#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace oasis::game::video {

inline constexpr std::uint32_t kHudPortraitDmaAddr = 0x002632U;
inline constexpr std::uint32_t kVdpDmaExecRegsAddr = 0x0027A6U;
inline constexpr std::uint32_t kVdpDmaExecStructAddr = 0x0027FCU;

inline constexpr std::uint32_t kHudPortraitTableRom = 0x000AA3ECU;
inline constexpr std::uint32_t kHudPortraitStrideBytes = 0x60U; // 96
inline constexpr std::uint16_t kHudPortraitVramDst = 0xF1C0U;
inline constexpr std::uint16_t kHudPortraitLenWords = 0x0060U;

inline constexpr std::uint32_t kZ80BusReqPort = 0x00A11100U;
inline constexpr std::uint32_t kVdpDataPort = 0x00C00000U;
inline constexpr std::uint32_t kVdpCtrlPort = 0x00C00004U;
inline constexpr std::uint32_t kVdpReg1ShadowRam = 0x00FF0BF4U;
inline constexpr std::uint8_t kVdpReg1DmaEnableBit = 4U;
inline constexpr std::uint16_t kIrqMaskSr = 0x0700U;

/// 8-byte VDP DMA descriptor consumed by 0x0027FC.
struct VdpDmaDescriptor {
    std::uint32_t src_word_addr{0}; // +0 (byte_addr >> 1)
    std::uint16_t vram_dst{0};      // +4
    std::uint16_t len_words{0};     // +6
};

/// Serialized VDP register writes (0x94..0x97) and VRAM DMA command words.
struct VdpDmaCommandSequence {
    std::array<std::uint16_t, 5> reg_words{};
    std::uint16_t vram_cmd_hi{0}; // (vram_dst & 0x3FFF) | 0x4000
    std::uint16_t vram_cmd_lo{0}; // ((vram_dst >> 14) & 3) | 0x0080
};

/// Models 0x0027FC..0x002896: serializes DMA length, source word address,
/// and VRAM DMA command words.
[[nodiscard]] VdpDmaCommandSequence serialize_vdp_dma_commands(
    const VdpDmaDescriptor& desc) noexcept;

struct HudPortraitDmaState {
    std::uint8_t requested_portrait{0}; // $FF183E
    std::uint8_t cached_portrait{0xFFU};// $FF183F
};

/// Computes ROM byte address 0x000AA3EC + portrait * 0x60.
[[nodiscard]] constexpr std::uint32_t hud_portrait_rom_addr(
    std::uint8_t portrait) noexcept {
    return kHudPortraitTableRom +
           static_cast<std::uint32_t>(portrait) * kHudPortraitStrideBytes;
}

/// Models 0x002632..0x002678: if requested_portrait != cached_portrait, updates
/// cached_portrait and returns the DMA descriptor for VRAM 0xF1C0 (0x60 words).
[[nodiscard]] std::optional<VdpDmaDescriptor> build_hud_portrait_dma(
    HudPortraitDmaState& state) noexcept;

} // namespace oasis::game::video
