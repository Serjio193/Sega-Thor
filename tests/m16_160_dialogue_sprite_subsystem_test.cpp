#include "game/render/dialogue_sprite_subsystem.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int g_failures = 0;

void expect_true(bool cond, std::string_view msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        ++g_failures;
    }
}

void expect_eq_u32(std::uint32_t a, std::uint32_t b, std::string_view msg) {
    if (a != b) {
        std::cerr << "FAIL: " << msg << " (expected " << b << ", got " << a << ")\n";
        ++g_failures;
    }
}

void expect_eq_i32(std::int32_t a, std::int32_t b, std::string_view msg) {
    if (a != b) {
        std::cerr << "FAIL: " << msg << " (expected " << b << ", got " << a << ")\n";
        ++g_failures;
    }
}

} // namespace

using namespace oasis::game::render;

static void test_rom_constants() {
    expect_eq_u32(DialogueSpriteSubsystemConfig::kDialogueCompositorAddr, 0x00A342U, "kDialogueCompositorAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kPrimaryTemplateAddr, 0x00A438U, "kPrimaryTemplateAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kSecondaryTemplateAddr, 0x00A480U, "kSecondaryTemplateAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kReservationRoutineAddr, 0x00A4C8U, "kReservationRoutineAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorDmaRoutineAddr, 0x00A3DCU, "kCursorDmaRoutineAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kSpecialEntityJumpAddr, 0x00A316U, "kSpecialEntityJumpAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kDemoSpriteHookAddr, 0x00A336U, "kDemoSpriteHookAddr");

    expect_eq_u32(DialogueSpriteSubsystemConfig::kVdpSatBufferRam, 0x00FF13CCU, "kVdpSatBufferRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kSatByteOffsetRam, 0x00FF188CU, "kSatByteOffsetRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kSatLinkCountRam, 0x00FF188AU, "kSatLinkCountRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kDialogueStyleRam, 0x00FF1858U, "kDialogueStyleRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kDialogueOffsetXRam, 0x00FF185AU, "kDialogueOffsetXRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kPortraitActiveRam, 0x00FF184FU, "kPortraitActiveRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorActiveRam, 0x00FF1856U, "kCursorActiveRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorNewFrameRam, 0x00FF1854U, "kCursorNewFrameRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorPrevFrameRam, 0x00FF1855U, "kCursorPrevFrameRam");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam, 0x00FF1892U, "kDmaQueuePtrRam");

    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorRomBaseAddr, 0x000AAEEBU, "kCursorRomBaseAddr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorVramDest, 0xB8E0U, "kCursorVramDest");
    expect_eq_u32(static_cast<std::uint32_t>(DialogueSpriteSubsystemConfig::kCursorEntrySizeBytes), 144U, "kCursorEntrySizeBytes");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorDmaWordLen, 0x0090U, "kCursorDmaWordLen");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorTileAttr, 0x85C7U, "kCursorTileAttr");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorFixedY, 0x0118U, "kCursorFixedY");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorXOffset, 0x0068U, "kCursorXOffset");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kCursorMaxSpriteLimit, 0x0050U, "kCursorMaxSpriteLimit");
    expect_eq_u32(DialogueSpriteSubsystemConfig::kPortraitBaseX, 0x0158U, "kPortraitBaseX");
}

static void test_rom_templates() {
    const auto& pri = get_primary_dialogue_template();
    expect_eq_u32(pri.size(), 9U, "primary template size");
    expect_eq_u32(pri[0].y, 136U, "pri[0].y");
    expect_eq_u32(pri[0].size_link, 0x0901U, "pri[0].size_link");
    expect_eq_u32(pri[0].attr, 0x8781U, "pri[0].attr");
    expect_eq_i32(pri[0].x, 104, "pri[0].x");

    const auto& sec = get_secondary_dialogue_template();
    expect_eq_u32(sec.size(), 9U, "secondary template size");
    expect_eq_u32(sec[0].y, 136U, "sec[0].y");
    expect_eq_u32(sec[0].size_link, 0x0001U, "sec[0].size_link");
    expect_eq_u32(sec[0].attr, 0x8780U, "sec[0].attr");
    expect_eq_i32(sec[0].x, 104, "sec[0].x");
}

static void test_reservations() {
    auto r1 = calculate_dialogue_reservations(false, false);
    expect_eq_u32(r1.byte_count, 48U, "base only bytes");
    expect_eq_u32(r1.sprite_count, 6U, "base only sprites");

    auto r2 = calculate_dialogue_reservations(true, false);
    expect_eq_u32(r2.byte_count, 72U, "base + portrait bytes");
    expect_eq_u32(r2.sprite_count, 9U, "base + portrait sprites");

    auto r3 = calculate_dialogue_reservations(false, true);
    expect_eq_u32(r3.byte_count, 56U, "base + cursor bytes");
    expect_eq_u32(r3.sprite_count, 7U, "base + cursor sprites");

    auto r4 = calculate_dialogue_reservations(true, true);
    expect_eq_u32(r4.byte_count, 80U, "base + portrait + cursor bytes");
    expect_eq_u32(r4.sprite_count, 10U, "base + portrait + cursor sprites");
}

static void test_cursor_dma() {
    auto cmd_same = calculate_cursor_dma_command(2, 2);
    expect_true(!cmd_same.triggered, "dma same frame not triggered");

    auto cmd_f0 = calculate_cursor_dma_command(0, 1);
    expect_true(cmd_f0.triggered, "dma f0 triggered");
    expect_eq_u32(cmd_f0.rom_source_addr, 0x000AAEEBU, "dma f0 addr");
    expect_eq_u32(cmd_f0.vram_dest_addr, 0xB8E0U, "dma f0 dest");
    expect_eq_u32(cmd_f0.word_length, 0x0090U, "dma f0 words");

    auto cmd_f1 = calculate_cursor_dma_command(1, 0);
    expect_true(cmd_f1.triggered, "dma f1 triggered");
    expect_eq_u32(cmd_f1.rom_source_addr, 0x000AAEEBU + 144U, "dma f1 addr");

    auto cmd_f3 = calculate_cursor_dma_command(3, 0);
    expect_eq_u32(cmd_f3.rom_source_addr, 0x000AAEEBU + 3U * 144U, "dma f3 addr");
}

static void test_dialogue_sprite_composition() {
    DialogueSpriteInput input{};
    input.style = 0U; // primary
    input.offset_x = 20;
    input.portrait_active = true;
    input.cursor_active = true;
    input.cursor_new_frame = 2U;
    input.cursor_prev_frame = 1U;
    input.start_sat_offset = 0U;
    input.start_link_count = 0U;

    auto res = compose_dialogue_sprites(input);
    expect_eq_u32(static_cast<std::uint32_t>(res.sprites.size()), 10U, "total emitted sprites");

    // Check first border sprite
    expect_eq_u32(res.sprites[0].y, 136U, "border[0].y");
    expect_eq_u32(res.sprites[0].link, 1U, "border[0].link");
    expect_eq_i32(res.sprites[0].x, 104 + 20, "border[0].x with offset");

    // Check portrait sprite 6
    expect_eq_u32(res.sprites[6].y, 136U, "portrait[0].y");
    expect_eq_u32(res.sprites[6].link, 7U, "portrait[0].link");
    expect_eq_i32(res.sprites[6].x, 104 + (0x158 - 20), "portrait[0].x");

    // Check cursor sprite 9
    expect_eq_u32(res.sprites[9].y, 0x0118U, "cursor.y");
    expect_eq_u32(res.sprites[9].link, 10U, "cursor.link");
    expect_eq_u32(res.sprites[9].attr, 0x85C7U, "cursor.attr");
    expect_eq_i32(res.sprites[9].x, 20 + 0x68, "cursor.x");

    // Check updated state
    expect_eq_u32(res.new_sat_byte_offset, 80U, "new sat offset");
    expect_eq_u32(res.new_sprite_link_count, 10U, "new link count");

    // Check cursor DMA
    expect_true(res.cursor_dma.triggered, "cursor dma triggered");
    expect_eq_u32(res.cursor_dma.rom_source_addr, 0x000AAEEBU + 2U * 144U, "cursor dma rom addr");
}

static void test_cursor_limit_suppression() {
    DialogueSpriteInput input{};
    input.style = 0U;
    input.offset_x = 0;
    input.portrait_active = false;
    input.cursor_active = true;
    input.start_link_count = 79U; // 6 border sprites advance link to 85, which is >= 80

    auto res = compose_dialogue_sprites(input);
    // 6 border sprites emitted, cursor suppressed because link count reaches 85 (>= 80)
    expect_eq_u32(static_cast<std::uint32_t>(res.sprites.size()), 6U, "cursor suppressed over limit");
}

static void test_special_entity_presets() {
    expect_true(evaluate_special_entity_preset(-2, 0x00012345U) == SpecialEntityAction::ExecuteScriptPointer,
                "type -2 with script");
    expect_true(evaluate_special_entity_preset(-2, 0U) == SpecialEntityAction::InvokeDemoSpriteHook,
                "type -2 with null script");
    expect_true(evaluate_special_entity_preset(-1, 0U) == SpecialEntityAction::InvokeDemoSpriteHook,
                "type -1");
    expect_true(evaluate_special_entity_preset(5, 0U) == SpecialEntityAction::None,
                "type 5 positive");
}

static void test_raw_ram_simulation() {
    std::vector<std::uint8_t> ram(0x10000, 0);
    auto ram_off = [](std::uint32_t addr) { return static_cast<std::size_t>(addr - 0x00FF0000U); };

    // Set style = 0, offset_x = 10, portrait_active = 1, cursor_active = 1
    ram[ram_off(DialogueSpriteSubsystemConfig::kDialogueStyleRam)] = 0;
    ram[ram_off(DialogueSpriteSubsystemConfig::kDialogueOffsetXRam)] = 0;
    ram[ram_off(DialogueSpriteSubsystemConfig::kDialogueOffsetXRam) + 1] = 10;
    ram[ram_off(DialogueSpriteSubsystemConfig::kPortraitActiveRam)] = 1;
    ram[ram_off(DialogueSpriteSubsystemConfig::kCursorActiveRam)] = 0;
    ram[ram_off(DialogueSpriteSubsystemConfig::kCursorActiveRam) + 1] = 1;

    // Cursor frames: new = 3, prev = 2
    ram[ram_off(DialogueSpriteSubsystemConfig::kCursorNewFrameRam)] = 3;
    ram[ram_off(DialogueSpriteSubsystemConfig::kCursorPrevFrameRam)] = 2;

    // SAT start: offset = 16, link = 2
    ram[ram_off(DialogueSpriteSubsystemConfig::kSatByteOffsetRam)] = 0;
    ram[ram_off(DialogueSpriteSubsystemConfig::kSatByteOffsetRam) + 1] = 16;
    ram[ram_off(DialogueSpriteSubsystemConfig::kSatLinkCountRam)] = 0;
    ram[ram_off(DialogueSpriteSubsystemConfig::kSatLinkCountRam) + 1] = 2;

    // DMA queue pointer at $FF1892 -> point to $FF18A0
    ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam)] = 0x00;
    ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 1] = 0xFF;
    ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 2] = 0x18;
    ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 3] = 0xA0;

    auto res = compose_dialogue_sprites_raw(ram, 0x00FF0000U);

    expect_eq_u32(static_cast<std::uint32_t>(res.sprites.size()), 10U, "raw sprites count");
    expect_eq_u32(res.new_sat_byte_offset, 16U + 80U, "raw new offset");
    expect_eq_u32(res.new_sprite_link_count, 2U + 10U, "raw new link");

    // Check RAM updated offset and link
    std::uint16_t ram_sat_off = (static_cast<std::uint16_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kSatByteOffsetRam)]) << 8) |
                                static_cast<std::uint16_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kSatByteOffsetRam) + 1]);
    expect_eq_u32(ram_sat_off, 96U, "ram sat off");

    // Check DMA queue updated
    std::uint32_t new_q_ptr = (static_cast<std::uint32_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam)]) << 24) |
                              (static_cast<std::uint32_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 1]) << 16) |
                              (static_cast<std::uint32_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 2]) << 8) |
                              static_cast<std::uint32_t>(ram[ram_off(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam) + 3]);
    expect_eq_u32(new_q_ptr, 0x00FF18A8U, "dma queue pointer advanced by 8");
}

int main() {
    std::cout << "Running M16.160 Dialogue Sprite Subsystem tests...\n";

    test_rom_constants();
    test_rom_templates();
    test_reservations();
    test_cursor_dma();
    test_dialogue_sprite_composition();
    test_cursor_limit_suppression();
    test_special_entity_presets();
    test_raw_ram_simulation();

    if (g_failures == 0) {
        std::cout << "ALL M16.160 TESTS PASSED!\n";
        return 0;
    }

    std::cerr << g_failures << " TEST(S) FAILED!\n";
    return 1;
}
