#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::dialogue {

// ROM 0x00C40E..0x00C534: Dialogue control codes.
enum class DialogueControlCode : std::uint8_t {
    End = 0x00,
    Newline = 0x01,
    WaitButtonNewline = 0x02,
    DelayFrames = 0x03,
    WaitButton = 0x05,
    ClearPage = 0x06,
    WaitButtonClearPage = 0x07,
};

struct DialogueEngineConfig {
    static constexpr std::uint32_t kEntryAddr = 0x0000C386U;
    static constexpr std::uint32_t kTableRomAddr = 0x00051514U;
    static constexpr std::uint16_t kTableEntryCount = 116U;
    static constexpr std::uint32_t kFontCompressedRomAddr = 0x0016943CU;
    static constexpr std::size_t kGlyphStrideBytes = 32U;
    static constexpr std::uint8_t kAsciiBase = 0x20U;
    static constexpr std::array<std::uint16_t, 4> kDefaultLineVramOffsets = {0x0CU, 0x18U, 0x24U, 0x30U};
};

// ROM 0x051514 (0x00C3F8): Resolves `entry_addr + u16_be(entry_addr)` where
// `entry_addr = 0x051514 + dialogue_id * 2` for `dialogue_id < 116`.
[[nodiscard]] std::optional<std::uint32_t> lookup_dialogue_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint16_t dialogue_id) noexcept;

// ROM 0x00C6BE / 0x16943C: Returns `(ch - 0x20) * 32` byte offset in the decompressed
// 8x8 4bpp font buffer, or std::nullopt if `ch < 0x20`.
[[nodiscard]] std::optional<std::size_t> glyph_rom_offset(char ch) noexcept;

enum class PendingWaitAction : std::uint8_t {
    None = 0,
    EndClose,
    Newline,
    ClearPage,
    ContinueOnly,
};

// Models RAM state for the ROM dialogue engine (0x00C386..0x00C755).
struct DialogueEngineState {
    std::uint16_t dialogue_id{0U};       // $FF1870
    std::uint32_t text_pc{0U};
    std::uint16_t col_cursor{0U};        // $FF187E
    std::uint16_t line_cursor{0U};       // $FF1880 (0..3)
    std::array<std::uint16_t, 4> line_vram_offsets{DialogueEngineConfig::kDefaultLineVramOffsets}; // $FF1876..$FF187C
    std::uint8_t delay_frames{0U};
    bool waiting_button{false};
    PendingWaitAction wait_action{PendingWaitAction::None};
    std::uint8_t end_flag_ff1887{0U};    // $FF1887 = 0xFF while waiting on 0x00
    bool box_open{false};
    bool finished{false};
    std::array<std::string, 4> emitted_lines{};
    std::vector<std::size_t> emitted_glyph_offsets{};
};

class DialogueEngine {
public:
    void reset() noexcept;
    bool start(std::span<const std::uint8_t> rom, std::uint16_t dialogue_id) noexcept;
    void start_at(std::uint32_t text_pc, std::uint16_t dialogue_id = 0U) noexcept;

    // Advances the dialogue engine by 1 frame. Returns true if still active, false if finished.
    bool step_frame(std::span<const std::uint8_t> rom, bool button_pressed = false);

    [[nodiscard]] DialogueEngineState& state() noexcept { return state_; }
    [[nodiscard]] const DialogueEngineState& state() const noexcept { return state_; }

private:
    void advance_line() noexcept;
    void clear_page() noexcept;

    DialogueEngineState state_{};
};

} // namespace oasis::game::dialogue
