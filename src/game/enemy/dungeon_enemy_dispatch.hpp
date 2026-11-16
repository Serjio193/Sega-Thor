#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace oasis::game::enemy {

// ROM 0x00DF54: Dungeon enemy families (0x006C, 0x006E, 0x0070, 0x0072) using $66(A6) substate dispatch.
struct DungeonEnemyFamilySpec {
    std::uint16_t type_id{0U};
    std::uint32_t handler_pc{0U};
    std::uint8_t substate_count{0U};
    std::uint32_t variant_table_pc{0U}; // 0x01F5C8 for 0x006C ($96 -> $90)
    std::uint32_t shared_helper_pc{0U}; // 0x01DFA4 for 0x0070
    std::uint32_t jump_table_pc{0U};    // 0x01D152 for 0x0072 (self-relative table)
};

// ROM 0x01F5C8: 4-entry variant parameter table for Entity 0x006C ($96 -> $90).
inline constexpr std::array<std::uint16_t, 4> kDungeonEnemy006CVariants = {3U, 6U, 9U, 12U};

// ROM 0x01D152: 10-entry substate target PCs for Entity 0x0072 ($66(A6) = 0..9).
inline constexpr std::array<std::uint32_t, 10> kDungeonEnemy0072Substates = {
    0x0001D412U, // 0
    0x0001DA20U, // 1
    0x0001DC14U, // 2
    0x0001D166U, // 3
    0x0001D194U, // 4
    0x0001D1D4U, // 5
    0x0001D226U, // 6
    0x0001D22EU, // 7
    0x0001D236U, // 8
    0x0001D23EU, // 9
};

inline constexpr std::array<DungeonEnemyFamilySpec, 4> kDungeonEnemyFamilies = {{
    {0x006CU, 0x0001F5A4U, 3U, 0x0001F5C8U, 0U, 0U},
    {0x006EU, 0x0001EB54U, 10U, 0U, 0U, 0U},
    {0x0070U, 0x0001DF1CU, 6U, 0U, 0x0001DFA4U, 0U},
    {0x0072U, 0x0001D140U, 10U, 0U, 0U, 0x0001D152U},
}};

// Looks up a dungeon enemy family spec by ROM entity type ID (0x006C, 0x006E, 0x0070, 0x0072).
[[nodiscard]] std::optional<DungeonEnemyFamilySpec> lookup_dungeon_enemy_family(
    std::uint16_t type_id) noexcept;

// ROM 0x01F5C8: Resolves 0x006C variant parameter ($96 -> $90). Returns 0 if index >= 4.
[[nodiscard]] std::uint16_t lookup_006c_variant_param(std::uint8_t variant_index) noexcept;

// ROM 0x01D152: Resolves 0x0072 substate target PC (0..9). Returns 0 if substate >= 10.
[[nodiscard]] std::uint32_t resolve_0072_substate_target(std::uint8_t substate) noexcept;

// Models $66(A6) substate clamping/reset, 0x006C variant lookup, and 0x0072 target resolution.
class DungeonEnemySubstateController {
public:
    bool init(std::uint16_t type_id, std::uint8_t variant_006c = 0U) noexcept;
    void reset() noexcept;

    // Sets $66(A6) substate; if `substate >= substate_count`, resets cleanly to 0.
    void set_substate(std::uint8_t substate) noexcept;

    // Advances $66(A6) substate by 1, wrapping to 0 at `substate_count`.
    void advance_substate() noexcept;

    // Applies 0x006C variant parameter ($96 -> $90 from 0x01F5C8: [3, 6, 9, 12]).
    void set_variant_006c(std::uint8_t variant_index) noexcept;

    [[nodiscard]] bool is_valid() const noexcept { return valid_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return spec_.type_id; }
    [[nodiscard]] const DungeonEnemyFamilySpec& spec() const noexcept { return spec_; }
    [[nodiscard]] std::uint8_t substate() const noexcept { return substate_; }
    [[nodiscard]] std::uint8_t variant_index_006c() const noexcept { return variant_index_; }
    [[nodiscard]] std::uint16_t param_90() const noexcept { return param_90_; }
    [[nodiscard]] std::uint32_t active_target_pc() const noexcept;

private:
    bool valid_{false};
    DungeonEnemyFamilySpec spec_{};
    std::uint8_t substate_{0U};       // $66(A6)
    std::uint8_t variant_index_{0U};  // $96(A6) for 0x006C
    std::uint16_t param_90_{0U};      // $90(A6) for 0x006C
};

} // namespace oasis::game::enemy
