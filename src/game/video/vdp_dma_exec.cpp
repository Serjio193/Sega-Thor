#include "game/video/vdp_dma_exec.hpp"

namespace oasis::game::video {

VdpDmaCommandSequence serialize_vdp_dma_commands(
    const VdpDmaDescriptor& desc) noexcept {
    VdpDmaCommandSequence seq{};
    seq.reg_words[0] = static_cast<std::uint16_t>(
        0x9400U | ((desc.len_words >> 8U) & 0xFFU));
    seq.reg_words[1] = static_cast<std::uint16_t>(
        0x9300U | (desc.len_words & 0xFFU));
    seq.reg_words[2] = static_cast<std::uint16_t>(
        0x9500U | (desc.src_word_addr & 0xFFU));
    seq.reg_words[3] = static_cast<std::uint16_t>(
        0x9600U | ((desc.src_word_addr >> 8U) & 0xFFU));
    seq.reg_words[4] = static_cast<std::uint16_t>(
        0x9700U | ((desc.src_word_addr >> 16U) & 0xFFU));
    seq.vram_cmd_hi = static_cast<std::uint16_t>((desc.vram_dst & 0x3FFFU) | 0x4000U);
    seq.vram_cmd_lo = static_cast<std::uint16_t>(((desc.vram_dst >> 14U) & 3U) | 0x0080U);
    return seq;
}

std::optional<VdpDmaDescriptor> build_hud_portrait_dma(
    HudPortraitDmaState& state) noexcept {
    if (state.requested_portrait == state.cached_portrait) {
        return std::nullopt;
    }
    state.cached_portrait = state.requested_portrait;
    return VdpDmaDescriptor{
        hud_portrait_rom_addr(state.requested_portrait) >> 1U,
        kHudPortraitVramDst,
        kHudPortraitLenWords,
    };
}

} // namespace oasis::game::video
