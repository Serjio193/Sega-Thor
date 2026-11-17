#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oasis::game::ui {

inline constexpr std::uint32_t kBuildDialogueSatAddr = 0x00C756U;
inline constexpr std::uint32_t kEmit8SpriteBarAddr = 0x00C81CU;
inline constexpr std::uint32_t kAppendSatEntryAddr = 0x00C838U;

inline constexpr std::size_t kDialogueSatMaxSprites = 80U; // 0x50
inline constexpr std::uint16_t kCursorTileBase = 0x86DCU;
inline constexpr std::uint16_t kCursorAltPaletteMask = 0x1000U; // bit 12 -> 0x96DC
inline constexpr std::uint16_t kCursorX = 0x011CU;
inline constexpr std::uint16_t kCursorYBase = 0x014CU;
inline constexpr std::uint16_t kCursorYStep = 12U;
inline constexpr std::uint16_t kBorderSizeLinkBase = 0x0C00U;
inline constexpr std::uint16_t kBorderStartX = 0x00A0U;
inline constexpr std::uint16_t kBorderXStep = 0x0020U;
inline constexpr std::uint16_t kBorderYBias = 0x0110U;
inline constexpr std::uint16_t kBorderSpritesPerBar = 8U;
inline constexpr std::uint16_t kBorderTileStep = 4U;

struct DialogueSatEntry {
    std::uint16_t y{0};
    std::uint16_t size_link{0};
    std::uint16_t tile{0};
    std::uint16_t x{0};
};

struct DialogueSatInput {
    bool cursor_active{false};      // $FF1886 != 0
    bool blink_visible{false};      // ($FF1657 & 0x10) != 0
    bool cursor_alt_palette{false}; // $FF1887 != 0
    std::uint8_t choice_row{0};

    bool top_bars_active{false};    // $FF1872 != 0
    std::uint16_t top_bar1_y{0};    // $FF1876
    std::uint16_t top_bar1_vram{0};
    std::uint16_t top_bar2_y{0};    // $FF1878
    std::uint16_t top_bar2_vram{0};

    bool bottom_bars_active{false}; // $FF1874 != 0
    std::uint16_t bottom_bar1_y{0}; // $FF187A
    std::uint16_t bottom_bar1_vram{0};
    std::uint16_t bottom_bar2_y{0}; // $FF187C
    std::uint16_t bottom_bar2_vram{0};
};

/// Models 0x00C838..0x00C84C: appends one SAT entry if sat.size() < 80.
bool append_dialogue_sat_entry(
    std::vector<DialogueSatEntry>& sat,
    std::uint16_t y,
    std::uint16_t size_base,
    std::uint16_t tile,
    std::uint16_t x);

/// Models 0x00C81C..0x00C836: emits an 8-sprite horizontal bar at X=0x00A0+0x20*i.
void emit_8_sprite_bar(
    std::vector<DialogueSatEntry>& sat,
    std::uint16_t bar_y,
    std::uint16_t bar_vram);

/// Models 0x00C756..0x00C81A: emits dialogue choice cursor and 4x8 border bars.
void build_dialogue_sat(
    const DialogueSatInput& input,
    std::vector<DialogueSatEntry>& sat);

} // namespace oasis::game::ui
