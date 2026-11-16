#pragma once

#include <cstdint>

namespace oasis::game::spirits {

// ROM 0x011B28: Shared Spirit SP-Cost Guard (`cmpi.w #$1, d1; blt`).
// Returns true and subtracts `cost` iff `player_sp - cost >= 1` (or `cost == 0`).
[[nodiscard]] bool try_consume_spirit_ability_sp(
    std::uint16_t& player_sp,
    std::uint16_t cost) noexcept;

// ROM 0x0125AE..0x012BA8: Efreet (0x001A) state table, SP drain, and abilities.
struct EfreetConfig {
    static constexpr std::uint16_t kEntityType = 0x001AU;
    static constexpr std::uint32_t kStateTableRom = 0x000125AEU;
    static constexpr std::uint32_t kDespawnHandlerRom = 0x00012BA8U;
    static constexpr std::uint16_t kSpDrainIntervalFrames = 35U;

    // 0x0125CA: Fire Dash
    static constexpr std::uint16_t kFireDashSpCost = 8U;
    static constexpr std::uint16_t kFireDashSpeed = 0x0100U;
    static constexpr std::uint32_t kFireDashSfx = 0x00800258U;

    // 0x012608: Flame Pillar
    static constexpr std::uint16_t kFlamePillarSpCost = 6U;
    static constexpr std::uint32_t kFlamePillarHelperSlotRam = 0x00FF1C1CU;

    // 0x01263C: Meltdown
    static constexpr std::uint16_t kMeltdownSpCost = 25U;
    static constexpr std::uint16_t kMeltdownBaseDuration = 96U; // 0x60 + $FF0DB7 level
};

class EfreetSpiritController {
public:
    enum class State : std::uint16_t {
        Idle = 0x0000U,
        Follow = 0x0004U,
        FireDash = 0x0008U,
        FlamePillar = 0x000AU,
        Meltdown = 0x000CU,
        Dismiss = 0x0010U,
    };

    void reset() noexcept;
    void init_spawn(std::int32_t x, std::int32_t y, std::int32_t z = 0) noexcept;

    bool trigger_fire_dash(std::uint16_t& player_sp) noexcept;
    bool trigger_flame_pillar(std::uint16_t& player_sp) noexcept;
    bool trigger_meltdown(std::uint16_t& player_sp, std::uint8_t spirit_level = 0U) noexcept;
    void trigger_dismiss() noexcept;

    void update_frame(std::uint16_t& player_sp) noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return EfreetConfig::kEntityType; }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] std::uint16_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint16_t speed() const noexcept { return speed_; }
    [[nodiscard]] std::uint16_t ability_timer() const noexcept { return ability_timer_; }
    [[nodiscard]] std::uint16_t sp_drain_timer() const noexcept { return sp_drain_timer_; }
    [[nodiscard]] std::uint32_t last_sound_cmd() const noexcept { return last_sound_cmd_; }
    [[nodiscard]] bool helper_slot_reserved() const noexcept { return helper_slot_reserved_; }
    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }

private:
    bool active_{false};
    State state_{State::Idle};
    std::uint16_t action_{0U};
    std::uint16_t speed_{0U};
    std::uint16_t ability_timer_{0U};
    std::uint16_t sp_drain_timer_{EfreetConfig::kSpDrainIntervalFrames};
    std::uint32_t last_sound_cmd_{0U};
    bool helper_slot_reserved_{false};
    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
};

} // namespace oasis::game::spirits
