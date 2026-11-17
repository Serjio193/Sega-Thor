#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::player {

struct EntityLifecycleConfig {
    static constexpr std::uint32_t kMasterEntityClearAddr = 0x008C5AU;
    static constexpr std::uint32_t kMasterEntityClearEndAddr = 0x008D04U;
    static constexpr std::uint32_t kUniversalConstructorAddr = 0x008D06U;
    static constexpr std::uint32_t kUniversalConstructorEndAddr = 0x008DA2U;
    static constexpr std::uint32_t kScriptPointersReinitAddr = 0x008DA4U;
    static constexpr std::uint32_t kScriptPointersReinitEndAddr = 0x008DD2U;
    static constexpr std::uint32_t kSpiritGlowPulseAddr = 0x008DD4U;
    static constexpr std::uint32_t kSpiritGlowPulseEndAddr = 0x008E22U;

    static constexpr std::uint32_t kDescriptorTableRomAddr = 0x0003F2FAU;
    static constexpr std::size_t kDescriptorStrideBytes = 32U; // 2 x 16-byte blocks per even entity type
    static constexpr std::uint32_t kSmallEntityThresholdRam = 0x00FF2954U;

    // Entity RAM Pools
    static constexpr std::uint32_t kPool1BaseRam = 0x00FF2954U;
    static constexpr std::size_t kPool1Slots = 4U;
    static constexpr std::size_t kPool1Stride = 90U;

    static constexpr std::uint32_t kPool2BaseRam = 0x00FF2D8CU;
    static constexpr std::size_t kPool2Slots = 6U;
    static constexpr std::size_t kPool2Stride = 90U;

    static constexpr std::uint32_t kPool3BaseRam = 0x00FF2ABCU;
    static constexpr std::size_t kPool3Slots = 8U;
    static constexpr std::size_t kPool3Stride = 90U;

    static constexpr std::uint32_t kPool4BaseRam = 0x00FF19C8U;
    static constexpr std::size_t kPool4Slots = 4U;
    static constexpr std::size_t kPool4Stride = 8U;

    static constexpr std::uint32_t kPlayerSlotRam = 0x00FF19E8U;
    static constexpr std::uint32_t kSpiritSlotRam = 0x00FF1AA4U; // Spirit entity base
    static constexpr std::uint32_t kPool5BaseRam = 0x00FF1B60U;  // Non-player entity slots
    static constexpr std::size_t kPool5Slots = 19U;
    static constexpr std::size_t kPool5Stride = 188U; // 0xBC

    static constexpr std::uint32_t kGlowLevelRam = 0x00FF183EU;
    static constexpr std::uint32_t kActiveSpiritRam = 0x00FF1AA4U;
    static constexpr std::uint32_t kTimerWordRam = 0x00FF199EU;
    static constexpr std::uint32_t kTimerSubbyteRam = 0x00FF199FU;
};

struct EntityDescriptor {
    std::uint16_t sprite_id{0U};      // +0x4A
    std::uint8_t radius{0U};          // +0x46 (also box +0x42, +0x44)
    std::int8_t offset_x{0};          // +0x48
    std::int8_t offset_y{0};          // +0x49
    std::uint8_t tile_attribute{0U};  // +0x3D
    std::uint8_t height{0U};          // +0x4C
    std::uint8_t anim_flags{0U};      // +0x3A
    std::uint8_t collision_flags{0U}; // +0x36
    std::uint8_t defense_flags{0U};   // +0x37
    std::uint8_t render_flags{0U};    // +0x38
    std::uint8_t ai_flags{0U};        // +0x3C
    std::uint32_t fn_ptr_1a{0U};      // +0x1A
    std::uint32_t fn_ptr_1e{0U};      // +0x1E
    std::uint32_t fn_ptr_26{0U};      // +0x26
    std::uint32_t fn_ptr_22{0U};      // +0x22
    std::uint32_t fn_ptr_5e{0U};      // +0x5E (large entities)
};

// 1. Master Entity Subsystems Clear (0x008C5A..0x008D04)
void clear_entity_slot_90(std::span<std::uint8_t> slot) noexcept;
void clear_entity_slot_188(std::span<std::uint8_t> slot) noexcept;
void clear_special_slot_8(std::span<std::uint8_t> slot) noexcept;

struct EntityPoolSet {
    std::vector<std::array<std::uint8_t, 90>> pool1_90b{};    // 4 slots ($FF2954)
    std::vector<std::array<std::uint8_t, 90>> pool2_90b{};    // 6 slots ($FF2D8C)
    std::vector<std::array<std::uint8_t, 90>> pool3_90b{};    // 8 slots ($FF2ABC)
    std::vector<std::array<std::uint8_t, 8>> pool4_8b{};      // 4 slots ($FF19C8)
    std::array<std::uint8_t, 188> player_slot{};             // 1 slot ($FF19E8, preserved)
    std::array<std::uint8_t, 188> spirit_slot{};             // 1 slot ($FF1A9C, preserved)
    std::vector<std::array<std::uint8_t, 188>> pool5_188b{};  // 19 slots ($FF1B60)

    void init_default() noexcept;
    void clear_subsystems() noexcept;
};

// 2. Universal Entity Constructor (0x008D06..0x008DA2)
[[nodiscard]] std::optional<EntityDescriptor> read_entity_descriptor(
    std::span<const std::uint8_t> rom_or_table,
    std::uint16_t entity_type,
    bool is_full_rom = true) noexcept;

void construct_entity(
    std::span<std::uint8_t> slot,
    std::uint16_t entity_type,
    const EntityDescriptor& desc,
    bool is_small_slot) noexcept;

// 3. Entity Script Pointers Re-init (0x008DA4..0x008DD2)
void reinit_entity_script_pointers(
    std::span<std::uint8_t> slot,
    const EntityDescriptor& desc) noexcept;

// 4. Spirit Glow Pulse Controller (0x008DD4..0x008E22)
[[nodiscard]] std::uint8_t step_spirit_glow_pulse(
    std::uint8_t current_glow,
    bool has_spirit,
    std::uint16_t anim_timer) noexcept;

} // namespace oasis::game::player
