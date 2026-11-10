#pragma once

#include <array>
#include <cstdlib>
#include <cstdint>
#include <vector>

namespace oasis::game::combat {

// Authentic 32-bit Genesis Z80 sound commands observed in ROM combat disassembly & GPGX F773..F825:
inline constexpr std::uint32_t kSndPlayerSwordSwing   = 0x01500218U; // ROM 0x00632A -> 0x060004
inline constexpr std::uint32_t kSndPlayerActionHit    = 0x007004D3U; // ROM 0x007698 / 0x0078C2 -> 0x060004
inline constexpr std::uint32_t kSndPlayerState36Leap  = 0x0080025BU; // ROM 0x007982 / 0x0079EE -> 0x060004
inline constexpr std::uint32_t kSndCollisionContact   = 0x00800207U; // ROM 0x00BF14 (0x00BF10 -> 0x00E7FC)
inline constexpr std::uint32_t kSndEnemy002cHop       = 0x008004D0U; // ROM 0x02567E -> 0x00E7E8
inline constexpr std::uint32_t kSndEnemy002cLunge     = 0x00700217U; // ROM 0x025A6E -> 0x00E7E8
inline constexpr std::uint32_t kSndEnemy002cBodyHit1  = 0x00800216U; // ROM 0x025AEC -> 0x00E7E8
inline constexpr std::uint32_t kSndEnemy002cBodyHit2  = 0x0080035BU; // ROM 0x025AF8 -> 0x00E7E8
inline constexpr std::uint32_t kSndEnemy002cLandThud  = 0x0080035AU; // ROM 0x00DF1C -> 0x00E7E8
inline constexpr std::uint32_t kSndHeavyBit13Hit      = 0x008004D9U; // ROM 0x01F41C (mailbox & 0x2000)

struct DispatchedSoundEvent {
    int frame_60hz{0};
    std::uint32_t command_32{0};
    std::uint16_t command_id() const noexcept {
        return static_cast<std::uint16_t>(command_32 & 0xFFFFU);
    }
    std::uint8_t priority() const noexcept {
        return static_cast<std::uint8_t>((command_32 >> 24U) & 0xFFU);
    }
    std::uint8_t pan_flags() const noexcept {
        return static_cast<std::uint8_t>((command_32 >> 16U) & 0xFFU);
    }
};

// Evaluates ROM 0x00E7E8 / 0x00E7FC spatial attenuation & stereo panning relative to Player.
// Returns 0 if dropped due to distance > max range (attenuation > 7 for >=0x300, or >=0x110 for 0x200).
[[nodiscard]] inline std::uint32_t attenuate_spatial_sound(
    std::uint32_t raw_cmd32, int src_x, int src_y, int player_x, int player_y,
    bool set_high_attenuation_bit = true) noexcept {
    const std::uint16_t id = static_cast<std::uint16_t>(raw_cmd32 & 0xFFFFU);
    if (id < 0x0200U) {
        return raw_cmd32;
    }

    const int dx_signed = src_x - player_x;
    const int dx = std::abs(dx_signed);
    const int dy = std::abs(src_y - player_y);
    int dist = dy + (dy >> 2);
    if (dx > dist) {
        dist = dx;
    }

    std::uint8_t prio = static_cast<std::uint8_t>((raw_cmd32 >> 24U) & 0xFFU);
    std::uint8_t pan = static_cast<std::uint8_t>((raw_cmd32 >> 16U) & 0xFFU);

    if (id >= 0x0300U) {
        int d = (dist >= 0x58) ? (dist - 0x58) : 0;
        const int att = d >> 5;
        if (att > 7) {
            return 0U; // 0x00E856: bhi $e89c drop
        }
        prio = static_cast<std::uint8_t>((prio - (att + 1)) & 0x1FU);
        if (!set_high_attenuation_bit) {
            // 0x00E88E: 0x00E7FC path preserves/adjusts pan differently only if prio bits < 4
        }
        if (dx >= 0x40) {
            pan = static_cast<std::uint8_t>(pan + (dx_signed < 0 ? 0x42U : 0x82U));
        }
        if (set_high_attenuation_bit) {
            pan = static_cast<std::uint8_t>((pan - (att + 1)) & 0xFFU);
        }
    } else {
        // 0x0200..0x02FF path (0x00E8A2)
        pan = 0U;
        if (dist >= 0xB0) {
            if (dist >= 0xE0) {
                if (dist >= 0x110) {
                    return 0U; // 0x00E8BC: bcc $e89c drop
                }
                prio = static_cast<std::uint8_t>(prio + 2U);
            } else {
                prio = static_cast<std::uint8_t>(prio + 1U);
            }
        }
        if (dx >= 0x60) {
            if (prio > 0U) {
                --prio;
            }
            pan |= (dx_signed < 0) ? 0x40U : 0x80U;
        }
        if (set_high_attenuation_bit && dist >= 0xA0) {
            --pan;
            if (dist >= 0xB8) {
                --pan;
            }
        }
    }

    return (static_cast<std::uint32_t>(prio) << 24U) |
           (static_cast<std::uint32_t>(pan) << 16U) |
           static_cast<std::uint32_t>(id);
}

// Tracks 32-bit sound commands dispatched by player/enemy gameplay code paths with
// ROM 0x06042A / 0x060F1C / 0x06112E channel priority arbitration.
class SoundCommandQueue {
public:
    void clear() noexcept {
        events_.clear();
        last_channel_prio_.fill(0U);
        last_channel_cmd_.fill(0U);
        last_channel_frame_.fill(-1);
    }

    bool push(int frame_60hz, std::uint32_t command_32) {
        if (command_32 == 0U) {
            return false;
        }
        const std::uint16_t cmd_id = static_cast<std::uint16_t>(command_32 & 0xFFFFU);
        const std::uint16_t cat = static_cast<std::uint16_t>(cmd_id & 0xFF00U);
        const std::uint8_t prio = static_cast<std::uint8_t>((command_32 >> 24U) & 0xFFU);

        std::size_t ch = 0U;
        if (cat == 0x0300U) ch = 1U;
        else if (cat == 0x0400U) ch = 2U;
        else if (cat == 0x0200U) ch = 3U;

        // Within the same logic step / active frame window, lower priority SFX cannot preempt higher priority SFX (ROM 0x060F30/0x061176)
        if (ch != 0U && last_channel_frame_[ch] == frame_60hz) {
            if (prio < last_channel_prio_[ch]) {
                return false;
            }
        }
        if (ch != 0U) {
            last_channel_prio_[ch] = prio;
            last_channel_cmd_[ch] = cmd_id;
            last_channel_frame_[ch] = frame_60hz;
        }
        events_.push_back({frame_60hz, command_32});
        return true;
    }

    [[nodiscard]] const std::vector<DispatchedSoundEvent>& events() const noexcept {
        return events_;
    }

    [[nodiscard]] std::uint16_t latest_command_id() const noexcept {
        return events_.empty() ? 0U : events_.back().command_id();
    }

    [[nodiscard]] static constexpr bool audio_output_parity_supported() noexcept {
        return false; // Sound command dispatch parity only; Z80 FM/PSG mixer is out of scope
    }

private:
    std::vector<DispatchedSoundEvent> events_{};
    std::array<std::uint8_t, 4> last_channel_prio_{};
    std::array<std::uint16_t, 4> last_channel_cmd_{};
    std::array<int, 4> last_channel_frame_{-1, -1, -1, -1};
};

} // namespace oasis::game::combat