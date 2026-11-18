#include "game/dialogue/dialogue_task_compositor.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::dialogue;

static void test_constants_and_addresses() {
    static_assert(DialogueTaskCompositorConfig::kEntryTaskCompositorAddr == 0x00C756U);
    static_assert(DialogueTaskCompositorConfig::kEntryEmitLineSpritesAddr == 0x00C81CU);
    static_assert(DialogueTaskCompositorConfig::kEntryEmitSatPieceAddr == 0x00C838U);
    static_assert(DialogueTaskCompositorConfig::kEntryFrameSyncRenderAddr == 0x00C84EU);
    static_assert(DialogueTaskCompositorConfig::kRangeEndAddr == 0x00C88CU);
    static_assert(DialogueTaskCompositorConfig::kRoutineSizeBytes == 310U);

    static_assert(DialogueTaskCompositorConfig::kContinueArrowTile == 0x86DCU);
    static_assert(DialogueTaskCompositorConfig::kDismissArrowTile == 0x96DCU);
    static_assert(DialogueTaskCompositorConfig::kArrowTileDismissMask == 0x1000U);
    static_assert(DialogueTaskCompositorConfig::kArrowX == 284);
    static_assert(DialogueTaskCompositorConfig::kArrowYBase == 332);
    static_assert(DialogueTaskCompositorConfig::kArrowYStep == 12);

    static_assert(DialogueTaskCompositorConfig::kSpritesPerRow == 8U);
    static_assert(DialogueTaskCompositorConfig::kLineSpriteSizeCode == 0x0C00U);
    static_assert(DialogueTaskCompositorConfig::kLineStartX == 160);
    static_assert(DialogueTaskCompositorConfig::kLineStepX == 32);
    static_assert(DialogueTaskCompositorConfig::kLineYBias == 272);
    static_assert(DialogueTaskCompositorConfig::kLineTileStep == 4U);

    static_assert(DialogueTaskCompositorConfig::kHardwareSpriteLimit == 80U);

    static_assert(DialogueTaskCompositorConfig::kSatBufferBaseRam == 0x00FF13CCU);
    static_assert(DialogueTaskCompositorConfig::kSatBufferOffsetRam == 0x00FF188CU);
    static_assert(DialogueTaskCompositorConfig::kSatLinkCounterRam == 0x00FF188AU);
    static_assert(DialogueTaskCompositorConfig::kPromptIconActiveRam == 0x00FF1886U);
    static_assert(DialogueTaskCompositorConfig::kPromptDismissFlagRam == 0x00FF1887U);
    static_assert(DialogueTaskCompositorConfig::kAnimationTimerRam == 0x00FF1657U);
    static_assert(DialogueTaskCompositorConfig::kLineCounterRam == 0x00FF1880U);
    static_assert(DialogueTaskCompositorConfig::kPlaneTopBaseRam == 0x00FF1872U);
    static_assert(DialogueTaskCompositorConfig::kPlaneBottomBaseRam == 0x00FF1874U);
    static_assert(DialogueTaskCompositorConfig::kLine1RowOffsetRam == 0x00FF1876U);
    static_assert(DialogueTaskCompositorConfig::kLine2RowOffsetRam == 0x00FF1878U);
    static_assert(DialogueTaskCompositorConfig::kLine3RowOffsetRam == 0x00FF187AU);
    static_assert(DialogueTaskCompositorConfig::kLine4RowOffsetRam == 0x00FF187CU);
    static_assert(DialogueTaskCompositorConfig::kRenderInProgressFlagRam == 0x00FF185DU);
    static_assert(DialogueTaskCompositorConfig::kFrameSyncCadenceRam == 0x00FF1654U);
    static_assert(DialogueTaskCompositorConfig::kVintStatusFlagsRam == 0x00FF164EU);

    std::cout << "[PASS] test_constants_and_addresses\n";
}

static void test_prompt_arrow_y_calculation() {
    assert(calculate_prompt_arrow_y(4) == 332);
    assert(calculate_prompt_arrow_y(3) == 332);
    assert(calculate_prompt_arrow_y(2) == 320);
    assert(calculate_prompt_arrow_y(1) == 308);
    assert(calculate_prompt_arrow_y(0) == 296);
    assert(calculate_prompt_arrow_y(-1) == 296);
    assert(calculate_prompt_arrow_y(-10) == 296);

    std::cout << "[PASS] test_prompt_arrow_y_calculation\n";
}

static void test_prompt_arrow_emission() {
    CompositorConfig config;
    config.plane_top_base = 0x2000U;
    config.plane_bottom_base = 0x4000U;
    config.row_offsets = {12, 24, 36, 48};
    config.line_counter = 3;

    // Case 1: Prompt inactive -> only 32 line pieces emitted
    config.prompt_active = false;
    config.anim_timer_visible = true;
    std::uint16_t link = 0U;
    std::vector<SatPiece> pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 32U);
    assert(link == 32U);

    // Case 2: Prompt active but blink invisible -> no arrow
    config.prompt_active = true;
    config.anim_timer_visible = false;
    link = 0U;
    pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 32U);
    assert(link == 32U);

    // Case 3: Prompt active & visible (continue mode) -> 33 pieces, first piece is arrow
    config.prompt_active = true;
    config.anim_timer_visible = true;
    config.prompt_dismiss = false;
    config.line_counter = 2; // Y = 320
    link = 0U;
    pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 33U);
    assert(link == 33U);
    assert(pieces[0].y == 320);
    assert(pieces[0].link_size == 1U); // size 0 | link 1
    assert(pieces[0].tile == 0x86DCU);
    assert(pieces[0].x == 284);

    // Case 4: Dismiss mode -> tile has bit 12 set (0x96DC)
    config.prompt_dismiss = true;
    config.line_counter = 0; // Y = 296
    link = 0U;
    pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 33U);
    assert(pieces[0].y == 296);
    assert(pieces[0].tile == 0x96DCU);

    std::cout << "[PASS] test_prompt_arrow_emission\n";
}

static void test_window_text_line_sprites() {
    CompositorConfig config;
    config.plane_top_base = 0x2000U;
    config.plane_bottom_base = 0x4000U;
    config.row_offsets = {12, 24, 36, 48};
    config.line_counter = 3;
    config.prompt_active = false;
    config.anim_timer_visible = true;

    std::uint16_t link = 0U;
    const std::vector<SatPiece> pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 32U);
    assert(link == 32U);

    const std::uint16_t top_base_tile = static_cast<std::uint16_t>((0x2000U >> 5) | 0x8000U);       // 0x8100
    const std::uint16_t bottom_base_tile = static_cast<std::uint16_t>((0x4000U >> 5) | 0x8000U);    // 0x8200

    // Check Line 0 (pieces 0..7)
    for (std::size_t i = 0; i < 8; ++i) {
        const auto& p = pieces[i];
        assert(p.y == 12 + 272); // 284
        assert(p.x == static_cast<std::int16_t>(160 + i * 32));
        assert(p.tile == static_cast<std::uint16_t>(top_base_tile + i * 4));
        assert(p.link_size == static_cast<std::uint16_t>(0x0C00U + (i + 1)));
    }

    // Check Line 1 (pieces 8..15)
    for (std::size_t i = 0; i < 8; ++i) {
        const auto& p = pieces[8 + i];
        assert(p.y == 24 + 272); // 296
        assert(p.x == static_cast<std::int16_t>(160 + i * 32));
        assert(p.tile == static_cast<std::uint16_t>(top_base_tile + i * 4));
        assert(p.link_size == static_cast<std::uint16_t>(0x0C00U + (8 + i + 1)));
    }

    // Check Line 2 (pieces 16..23)
    for (std::size_t i = 0; i < 8; ++i) {
        const auto& p = pieces[16 + i];
        assert(p.y == 36 + 272); // 308
        assert(p.x == static_cast<std::int16_t>(160 + i * 32));
        assert(p.tile == static_cast<std::uint16_t>(bottom_base_tile + i * 4));
        assert(p.link_size == static_cast<std::uint16_t>(0x0C00U + (16 + i + 1)));
    }

    // Check Line 3 (pieces 24..31)
    for (std::size_t i = 0; i < 8; ++i) {
        const auto& p = pieces[24 + i];
        assert(p.y == 48 + 272); // 320
        assert(p.x == static_cast<std::int16_t>(160 + i * 32));
        assert(p.tile == static_cast<std::uint16_t>(bottom_base_tile + i * 4));
        assert(p.link_size == static_cast<std::uint16_t>(0x0C00U + (24 + i + 1)));
    }

    std::cout << "[PASS] test_window_text_line_sprites\n";
}

static void test_hardware_sprite_limit_80_clamp() {
    CompositorConfig config;
    config.plane_top_base = 0x2000U;
    config.plane_bottom_base = 0x4000U;
    config.row_offsets = {12, 24, 36, 48};
    config.line_counter = 3;
    config.prompt_active = true;
    config.anim_timer_visible = true;

    // Start with link = 75. 33 requested pieces -> only 5 should be accepted (links 76..80).
    std::uint16_t link = 75U;
    std::vector<SatPiece> pieces = composite_dialogue_sprites(config, link);
    assert(pieces.size() == 5U);
    assert(link == 80U);

    // If starting with link = 80 -> 0 pieces accepted
    link = 80U;
    pieces = composite_dialogue_sprites(config, link);
    assert(pieces.empty());
    assert(link == 80U);

    std::cout << "[PASS] test_hardware_sprite_limit_80_clamp\n";
}

static void test_frame_sync_and_render() {
    FrameSyncState state;
    state.render_in_progress = false;
    state.frame_sync_cadence = 0U; // cadence uninitialized
    state.vint_status_flags = 0x00U; // bit 0 clear (V-Int arrived)

    bool pal_called = false;
    bool rnd_called = false;
    bool post_called = false;

    step_frame_sync_and_render(
        state,
        [&]() { pal_called = true; },
        [&]() { rnd_called = true; },
        [&]() { post_called = true; });

    assert(state.frame_sync_cadence == 2U);
    assert(state.palette_ticks == 1U);
    assert(state.render_passes == 1U);
    assert(state.post_render_passes == 1U);
    assert(pal_called && rnd_called && post_called);
    assert(!state.render_in_progress);

    // Second step: cadence already 2 -> skip palette and render
    pal_called = false;
    rnd_called = false;
    post_called = false;
    state.vint_status_flags = 0x01U; // bit 0 set (V-Int pending)

    step_frame_sync_and_render(
        state,
        [&]() { pal_called = true; },
        [&]() { rnd_called = true; },
        [&]() { post_called = true; });

    assert(state.frame_sync_cadence == 2U);
    assert(state.palette_ticks == 1U);
    assert(state.render_passes == 1U);
    assert(state.post_render_passes == 1U); // not incremented because vint bit 0 set
    assert(!pal_called && !rnd_called && !post_called);

    std::cout << "[PASS] test_frame_sync_and_render\n";
}

static void test_ram_integration() {
    std::vector<std::uint8_t> ram(0x10000U, 0U);

    auto write_be16 = [&](std::uint32_t addr, std::uint16_t val) {
        const std::size_t off = addr & 0xFFFFU;
        ram[off] = static_cast<std::uint8_t>(val >> 8);
        ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    };

    auto read_be16 = [&](std::uint32_t addr) -> std::uint16_t {
        const std::size_t off = addr & 0xFFFFU;
        return static_cast<std::uint16_t>((ram[off] << 8) | ram[off + 1U]);
    };

    // Initialize RAM registers
    write_be16(DialogueTaskCompositorConfig::kPlaneTopBaseRam, 0x1000U);
    write_be16(DialogueTaskCompositorConfig::kPlaneBottomBaseRam, 0x2000U);
    write_be16(DialogueTaskCompositorConfig::kLine1RowOffsetRam, 10U);
    write_be16(DialogueTaskCompositorConfig::kLine2RowOffsetRam, 20U);
    write_be16(DialogueTaskCompositorConfig::kLine3RowOffsetRam, 30U);
    write_be16(DialogueTaskCompositorConfig::kLine4RowOffsetRam, 40U);
    write_be16(DialogueTaskCompositorConfig::kLineCounterRam, 1U);

    ram[DialogueTaskCompositorConfig::kPromptIconActiveRam & 0xFFFFU] = 1U;
    ram[DialogueTaskCompositorConfig::kPromptDismissFlagRam & 0xFFFFU] = 0U;
    ram[DialogueTaskCompositorConfig::kAnimationTimerRam & 0xFFFFU] = 0x10U; // bit 4 set

    write_be16(DialogueTaskCompositorConfig::kSatBufferOffsetRam, 0x0020U);
    write_be16(DialogueTaskCompositorConfig::kSatLinkCounterRam, 4U);

    composite_dialogue_sprites_raw(ram);

    // Initial link 4 -> emits 33 pieces -> final link 37
    assert(read_be16(DialogueTaskCompositorConfig::kSatLinkCounterRam) == 37U);
    // Initial offset 0x20 + 33 * 8 (264 = 0x108) = 0x0128
    assert(read_be16(DialogueTaskCompositorConfig::kSatBufferOffsetRam) == 0x0128U);

    // First piece is prompt arrow at $FF13CC + 0x20 = $FF13EC
    const std::uint32_t first_piece_addr = DialogueTaskCompositorConfig::kSatBufferBaseRam + 0x20U;
    assert(read_be16(first_piece_addr) == 308U);     // line_counter 1 -> Y=308
    assert(read_be16(first_piece_addr + 2U) == 5U);   // size 0 + link 5
    assert(read_be16(first_piece_addr + 4U) == 0x86DCU); // continue tile
    assert(read_be16(first_piece_addr + 6U) == 284U); // X=284

    // Test raw frame sync
    ram[DialogueTaskCompositorConfig::kVintStatusFlagsRam & 0xFFFFU] = 0x00U;
    write_be16(DialogueTaskCompositorConfig::kFrameSyncCadenceRam, 0U);

    bool tick_run = false;
    step_frame_sync_raw(
        ram,
        [&]() { tick_run = true; },
        {},
        {});

    assert(tick_run);
    assert(read_be16(DialogueTaskCompositorConfig::kFrameSyncCadenceRam) == 2U);

    std::cout << "[PASS] test_ram_integration\n";
}

static void test_thor_namespace_aliases() {
    thor::dialogue::CompositorConfig conf;
    conf.line_counter = 3;
    conf.prompt_active = false;
    conf.anim_timer_visible = true;
    std::uint16_t link = 0U;

    std::vector<thor::dialogue::SatPiece> pieces = thor::dialogue::composite_dialogue_sprites(conf, link);
    assert(pieces.size() == 32U);
    assert(link == 32U);

    assert(thor::dialogue::calculate_prompt_arrow_y(3) == 332);

    std::cout << "[PASS] test_thor_namespace_aliases\n";
}

int main() {
    std::cout << "Starting M16.179 Dialogue Task Compositor tests...\n";
    test_constants_and_addresses();
    test_prompt_arrow_y_calculation();
    test_prompt_arrow_emission();
    test_window_text_line_sprites();
    test_hardware_sprite_limit_80_clamp();
    test_frame_sync_and_render();
    test_ram_integration();
    test_thor_namespace_aliases();
    std::cout << "All M16.179 Dialogue Task Compositor tests PASS!\n";
    return 0;
}
