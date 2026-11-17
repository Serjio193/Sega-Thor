#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::video {

enum class PaletteFadeDirection : std::uint8_t {
    None = 0,
    FadeIn,  // Steps 0..7 (0x002D84 / 0x002DB6)
    FadeOut, // Steps 7..0 (0x002E1E / 0x002E78)
};

struct PaletteFadeConfig {
    static constexpr std::size_t kCramColorCount = 64U;
    static constexpr std::uint8_t kChannelDivisor = 7U;
    static constexpr std::uint16_t kMaskR = 0x000EU;
    static constexpr std::uint16_t kMaskG = 0x00E0U;
    static constexpr std::uint16_t kMaskB = 0x0E00U;
    static constexpr std::uint8_t kFlagCramDmaDirty = 0x01U;  // $FF164D bit 0
    static constexpr std::uint8_t kFlagDisplayUnblank = 0x02U; // $FF164D bit 1
};

// ROM 0x002FDA: Scales 9-bit BGR333 color `c` by `step & 7` across R/G/B nibbles.
[[nodiscard]] std::uint16_t scale_bgr333_color(std::uint16_t color, std::uint8_t step) noexcept;

// ROM 0x002D58..0x00302D: 64-color CRAM shadow ($FF134C) & backup ($FF18A6) fade engine.
class PaletteFadeEngine {
public:
    void reset() noexcept;

    void set_palette(const std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount>& colors) noexcept;

    // Fade In: steps 0..7 with `delay + 1` frames per step; unblanks ($FF164D bit 1) on step 0.
    void start_fade_in(std::uint8_t delay = 0U) noexcept;

    // Fade Out: steps 7..0 with `delay + 1` frames per step; restores backup -> shadow on completion.
    void start_fade_out(std::uint8_t delay = 0U) noexcept;

    // Advances fade by 1 frame. Returns true while fade remains active, false when idle/complete.
    bool step_frame() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return direction_ != PaletteFadeDirection::None; }
    [[nodiscard]] PaletteFadeDirection direction() const noexcept { return direction_; }
    [[nodiscard]] std::uint8_t current_step() const noexcept { return step_; }
    [[nodiscard]] std::uint8_t vdp_flags_ff164d() const noexcept { return vdp_flags_ff164d_; }
    void clear_cram_dma_dirty() noexcept { vdp_flags_ff164d_ &= static_cast<std::uint8_t>(~PaletteFadeConfig::kFlagCramDmaDirty); }

    [[nodiscard]] const std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount>& cram_shadow() const noexcept {
        return cram_shadow_;
    }
    [[nodiscard]] const std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount>& fade_backup() const noexcept {
        return fade_backup_;
    }

private:
    void apply_scaled_step(std::uint8_t step) noexcept;

    std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount> cram_shadow_{}; // $FF134C
    std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount> fade_backup_{}; // $FF18A6
    PaletteFadeDirection direction_{PaletteFadeDirection::None};
    std::uint8_t step_{0U};          // $FF189A
    std::uint8_t delay_{0U};         // $FF1898
    std::uint8_t delay_counter_{0U}; // $FF1896
    std::uint8_t vdp_flags_ff164d_{0U};
};

} // namespace oasis::game::video
