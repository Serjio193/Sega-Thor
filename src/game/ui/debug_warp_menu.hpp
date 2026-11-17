#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::ui {

inline constexpr std::uint32_t kDebugWarpMenuAddr = 0x004C9CU;
inline constexpr std::uint32_t kDebugWarpStepAddr = 0x004D4EU;
inline constexpr std::uint32_t kDebugWarpSpawnTableAddr = 0x0005D918U;
inline constexpr std::uint32_t kDebugWarpExecAddr = 0x00CA24U;

inline constexpr std::uint8_t kDebugWarpMaxAreaIndex = 31U; // 0x1F
inline constexpr std::size_t kDebugWarpAreaSlotCount = 32U;
inline constexpr std::uint16_t kDebugWarpRepeatInitialFrames = 10U; // 0x0A
inline constexpr std::uint16_t kDebugWarpRepeatHeldFrames = 3U;     // 0x03
inline constexpr std::uint8_t kDebugWarpSpawnTileShift = 3U;

inline constexpr std::uint32_t kDebugWarpMoveSfx = 0x006003A6U;
inline constexpr std::uint32_t kDebugWarpConfirmSfx = 0x00A00256U;
inline constexpr std::uint32_t kDebugWarpCancelSfx = 0x00A00347U;

// 16-bit joypad word ($FF165E): low byte = newly pressed, high byte = held.
inline constexpr std::uint16_t kDebugWarpBtnUp = 0x0001U;
inline constexpr std::uint16_t kDebugWarpBtnDown = 0x0002U;
inline constexpr std::uint16_t kDebugWarpBtnLeft = 0x0004U;
inline constexpr std::uint16_t kDebugWarpBtnRight = 0x0008U;
inline constexpr std::uint16_t kDebugWarpBtnA = 0x0010U;
inline constexpr std::uint16_t kDebugWarpBtnUpHeld = 0x0100U;
inline constexpr std::uint16_t kDebugWarpBtnDownHeld = 0x0200U;
inline constexpr std::uint16_t kDebugWarpBtnLeftHeld = 0x0400U;
inline constexpr std::uint16_t kDebugWarpBtnRightHeld = 0x0800U;

// Cancel byte ($FF165F): bits 5..6 (B / C).
inline constexpr std::uint8_t kDebugWarpBtnCancelMask = 0x60U;

struct DebugWarpSpawnEntry {
    std::uint8_t area_id{0U};
    std::uint8_t room_id{0U};
    std::uint16_t spawn_x_px{0U};
    std::uint16_t spawn_y_px{0U};
};

struct DebugWarpAreaInfo {
    std::uint8_t area_id{0U};
    std::uint16_t max_room_id{0U};
    std::vector<DebugWarpSpawnEntry> rooms{};
};

/// Parses and queries the 32-area ROM spawn table at 0x0005D918.
class DebugWarpSpawnTable {
public:
    /// Parses big-endian ROM bytes starting at table_offset (default 0x0005D918).
    [[nodiscard]] static DebugWarpSpawnTable parse_from_rom(
        std::span<const std::uint8_t> rom,
        std::uint32_t table_offset = kDebugWarpSpawnTableAddr);

    /// Builds a spawn table from pre-decoded area entries (useful for tests).
    [[nodiscard]] static DebugWarpSpawnTable from_areas(
        std::vector<DebugWarpAreaInfo> areas);

    [[nodiscard]] bool has_area(std::uint8_t area_id) const noexcept;
    [[nodiscard]] std::uint16_t max_room_id(std::uint8_t area_id) const noexcept;
    [[nodiscard]] std::optional<DebugWarpSpawnEntry> find_spawn(
        std::uint8_t area_id,
        std::uint8_t room_id) const noexcept;
    [[nodiscard]] std::size_t active_area_count() const noexcept;
    [[nodiscard]] const std::array<std::optional<DebugWarpAreaInfo>, kDebugWarpAreaSlotCount>&
    areas() const noexcept {
        return areas_;
    }

private:
    std::array<std::optional<DebugWarpAreaInfo>, kDebugWarpAreaSlotCount> areas_{};
};

enum class DebugWarpOutcome : std::uint8_t {
    Pending = 0,
    ConfirmedWarp,
    Cancelled,
};

struct DebugWarpMenuState {
    std::uint8_t area_id{0U};
    std::uint8_t room_id{0U};
    std::uint16_t repeat_timer{0U};
};

struct DebugWarpStepResult {
    DebugWarpMenuState next_state{};
    DebugWarpOutcome outcome{DebugWarpOutcome::Pending};
    std::uint32_t sound_cmd{0U};
    std::optional<DebugWarpSpawnEntry> warp_target{};
};

/// Models 0x004CE2..0x004F42: steps the debug stage/room warp menu by one frame.
[[nodiscard]] DebugWarpStepResult step_debug_warp_menu(
    const DebugWarpSpawnTable& table,
    DebugWarpMenuState state,
    std::uint16_t joypad_word,
    std::uint8_t cancel_flags = 0U) noexcept;

} // namespace oasis::game::ui
