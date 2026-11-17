#include "game/video/palette_fade.hpp"

namespace oasis::game::video {

std::uint16_t scale_bgr333_color(std::uint16_t color, std::uint8_t step) noexcept {
    const std::uint32_t s = step & 7U;
    const std::uint32_t r = (((color & PaletteFadeConfig::kMaskR) * s) / PaletteFadeConfig::kChannelDivisor) & PaletteFadeConfig::kMaskR;
    const std::uint32_t g = (((color & PaletteFadeConfig::kMaskG) * s) / PaletteFadeConfig::kChannelDivisor) & PaletteFadeConfig::kMaskG;
    const std::uint32_t b = (((color & PaletteFadeConfig::kMaskB) * s) / PaletteFadeConfig::kChannelDivisor) & PaletteFadeConfig::kMaskB;
    return static_cast<std::uint16_t>(r | g | b);
}

void PaletteFadeEngine::reset() noexcept {
    cram_shadow_.fill(0U);
    fade_backup_.fill(0U);
    direction_ = PaletteFadeDirection::None;
    step_ = 0U;
    delay_ = 0U;
    delay_counter_ = 0U;
    vdp_flags_ff164d_ = 0U;
}

void PaletteFadeEngine::set_palette(
    const std::array<std::uint16_t, PaletteFadeConfig::kCramColorCount>& colors) noexcept {
    cram_shadow_ = colors;
    fade_backup_ = colors;
}

void PaletteFadeEngine::apply_scaled_step(std::uint8_t step) noexcept {
    for (std::size_t i = 0U; i < PaletteFadeConfig::kCramColorCount; ++i) {
        cram_shadow_[i] = scale_bgr333_color(fade_backup_[i], step);
    }
    vdp_flags_ff164d_ |= PaletteFadeConfig::kFlagCramDmaDirty;
}

void PaletteFadeEngine::start_fade_in(std::uint8_t delay) noexcept {
    fade_backup_ = cram_shadow_;
    direction_ = PaletteFadeDirection::FadeIn;
    step_ = 0U;
    delay_ = delay;
    delay_counter_ = 0U;
    vdp_flags_ff164d_ |= PaletteFadeConfig::kFlagDisplayUnblank;
    apply_scaled_step(0U);
}

void PaletteFadeEngine::start_fade_out(std::uint8_t delay) noexcept {
    fade_backup_ = cram_shadow_;
    direction_ = PaletteFadeDirection::FadeOut;
    step_ = 7U;
    delay_ = delay;
    delay_counter_ = 0U;
    apply_scaled_step(7U);
}

bool PaletteFadeEngine::step_frame() noexcept {
    if (direction_ == PaletteFadeDirection::None) {
        return false;
    }

    if (delay_counter_ < delay_) {
        ++delay_counter_;
        return true;
    }
    delay_counter_ = 0U;

    if (direction_ == PaletteFadeDirection::FadeIn) {
        if (step_ < 7U) {
            ++step_;
            apply_scaled_step(step_);
            return true;
        }
        direction_ = PaletteFadeDirection::None;
        return false;
    }

    // FadeOut: steps 7..0, restoring $FF18A6 -> $FF134C on completion.
    if (step_ > 0U) {
        --step_;
        apply_scaled_step(step_);
        return true;
    }
    cram_shadow_ = fade_backup_;
    vdp_flags_ff164d_ |= PaletteFadeConfig::kFlagCramDmaDirty;
    direction_ = PaletteFadeDirection::None;
    return false;
}

} // namespace oasis::game::video
