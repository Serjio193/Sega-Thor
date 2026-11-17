#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::entities {

inline constexpr std::uint32_t kAirborneMoveAndLandAddr = 0x00E522U;
inline constexpr std::uint32_t kIsOffscreenWithZAddr = 0x00E5AEU;
inline constexpr std::uint32_t kIsOffscreenGroundAddr = 0x00E610U;
inline constexpr std::uint32_t kCopyEntityStruct0xBCAddr = 0x00E66EU;

inline constexpr std::int16_t kViewportWidthPx = 320;  // 0x0140
inline constexpr std::int16_t kViewportHeightPx = 224; // 0x00E0
inline constexpr std::uint8_t kLandingStunFrames = 15U;
inline constexpr std::uint32_t kLandingThudSfx = 0x00800270U;
inline constexpr std::size_t kEntityStructWords = 94U;
inline constexpr std::size_t kEntityStructBytes = 0xBCU; // 188 bytes

enum class AirborneLandStatus : std::uint8_t {
    GroundedClear = 0, // z == 0 and (flags_37 & 0x02) != 0
    JustLanded = 1,    // z transitioned < 0
    Airborne = 2,      // z > 0
    Grounded = 3,      // z == 0 and (flags_37 & 0x02) == 0
};

struct AirborneEntityState {
    std::int32_t x_16_16{0};
    std::int32_t y_16_16{0};
    std::int32_t z_16_16{0};
    std::int32_t vx_16_16{0};
    std::int32_t vy_16_16{0};
    std::int32_t vz_16_16{0};
    std::uint16_t facing{0U};
    std::uint16_t air_timer_2e{0U};
    std::uint8_t byte_83{0U};
    std::uint8_t flags_37{0U};
    std::uint8_t flags_39{0U};
    std::uint8_t landing_stun_9a{0U};
};

struct AirborneStepResult {
    AirborneEntityState next_state{};
    AirborneLandStatus status{AirborneLandStatus::Grounded};
    std::uint32_t sound_cmd{0U};
};

/// Models 0x00E502..0x00E5AC: steps Z-gravity via step_z_gravity() and handles
/// airborne XY integration, landing stun (15f) + thud SFX (0x00800270), and grounded codes.
[[nodiscard]] AirborneStepResult step_airborne_move_and_land(
    AirborneEntityState state,
    bool spin_when_idle_ff1971_bit0 = false) noexcept;

/// Models 0x00E5AE..0x00E60E: checks if (x, y - z) is outside [cam - margin, cam + 320x224 + margin].
[[nodiscard]] bool is_offscreen_with_z(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t margin = 0) noexcept;

/// Models 0x00E610..0x00E66C: checks if (x, y) is outside [cam - margin, cam + 320x224 + margin].
[[nodiscard]] bool is_offscreen_ground(
    std::int16_t x,
    std::int16_t y,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::int16_t margin = 0) noexcept;

/// Models 0x00E66E..0x00E67E: copies 94 words (0xBC = 188 bytes) from src to dst.
void copy_entity_struct_0xbc(
    std::span<const std::uint8_t, kEntityStructBytes> src,
    std::span<std::uint8_t, kEntityStructBytes> dst) noexcept;

} // namespace oasis::game::entities
