#pragma once

#include "game/spirits/efreet_controller.hpp"

#include <cstdint>

namespace oasis::game::spirits {

// ROM 0x013008..0x0136CC: Shade (0x001C) state table, 40-frame SP drain, and abilities.
struct ShadeConfig {
    static constexpr std::uint16_t kEntityType = 0x001CU;
    static constexpr std::uint32_t kStateTableRom = 0x00013008U;
    static constexpr std::uint32_t kDespawnHandlerRom = 0x000136CCU;
    static constexpr std::uint16_t kSpDrainIntervalFrames = 40U;

    // 0x013036: Shadow Tether — consumes 7 SP via 0x011B28, transitions to State 0x06 / Action 0x02,
    // and locks player movement/input via $FF1982 = 0xFF.
    static constexpr std::uint32_t kShadowTetherRom = 0x00013036U;
    static constexpr std::uint16_t kShadowTetherSpCost = 7U;
    static constexpr std::uint32_t kPlayerLockRam = 0x00FF1982U;

    // 0x013180: Shadow Surge — transitions to State 0x0C / Action 0x0E (no additional SP cost).
    static constexpr std::uint32_t kShadowSurgeRom = 0x00013180U;
};

class ShadeSpiritController {
public:
    enum class State : std::uint16_t {
        Idle = 0x0000U,
        Follow = 0x0004U,
        ShadowTether = 0x0006U,
        ShadowSurge = 0x000CU,
        Dismiss = 0x0010U,
    };

    void reset() noexcept;
    void init_spawn(std::int32_t x, std::int32_t y, std::int32_t z = 0) noexcept;

    // ROM 0x013036: Consumes 7 SP via try_consume_spirit_ability_sp (0x011B28).
    // Sets State 0x06, Action 0x02, and player_lock_flag_ ($FF1982) = 0xFF.
    bool cast_shadow_tether(std::uint16_t& player_sp) noexcept;

    // ROM 0x013180: Transitions Shade to State 0x0C / Action 0x0E.
    bool trigger_shadow_surge() noexcept;

    void trigger_dismiss() noexcept;
    void update_frame(std::uint16_t& player_sp) noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return ShadeConfig::kEntityType; }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] std::uint16_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint8_t player_lock_flag() const noexcept { return player_lock_flag_; }
    [[nodiscard]] std::uint16_t ability_timer() const noexcept { return ability_timer_; }
    [[nodiscard]] std::uint16_t sp_drain_timer() const noexcept { return sp_drain_timer_; }
    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }

private:
    bool active_{false};
    State state_{State::Idle};
    std::uint16_t action_{0U};
    std::uint8_t player_lock_flag_{0U}; // $FF1982: 0xFF while Shadow Tether holds player
    std::uint16_t ability_timer_{0U};
    std::uint16_t sp_drain_timer_{ShadeConfig::kSpDrainIntervalFrames};
    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
};

// ROM 0x0138E6..0x013F22: Bow (0x001E) state table, 30-frame SP drain, and abilities.
struct BowConfig {
    static constexpr std::uint16_t kEntityType = 0x001EU;
    static constexpr std::uint32_t kStateTableRom = 0x000138E6U;
    static constexpr std::uint32_t kDespawnHandlerRom = 0x00013F22U;
    static constexpr std::uint16_t kSpDrainIntervalFrames = 30U;

    // 0x013934: Bite — consumes 4 SP via 0x011B28.
    static constexpr std::uint32_t kBiteRom = 0x00013934U;
    static constexpr std::uint16_t kBiteSpCost = 4U;

    // 0x0139BA: Pollen / Vine — consumes 9 SP via 0x011B28, calls helper 0x013FDC,
    // transitions to State 0x04 / Action 0x00.
    static constexpr std::uint32_t kPollenVineRom = 0x000139BAU;
    static constexpr std::uint16_t kPollenVineSpCost = 9U;
    static constexpr std::uint32_t kPollenVineHelperRom = 0x00013FDCU;

    // 0x013A18: Plant Storm — consumes 28 (0x1C) SP via 0x011B28,
    // transitions to State 0x0A / Action 0x10.
    static constexpr std::uint32_t kPlantStormRom = 0x00013A18U;
    static constexpr std::uint16_t kPlantStormSpCost = 28U;
};

class BowSpiritController {
public:
    enum class State : std::uint16_t {
        Idle = 0x0000U,
        Follow = 0x0002U,
        Bite = 0x0003U,
        PollenVine = 0x0004U,
        PlantStorm = 0x000AU,
        Dismiss = 0x0010U,
    };

    void reset() noexcept;
    void init_spawn(std::int32_t x, std::int32_t y, std::int32_t z = 0) noexcept;

    // ROM 0x013934: Consumes 4 SP via try_consume_spirit_ability_sp (0x011B28).
    bool cast_bite(std::uint16_t& player_sp) noexcept;

    // ROM 0x0139BA: Consumes 9 SP via 0x011B28, invokes helper 0x013FDC,
    // and sets State 0x04 / Action 0x00.
    bool cast_pollen_vine(std::uint16_t& player_sp) noexcept;

    // ROM 0x013A18: Consumes 28 SP via 0x011B28 and sets State 0x0A / Action 0x10.
    bool cast_plant_storm(std::uint16_t& player_sp) noexcept;

    void trigger_dismiss() noexcept;
    void update_frame(std::uint16_t& player_sp) noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return BowConfig::kEntityType; }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] std::uint16_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint32_t last_helper_pc() const noexcept { return last_helper_pc_; }
    [[nodiscard]] std::uint16_t ability_timer() const noexcept { return ability_timer_; }
    [[nodiscard]] std::uint16_t sp_drain_timer() const noexcept { return sp_drain_timer_; }
    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }

private:
    bool active_{false};
    State state_{State::Idle};
    std::uint16_t action_{0U};
    std::uint32_t last_helper_pc_{0U};
    std::uint16_t ability_timer_{0U};
    std::uint16_t sp_drain_timer_{BowConfig::kSpDrainIntervalFrames};
    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
};

} // namespace oasis::game::spirits
