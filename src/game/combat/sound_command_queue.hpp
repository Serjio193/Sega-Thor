#pragma once

#include <cstdint>
#include <vector>

namespace oasis::game::combat {

// Authentic 32-bit Genesis Z80 sound commands observed in ROM combat disassembly & GPGX F773..F825:
inline constexpr std::uint32_t kSndPlayerSwordSwing   = 0x01500218U; // ROM 0x00632A -> 0x060004
inline constexpr std::uint32_t kSndPlayerActionHit    = 0x007004D3U; // ROM 0x007698 / 0x0078C2 -> 0x060004
inline constexpr std::uint32_t kSndPlayerState36Leap  = 0x0080025BU; // ROM 0x007982 / 0x0079EE -> 0x060004
inline constexpr std::uint32_t kSndCollisionContact   = 0x00800207U; // ROM 0x00BF14 (0x00BF10 -> 0x00E7FC)
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
};

// Tracks 32-bit sound commands dispatched by player/enemy gameplay code paths.
// Distinct from Z80/YM2612/PSG PCM audio output synthesis (not implemented standalone).
class SoundCommandQueue {
public:
    void clear() noexcept { events_.clear(); }

    void push(int frame_60hz, std::uint32_t command_32) {
        if (command_32 != 0U) {
            events_.push_back({frame_60hz, command_32});
        }
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
};

} // namespace oasis::game::combat
