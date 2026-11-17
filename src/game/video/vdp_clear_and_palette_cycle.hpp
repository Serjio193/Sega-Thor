#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::video {

inline constexpr std::uint32_t kVdpClearEntryAddr = 0x002B28U;
inline constexpr std::uint32_t kVdpDmaFillAddr = 0x002BD2U;
inline constexpr std::uint16_t kVdpDmaAutoIncReg = 0x8F01U;
inline constexpr std::uint16_t kVdpDmaModeFill = 0x9780U;
inline constexpr std::uint32_t kArmletGlowAddr = 0x008DD4U;
inline constexpr std::uint32_t kPaletteCyclerAddr = 0x008E22U;
inline constexpr std::uint8_t kArmletPulseThreshold = 0x14U; // 20

/// Single VDP region cleared by 0x002B28.
struct VdpClearRegion {
    const char* name;
    std::uint32_t vdp_cmd;
    std::uint16_t addr;
    std::uint16_t size_value; // words for VSRAM, bytes_minus_1 for VRAM fills
    bool is_words;
};

inline constexpr std::array<VdpClearRegion, 6> kVdpClearRegions = {{
    {"hscroll",   0x5C000003U, 0xDC00U, 1023U, false},
    {"vsram",     0x40000010U, 0x0000U,   40U, true},
    {"tile_zero", 0x40000000U, 0x0000U,   31U, false},
    {"sat",       0x50000003U, 0xD000U,  639U, false},
    {"plane_a",   0x40000003U, 0xC000U, 4095U, false},
    {"plane_b",   0x60000003U, 0xE000U, 4095U, false},
}};

/// Models 0x008DD4: ramps $FF183E 0..3 when spirit_active, pulses 3/4 when
/// timer_ff199e >= 0x14 and (pulse_ff199f & 1) == 0, and decays 3..0 (>=4 -> 2)
/// when inactive.
[[nodiscard]] std::uint8_t step_armlet_glow(
    std::uint8_t glow,
    bool spirit_active,
    std::uint8_t timer_ff199e,
    std::uint8_t pulse_ff199f) noexcept;

/// Single (duration, bgr333) keyframe in a palette cycle list.
struct PaletteCycleKeyframe {
    std::uint16_t duration{0};
    std::uint16_t bgr333{0};
};

/// One active palette cycle entry from the $FF17A2 linked list.
struct PaletteCycleEntry {
    std::uint16_t cram_offset{0}; // byte offset into $FF134C (0..126)
    std::vector<PaletteCycleKeyframe> keyframes{};
    std::size_t cursor{0};
    std::uint16_t timer{1};
};

/// Models 0x008E22 CRAM palette cycler.
struct PaletteCycler {
    bool enabled{true}; // $FF164F
    std::vector<PaletteCycleEntry> entries{};

    /// Advances active palette cycle entries by 1 frame, updating cram_shadow
    /// and returning true if any CRAM entry was modified.
    bool step_frame(std::span<std::uint16_t, 64> cram_shadow) noexcept;
};

} // namespace oasis::game::video
