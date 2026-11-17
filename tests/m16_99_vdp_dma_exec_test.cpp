#include "game/video/vdp_dma_exec.hpp"

#include <cassert>

namespace {

using namespace oasis::game::video;

void test_dma_command_serialization() {
    // src_byte = 0x123456 -> src_word_addr = 0x091A2B
    // vram_dst = 0xF1C0 -> (0xF1C0 & 0x3FFF) | 0x4000 = 0x71C0, (0xF1C0 >> 14) | 0x80 = 0x0083
    // len_words = 0x0123 -> reg94 = 0x9401, reg93 = 0x9323
    const VdpDmaDescriptor desc{0x091A2BU, 0xF1C0U, 0x0123U};
    const auto seq = serialize_vdp_dma_commands(desc);
    assert(seq.reg_words[0] == 0x9401U);
    assert(seq.reg_words[1] == 0x9323U);
    assert(seq.reg_words[2] == 0x952BU);
    assert(seq.reg_words[3] == 0x961AU);
    assert(seq.reg_words[4] == 0x9709U);
    assert(seq.vram_cmd_hi == 0x71C0U);
    assert(seq.vram_cmd_lo == 0x0083U);
}

void test_hud_portrait_dma_dirty_check() {
    HudPortraitDmaState st{};
    st.requested_portrait = 0U;
    st.cached_portrait = 0xFFU;

    const auto d0 = build_hud_portrait_dma(st);
    assert(d0.has_value());
    assert(st.cached_portrait == 0U);
    assert(d0->src_word_addr == (0x000AA3ECU >> 1U));
    assert(d0->vram_dst == 0xF1C0U);
    assert(d0->len_words == 0x0060U);

    // Same portrait again -> clean, no DMA issued
    assert(!build_hud_portrait_dma(st).has_value());

    // Switch to portrait 3 -> 0x000AA3EC + 3 * 0x60 = 0x000AA50C
    st.requested_portrait = 3U;
    assert(hud_portrait_rom_addr(3U) == 0x000AA50CU);
    const auto d3 = build_hud_portrait_dma(st);
    assert(d3.has_value());
    assert(d3->src_word_addr == (0x000AA50CU >> 1U));
    assert(d3->vram_dst == 0xF1C0U);
    assert(d3->len_words == 0x0060U);

    const auto seq3 = serialize_vdp_dma_commands(*d3);
    assert(seq3.reg_words[0] == 0x9400U);
    assert(seq3.reg_words[1] == 0x9360U);
    assert(seq3.vram_cmd_hi == 0x71C0U);
    assert(seq3.vram_cmd_lo == 0x0083U);
}

} // namespace

int main() {
    test_dma_command_serialization();
    test_hud_portrait_dma_dirty_check();
    return 0;
}
