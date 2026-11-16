#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace oasis::game::enemy {

// ROM 0x00DF54: Heavy & Boss enemy families (0x0068, 0x0074, 0x0076, 0x0080) using $66(A6) dispatch.
struct HeavyEnemyFamilySpec {
    std::uint16_t type_id{0U};
    std::uint32_t handler_pc{0U};
    std::uint8_t substate_mask{0xFFU}; // 0x7F for 0x0076; 0xFF (unmasked) otherwise
    std::uint8_t substate_count{0U};
    std::uint32_t substate_0_target_pc{0U};
    std::uint32_t substate_1_target_pc{0U};
    std::uint16_t init_timer_frames{0U}; // 20 (0x14) for 0x0076, 40 (0x28) for 0x0080
    std::uint16_t init_z{0U};            // 0x20 for 0x0076
    std::uint16_t init_state{0U};        // 6 for 0x0080
    std::uint16_t init_action{0U};       // 4 for 0x0080
    bool copy_96_to_90{false};           // $90(A6) = $96(A6) for 0x0076 and 0x0080
};

inline constexpr std::array<HeavyEnemyFamilySpec, 4> kHeavyEnemyFamilies = {{
    {0x0068U, 0x00025B32U, 0xFFU, 5U, 0x00025D30U, 0x000263FAU, 0U, 0U, 0U, 0U, false},
    {0x0074U, 0x00027C8AU, 0xFFU, 5U, 0x00027DDAU, 0x00027CAAU, 0U, 0U, 0U, 0U, false},
    {0x0076U, 0x000283E6U, 0x7FU, 4U, 0x000284E8U, 0U, 20U, 0x0020U, 0U, 0U, true},
    {0x0080U, 0x00028810U, 0xFFU, 3U, 0x000288C2U, 0U, 40U, 0U, 6U, 4U, true},
}};

// Looks up a heavy/boss enemy family spec by ROM entity type ID (0x0068, 0x0074, 0x0076, 0x0080).
[[nodiscard]] std::optional<HeavyEnemyFamilySpec> lookup_heavy_enemy_family(
    std::uint16_t type_id) noexcept;

// Models $66(A6) masking (& 0x7F for 0x0076), clamping/reset, and init timer/state/action setup.
class HeavyEnemySubstateController {
public:
    bool init(std::uint16_t type_id, std::uint16_t param_96 = 0U) noexcept;
    void reset() noexcept;

    // Applies family substate_mask (& 0x7F for 0x0076) and resets to 0 if >= substate_count.
    void set_substate(std::uint8_t raw_substate) noexcept;

    // Advances $66(A6) substate by 1, wrapping to 0 at `substate_count`.
    void advance_substate() noexcept;

    [[nodiscard]] bool is_valid() const noexcept { return valid_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return spec_.type_id; }
    [[nodiscard]] const HeavyEnemyFamilySpec& spec() const noexcept { return spec_; }
    [[nodiscard]] std::uint8_t substate() const noexcept { return substate_; }
    [[nodiscard]] std::uint16_t timer_frames() const noexcept { return timer_frames_; }
    [[nodiscard]] std::uint16_t z() const noexcept { return z_; }
    [[nodiscard]] std::uint16_t state() const noexcept { return state_; }
    [[nodiscard]] std::uint16_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint16_t param_90() const noexcept { return param_90_; }
    [[nodiscard]] std::uint32_t active_target_pc() const noexcept;

private:
    bool valid_{false};
    HeavyEnemyFamilySpec spec_{};
    std::uint8_t substate_{0U};       // $66(A6)
    std::uint16_t timer_frames_{0U};
    std::uint16_t z_{0U};
    std::uint16_t state_{0U};
    std::uint16_t action_{0U};
    std::uint16_t param_90_{0U};      // $90(A6)
};

} // namespace oasis::game::enemy
