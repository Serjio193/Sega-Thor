#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::player {

struct MasterFrameSequencerConfig {
    static constexpr std::uint32_t kMasterFrameSequencerAddr = 0x008AA6U;
    static constexpr std::uint32_t kMasterFrameSequencerEndAddr = 0x008B8CU;
    static constexpr std::uint32_t kScreenShakeDispatcherAddr = 0x008B8EU;
    static constexpr std::uint32_t kScreenShakeDispatcherEndAddr = 0x008BAAU;
    static constexpr std::uint32_t kJoypadMenuHotkeysAddr = 0x008BACU;
    static constexpr std::uint32_t kJoypadMenuHotkeysEndAddr = 0x008C58U;

    static constexpr std::uint32_t kPaletteFlashHandlerAddr = 0x00530CU;
    static constexpr std::uint32_t kEarthquakeShakeHandlerAddr = 0x007CC6U;
    static constexpr std::uint32_t kMenuHandlerAddr = 0x003B3EU;
    static constexpr std::uint32_t kTitleSequenceAddr = 0x002934U;

    // 14 Pipeline routine call targets
    static constexpr std::uint32_t kPauseMenuAddr = 0x00A4C8U;
    static constexpr std::uint32_t kJoypadPollAddr = 0x008BACU;
    static constexpr std::uint32_t kScreenShakeStepAddr = 0x008B8EU;
    static constexpr std::uint32_t kEventVmTickAddr = 0x00F788U;
    static constexpr std::uint32_t kDialogueHudStepAddr = 0x00557AU;
    static constexpr std::uint32_t kSpiritManaStepAddr = 0x00DEECU;
    static constexpr std::uint32_t kSecondaryWeaponFlightAddr = 0x00E0BAU;
    static constexpr std::uint32_t kHazardBlastStepAddr = 0x00E0F4U;
    static constexpr std::uint32_t kAliMachineStepAddr = 0x008E22U;
    static constexpr std::uint32_t kEntitiesListStepAddr = 0x008E90U;
    static constexpr std::uint32_t kEntityGcReclaimAddr = 0x008DD4U;
    static constexpr std::uint32_t kCameraBoundaryClampAddr = 0x00C998U;
    static constexpr std::uint32_t kPlaneScrollSyncAddr = 0x00C980U;
    static constexpr std::uint32_t kSatComposeAddr = 0x00A196U;

    // RAM locations
    static constexpr std::uint32_t kGlobalTickRam = 0x00FF0D76U;
    static constexpr std::uint32_t kAnimSubtickRam = 0x00FF1971U;
    static constexpr std::uint32_t kVblankSyncRam = 0x00FF1654U;
    static constexpr std::uint32_t kVblankFlagRam = 0x00FF164DU;
    static constexpr std::uint32_t kFrameCountRam = 0x00FF199CU;
    static constexpr std::uint32_t kSlowMotionRam = 0x00FF17B8U;
    static constexpr std::uint32_t kPauseFlagRam = 0x00FF1651U;
    static constexpr std::uint32_t kPauseBusyRam = 0x00FF164EU;
    static constexpr std::uint32_t kEnemyGaugeTimerRam = 0x00FF184FU;
    static constexpr std::uint32_t kDemoFlagRam = 0x00FF1996U;
    static constexpr std::uint32_t kDemoFinishedRam = 0x00FF1997U;
    static constexpr std::uint32_t kScreenShakeRam = 0x00FF1864U;
    static constexpr std::uint32_t kJoypadHeldRam = 0x00FF165EU;
    static constexpr std::uint32_t kJoypadEdgeRam = 0x00FF165FU;
    static constexpr std::uint32_t kJoypadExtraRam = 0x00FF1660U;
    static constexpr std::uint32_t kMenuRequestRam = 0x00FF185EU;
    static constexpr std::uint32_t kKeyRepeatRam = 0x00FF19A7U;
    static constexpr std::uint32_t kRingCountRam = 0x00FF16EAU;
    static constexpr std::uint32_t kAliSubstateRam = 0x00FF19ECU;
    static constexpr std::uint32_t kSpiritActiveRam = 0x00FF1A6EU;
    static constexpr std::uint32_t kInputLock1987Ram = 0x00FF1987U;
    static constexpr std::uint32_t kInputLock1983Ram = 0x00FF1983U;
    static constexpr std::uint32_t kInputLock1982Ram = 0x00FF1982U;

    static constexpr std::uint8_t kButtonsReleaseMask = 0x70U; // Bits 4, 5, 6 (A/B/C)
    static constexpr std::uint8_t kStartButtonBit = 7U;        // Start button bit in edge joypad
};

enum class FrameSequencerStatus : std::uint8_t {
    Continue = 0,
    ExitToTitle = 1
};

enum class PipelinePhase : std::uint8_t {
    PauseMenu = 0,
    JoypadPoll = 1,
    ScreenShakeStep = 2,
    EventVmTick = 3,
    DialogueHudStep = 4,
    SpiritManaStep = 5,
    SecondaryWeapon = 6,
    HazardBlast = 7,
    AliMachineStep = 8,
    EntitiesListStep = 9,
    EntityGcReclaim = 10,
    CameraClamp = 11,
    PlaneScrollSync = 12,
    ClearEnemyGaugeInDemo = 13,
    SatCompose = 14
};

enum class ScreenShakeAction : std::uint8_t {
    None = 0,
    PaletteFlash = 1, // 0x00530C
    ScreenShake = 2   // 0x007CC6
};

struct ScreenShakeDispatchResult {
    ScreenShakeAction action{ScreenShakeAction::None};
    std::uint8_t remaining_trigger{0U};
};

struct MenuHotkeyResult {
    bool menu_triggered{false};
    std::uint8_t menu_index{0U};      // 0: Pause, 1: Item Ring, 2: Weapon Ring, 3: Map/Status
    bool key_repeat_reset{false};    // $FF19A7 set to 0xFF
    bool ring_count_cleared{false};  // $FF16EA set to 0
};

struct GameplayFrameState {
    std::uint32_t global_tick{0U};         // $FF0D76
    std::uint8_t anim_subtick{0U};         // $FF1971
    std::uint16_t frame_countdown{0U};     // $FF1654
    std::uint16_t frame_count{0U};         // $FF199C
    std::uint8_t flags_ff164d{0U};         // $FF164D
    std::uint8_t slow_motion_flags{0U};    // $FF17B8
    std::uint8_t pause_flag{0U};           // $FF1651
    std::uint8_t pause_busy{0U};           // $FF164E
    std::uint8_t enemy_gauge_timer{0U};    // $FF184F
    bool demo_active{false};               // $FF1996
    bool demo_playing{false};              // $FF1997
    std::uint8_t screen_shake_trigger{0U}; // $FF1864
    std::uint8_t joypad_held{0U};          // $FF165E
    std::uint8_t joypad_edge{0U};          // $FF165F
    std::uint16_t joypad_extra{0U};        // $FF1660
    std::uint8_t menu_request{0U};         // $FF185E
    std::uint8_t key_repeat_flags{0U};     // $FF19A7
    std::uint16_t input_ring_count{0U};    // $FF16EA
    std::uint16_t ali_substate{0U};        // $FF19EC
    std::uint16_t spirit_active{0U};       // $FF1A6E
    std::uint8_t input_lock_1987{0U};      // $FF1987
    std::uint8_t input_lock_1983{0U};      // $FF1983
    std::uint8_t input_lock_1982{0U};      // $FF1982
};

/// ROM 0x008B8E..0x008BAA: Screen Shake & Palette Flash Dispatcher.
[[nodiscard]] ScreenShakeDispatchResult dispatch_screen_shake(std::uint8_t& trigger_byte) noexcept;

/// ROM 0x008BAC..0x008C58: Joypad Menu Hotkeys & Input Reset.
[[nodiscard]] MenuHotkeyResult poll_joypad_menu_hotkeys(
    std::uint8_t held_joypad,
    std::uint8_t edge_joypad,
    std::uint16_t extra_buttons,
    std::uint8_t lock_1987,
    std::uint8_t slow_motion_flags,
    std::uint8_t lock_1983,
    std::uint8_t lock_1982,
    std::uint16_t ali_substate,
    std::uint16_t spirit_active,
    std::uint8_t& menu_request,
    std::uint8_t& key_repeat,
    std::uint16_t& ring_count) noexcept;

/// Convenience overload operating directly on GameplayFrameState.
[[nodiscard]] MenuHotkeyResult poll_joypad_menu_hotkeys(GameplayFrameState& state) noexcept;

/// ROM 0x008AA6..0x008B8C: Master Gameplay Frame Sequencer.
[[nodiscard]] FrameSequencerStatus step_gameplay_frame(
    GameplayFrameState& state,
    std::vector<PipelinePhase>* executed_phases = nullptr) noexcept;

} // namespace oasis::game::player
