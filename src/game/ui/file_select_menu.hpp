#pragma once

#include <cstdint>
#include <vector>

namespace oasis::game::ui {

enum class FileSelectSubmode : std::uint8_t {
    Main = 0,
    CopySource,
    CopyDest,
    EraseSelect,
};

enum class FileSelectAction : std::uint8_t {
    None = 0,
    NewGame,        // 0x001300
    ContinueGame,   // 0x00161C
    ReturnToTitle,  // Mode 0x04
    CopyCompleted,  // 0x000B04
    EraseCompleted, // 0x000C46
};

struct FileSelectConfig {
    static constexpr std::uint32_t kEntryAddr = 0x000004C6U;
    static constexpr std::uint32_t kGridNavAddr = 0x000006F6U;
    static constexpr std::uint32_t kCopyFlowAddr = 0x00000B04U;
    static constexpr std::uint32_t kEraseFlowAddr = 0x00000C46U;
    static constexpr std::uint32_t kNewGameAddr = 0x00001300U;
    static constexpr std::uint32_t kContinueGameAddr = 0x0000161CU;
    static constexpr std::uint32_t kSramLastSlotAddr = 0x002025C9U;

    static constexpr std::uint32_t kCursorMoveSfx = 0x006003A6U;
    static constexpr std::uint32_t kConfirmSfx = 0x00A00256U;
    static constexpr std::uint32_t kInvalidEraseSfx = 0x00A00253U;
};

struct FileSelectInput {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool confirm{false}; // Start / A
    bool cancel{false};  // B / C
};

// Models the 2x3 Save/Load File Select Menu (0x0004C6..0x000D30).
struct FileSelectMenuState {
    std::uint8_t cursor{0U};                // $FF10A8 (0..5)
    std::uint8_t occupied_mask{0U};         // $FF10AA (bits 0..3)
    std::uint8_t copy_src_slot{0U};         // $FF19A8 (0..3)
    std::uint8_t last_slot_sram_2025c9{0U}; // $2025C9
    FileSelectSubmode submode{FileSelectSubmode::Main};
    FileSelectAction last_action{FileSelectAction::None};
    std::uint8_t selected_slot{0U};
    std::vector<std::uint32_t> sound_commands{};

    void reset(std::uint8_t initial_occupied_mask = 0U, std::uint8_t initial_cursor = 0U) noexcept;

    // ROM 0x0006F6: 2D grid navigation with `max_row` (4 in Main, 2 in Copy/Erase) and
    // `excluded_slot` guard (`d3 = copy_src_slot` in CopyDest).
    bool navigate(bool up, bool down, bool left, bool right);

    // Advances menu state for one input step (navigation, confirm, cancel).
    void step(const FileSelectInput& input);
};

} // namespace oasis::game::ui
