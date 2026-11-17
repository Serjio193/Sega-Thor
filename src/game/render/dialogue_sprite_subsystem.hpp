#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

/// Configuration and constants for Dialogue & HUD Sprite SAT Subsystem (0x00A2F0..0x00A524).
struct DialogueSpriteSubsystemConfig {
    static constexpr std::uint32_t kDialogueCompositorAddr = 0x00A342U;
    static constexpr std::uint32_t kPrimaryTemplateAddr = 0x00A438U;
    static constexpr std::uint32_t kSecondaryTemplateAddr = 0x00A480U;
    static constexpr std::uint32_t kReservationRoutineAddr = 0x00A4C8U;
    static constexpr std::uint32_t kCursorDmaRoutineAddr = 0x00A3DCU;
    static constexpr std::uint32_t kSpecialEntityJumpAddr = 0x00A316U;
    static constexpr std::uint32_t kDemoSpriteHookAddr = 0x00A336U;

    // RAM variables
    static constexpr std::uint32_t kVdpSatBufferRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatByteOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kSatLinkCountRam = 0x00FF188AU;
    static constexpr std::uint32_t kDialogueStyleRam = 0x00FF1858U;
    static constexpr std::uint32_t kDialogueOffsetXRam = 0x00FF185AU;
    static constexpr std::uint32_t kPortraitActiveRam = 0x00FF184FU;
    static constexpr std::uint32_t kCursorActiveRam = 0x00FF1856U;
    static constexpr std::uint32_t kCursorNewFrameRam = 0x00FF1854U;
    static constexpr std::uint32_t kCursorPrevFrameRam = 0x00FF1855U;
    static constexpr std::uint32_t kDmaQueuePtrRam = 0x00FF1892U;

    // Cursor DMA parameters
    static constexpr std::uint32_t kCursorRomBaseAddr = 0x000AAEEBU;
    static constexpr std::uint16_t kCursorVramDest = 0xB8E0U;
    static constexpr std::size_t kCursorEntrySizeBytes = 144U; // 9 * 16 bytes
    static constexpr std::uint16_t kCursorDmaWordLen = 0x0090U; // 144 words (288 bytes)
    static constexpr std::uint16_t kCursorTileAttr = 0x85C7U;
    static constexpr std::uint16_t kCursorFixedY = 0x0118U; // 280
    static constexpr std::uint16_t kCursorXOffset = 0x0068U; // 104
    static constexpr std::uint16_t kCursorMaxSpriteLimit = 0x0050U; // 80 sprites

    // Portrait positioning
    static constexpr std::uint16_t kPortraitBaseX = 0x0158U; // 344

    // Sprite reservations
    static constexpr std::uint16_t kBaseFrameBytes = 48U;
    static constexpr std::uint16_t kBaseFrameSprites = 6U;
    static constexpr std::uint16_t kPortraitBytes = 24U;
    static constexpr std::uint16_t kPortraitSprites = 3U;
    static constexpr std::uint16_t kCursorBytes = 8U;
    static constexpr std::uint16_t kCursorSprites = 1U;

    // Template sizes
    static constexpr std::size_t kTemplateSpriteCount = 9U;
    static constexpr std::size_t kBorderSpriteCount = 6U;
    static constexpr std::size_t kPortraitSpriteCount = 3U;
};

/// 8-byte Mega Drive VDP Sprite Template entry stored in ROM.
struct VdpSpriteTemplate {
    std::uint16_t y{0U};
    std::uint16_t size_link{0U};
    std::uint16_t attr{0U};
    std::int16_t x{0};
};

/// 8-byte emitted SAT sprite entry.
struct DialogueSpriteOutput {
    std::uint16_t y{0U};
    std::uint8_t size{0U};
    std::uint8_t link{0U};
    std::uint16_t attr{0U};
    std::int16_t x{0};
};

/// SAT hardware sprite reservation result.
struct SatReservationResult {
    std::uint16_t byte_count{0U};
    std::uint16_t sprite_count{0U};
};

/// Enqueued DMA command for flashing dialogue cursor tile pattern.
struct CursorDmaCommand {
    bool triggered{false};
    std::uint32_t rom_source_addr{0U};
    std::uint16_t vram_dest_addr{0U};
    std::uint16_t word_length{0U};
    std::uint8_t updated_frame{0U};
};

/// Input configuration for dialogue sprite compositor.
struct DialogueSpriteInput {
    std::uint8_t style{0U};          // $FF1858: 0 = Primary (0x00A438), non-zero = Secondary (0x00A480)
    std::int16_t offset_x{0};        // $FF185A: horizontal displacement
    bool portrait_active{false};     // $FF184F != 0
    bool cursor_active{false};       // $FF1856 != 0
    std::uint8_t cursor_new_frame{0U};  // $FF1854
    std::uint8_t cursor_prev_frame{0U}; // $FF1855
    std::uint16_t start_sat_offset{0U}; // $FF188C
    std::uint16_t start_link_count{0U}; // $FF188A
};

/// Full output result of dialogue sprite compositor.
struct DialogueSpriteCompositionResult {
    std::vector<DialogueSpriteOutput> sprites;
    std::uint16_t new_sat_byte_offset{0U};
    std::uint16_t new_sprite_link_count{0U};
    CursorDmaCommand cursor_dma{};
};

/// Special entity preset mode (0x00A316..0x00A336).
enum class SpecialEntityAction : std::uint8_t {
    None = 0,
    ExecuteScriptPointer = 1,
    InvokeDemoSpriteHook = 2,
};

/// Calculates static SAT byte and sprite reservations (0x00A4C8..0x00A524).
[[nodiscard]] constexpr SatReservationResult calculate_dialogue_reservations(
    bool has_portrait, bool has_cursor) noexcept {
    SatReservationResult res{};
    res.byte_count = DialogueSpriteSubsystemConfig::kBaseFrameBytes;
    res.sprite_count = DialogueSpriteSubsystemConfig::kBaseFrameSprites;
    if (has_portrait) {
        res.byte_count += DialogueSpriteSubsystemConfig::kPortraitBytes;
        res.sprite_count += DialogueSpriteSubsystemConfig::kPortraitSprites;
    }
    if (has_cursor) {
        res.byte_count += DialogueSpriteSubsystemConfig::kCursorBytes;
        res.sprite_count += DialogueSpriteSubsystemConfig::kCursorSprites;
    }
    return res;
}

/// Calculates cursor VRAM DMA command (0x00A3DC..0x00A422).
[[nodiscard]] constexpr CursorDmaCommand calculate_cursor_dma_command(
    std::uint8_t new_frame, std::uint8_t prev_frame) noexcept {
    CursorDmaCommand cmd{};
    if (new_frame != prev_frame) {
        cmd.triggered = true;
        cmd.rom_source_addr = static_cast<std::uint32_t>(new_frame) *
                                  DialogueSpriteSubsystemConfig::kCursorEntrySizeBytes +
                              DialogueSpriteSubsystemConfig::kCursorRomBaseAddr;
        cmd.vram_dest_addr = DialogueSpriteSubsystemConfig::kCursorVramDest;
        cmd.word_length = DialogueSpriteSubsystemConfig::kCursorDmaWordLen;
        cmd.updated_frame = new_frame;
    }
    return cmd;
}

/// Returns authentic 9-sprite primary dialogue template (0x00A438).
[[nodiscard]] const std::array<VdpSpriteTemplate, 9>& get_primary_dialogue_template() noexcept;

/// Returns authentic 9-sprite secondary dialogue template (0x00A480).
[[nodiscard]] const std::array<VdpSpriteTemplate, 9>& get_secondary_dialogue_template() noexcept;

/// Emits dialogue frame, portrait, and cursor sprites into SAT (0x00A342..0x00A436).
[[nodiscard]] DialogueSpriteCompositionResult compose_dialogue_sprites(
    const DialogueSpriteInput& input) noexcept;

/// Runs dialogue sprite composition against raw Mega Drive RAM byte buffer.
[[nodiscard]] DialogueSpriteCompositionResult compose_dialogue_sprites_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr = 0x00FF0000U);

/// Evaluates special entity preset actions (0x00A316..0x00A336).
[[nodiscard]] SpecialEntityAction evaluate_special_entity_preset(
    std::int16_t entity_type,
    std::uint32_t script_ptr) noexcept;

} // namespace oasis::game::render
