#pragma once

#include <cstdint>

namespace oasis::game::ui {

// ROM 0x001300..0x00144F: Secret P1+P2 input chord branches on $FF165E.L.
enum class NewGameChordBranch : std::uint8_t {
    Normal = 0,               // 0x03BF86 intro -> 0x00CA38 gameplay
    SecretMenu_0xD6000100,    // -> 0x03C8DE
    DebugSuperStart_0xC9000200, // -> 0x001450
    SpecialFlag_0xCA000100,   // -> 0x001394 ($FF0D7A = 0x80)
    AltScreen_0x90000000,     // -> 0x03E7F4
};

struct NewGameConfig {
    static constexpr std::uint32_t kEntryAddr = 0x00001300U;
    static constexpr std::uint32_t kIntroCutsceneAddr = 0x0003BF86U;
    static constexpr std::uint32_t kGameplayEntryAddr = 0x0000CA38U;

    static constexpr std::uint32_t kMaskSecretMenu = 0xD6000100U;
    static constexpr std::uint32_t kTargetSecretMenu = 0x0003C8DEU;

    static constexpr std::uint32_t kMaskDebugSuperStart = 0xC9000200U;
    static constexpr std::uint32_t kTargetDebugSuperStart = 0x00001450U;

    static constexpr std::uint32_t kMaskSpecialFlag = 0xCA000100U;
    static constexpr std::uint32_t kTargetSpecialFlag = 0x00001394U;

    static constexpr std::uint32_t kMaskAltScreen = 0x90000000U;
    static constexpr std::uint32_t kTargetAltScreen = 0x0003E7F4U;
};

// ROM 0x001300: Initial player, room, camera, and chord state for a New Game.
struct NewGameStartState {
    std::uint16_t world_flags_ff0d80{0x01FFU}; // $FF0D80
    std::uint16_t entity_id{2U};               // $FF19E8 id = 2
    std::uint16_t facing{2U};                  // facing = 2
    std::uint16_t max_hp{200U};                // 0xC8
    std::uint16_t hp{200U};                    // 0xC8
    std::uint16_t max_sp{0U};
    std::uint16_t sp{0U};
    std::uint16_t base_attack{10U};            // $FF0D7E = 0x0A
    std::uint16_t active_spirit{0U};           // $FF1AA4 = 0
    std::uint16_t start_room_id{0x001BU};      // Room 27
    std::int32_t start_cam_x{96};
    std::int32_t start_cam_y{0};
    std::int32_t start_x{256};
    std::int32_t start_y{80};
    std::uint8_t special_flag_ff0d7a{0U};      // $FF0D7A (0x80 on 0xCA000100)
    std::uint8_t debug_flags{0U};              // Nonzero on DebugSuperStart
    NewGameChordBranch chord_branch{NewGameChordBranch::Normal};
    std::uint32_t target_pc{NewGameConfig::kGameplayEntryAddr};
};

// ROM 0x001300..0x00144F: Initializes New Game state and evaluates secret chords on $FF165E.L.
[[nodiscard]] NewGameStartState initialize_new_game(std::uint32_t input_ff165e_l = 0U) noexcept;

} // namespace oasis::game::ui
