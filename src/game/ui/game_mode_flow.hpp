#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::ui {

// ROM 0x00045E: Top-level game mode dispatch table (indexed by mode & 0x7C).
enum class TopLevelGameMode : std::uint16_t {
    SegaLogo = 0x0000U,           // 0x00307A
    TitleAndFileSelect = 0x0004U, // 0x0032F8
    GameplayNew = 0x0008U,        // 0x0089B2
    GameplayAlt = 0x000CU,        // 0x0089B2
    EndingAndCredits = 0x0010U,   // 0x03A748
};

struct GameModeFlowConfig {
    static constexpr std::uint32_t kModeTableRomAddr = 0x0000045EU;
    static constexpr std::uint16_t kNoPendingMode = 0xFFFFU;

    // Title Screen (0x0032F8)
    static constexpr std::uint16_t kTitleAttractTimeoutFrames = 720U; // 0x2D0
    static constexpr std::uint16_t kTitleBlinkMask = 0x007FU;
    static constexpr std::uint16_t kTitleBlinkOnThreshold = 0x0060U;
    static constexpr std::uint16_t kTitleBgmId = 0x0104U;
    static constexpr std::uint32_t kChord90Sfx = 0x00A003AAU;
    static constexpr std::uint32_t kChordD5Sfx = 0x00A00205U;
    static constexpr std::uint32_t kNormalStartSfx = 0x00000002U;

    // Clear Records Screen (0x03E4FA)
    static constexpr std::uint32_t kClearRecordsEntryAddr = 0x0003E4FAU;
    static constexpr std::uint16_t kClearRecordsBgmId = 0x0121U;
    static constexpr std::uint32_t kClearRecordsExitSfx = 0x00000007U;
    static constexpr std::uint32_t kSoftResetTargetAddr = 0x0000020EU;
    static constexpr std::uint32_t kSramMagicAddr1 = 0x002025CDU;
    static constexpr std::uint32_t kSramMagicAddr2 = 0x002026E1U;
};

// ROM 0x00045E: Reads the 32-bit BE handler pointer at `0x00045E + (mode & 0x7C)`.
// Returns std::nullopt if `mode` is not one of {0x00, 0x04, 0x08, 0x0C, 0x10} or OOB.
[[nodiscard]] std::optional<std::uint32_t> lookup_mode_handler_addr(
    std::span<const std::uint8_t> rom,
    TopLevelGameMode mode) noexcept;

// Models $FF10AC (next_mode) -> $FF10AE (current_mode) latch semantics at 0x00042C.
struct GameModeStateMachine {
    TopLevelGameMode current_mode{TopLevelGameMode::SegaLogo}; // $FF10AE
    std::uint16_t pending_mode{GameModeFlowConfig::kNoPendingMode}; // $FF10AC

    void request_mode(TopLevelGameMode next) noexcept;
    bool latch_pending_mode() noexcept;
};

// Models Title Screen (Mode 0x04, 0x0032F8..0x003586).
struct TitleScreenState {
    std::uint16_t timer_frames{0U};
    std::uint16_t active_bgm_id{GameModeFlowConfig::kTitleBgmId};
    bool demo_flag{false}; // $FF1996 (0xFF = true)
    bool entered_clear_records{false};
    bool clear_sram_records_requested{false};
    std::uint16_t pending_mode{GameModeFlowConfig::kNoPendingMode};
    std::uint32_t last_sound_cmd{0U};

    void reset() noexcept;
    [[nodiscard]] bool press_start_visible() const noexcept;
    void step_frame(bool start_pressed = false, std::uint8_t pad_byte_ff165e = 0U) noexcept;
};

struct ClearRecordSlot {
    bool populated{false};
    std::uint32_t clear_time_frames{0U};
    std::uint16_t rank{0U};
    std::uint16_t kills{0U};
    std::uint16_t jewels{0U};
};

// Models "BEST 5 PLAYERS" Clear Records Screen (0x03E4FA..0x03E60D).
struct ClearRecordsScreenState {
    std::array<ClearRecordSlot, 5> slots{};
    std::uint16_t active_bgm_id{GameModeFlowConfig::kClearRecordsBgmId};
    std::uint8_t sram_magic_2025cd{0x5AU};
    std::uint8_t sram_magic_2026e1{0xA5U};
    bool soft_reset_requested{false};
    std::uint32_t soft_reset_target{0U};
    std::uint32_t last_sound_cmd{0U};

    void reset() noexcept;
    // ROM 0x001E98: Clears SRAM clear-record magic bytes ($2025CD, $2026E1) and slots.
    void clear_sram_record_magic() noexcept;
    void step_frame(bool start_pressed = false) noexcept;
};

} // namespace oasis::game::ui
