#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace thor::world {

// ROM entrypoints and layout (0x00DEEC..0x00E0BA, 462 bytes: 104B code + 356B table + 2B rts).
inline constexpr std::uint32_t kMasterGroup2EntityDispatcherAddr = 0x00DEECU;
inline constexpr std::uint32_t kGroup2ActionTableAddr = 0x00DF54U;
inline constexpr std::uint32_t kGroup2DummyHandlerAddr = 0x00E0B8U;
inline constexpr std::uint32_t kMasterGroup2EntityDispatcherEndAddr = 0x00E0BAU;
inline constexpr std::size_t kGroup2DispatcherCodeBytes = 104U;
inline constexpr std::size_t kGroup2ActionTableBytes = 356U;
inline constexpr std::size_t kGroup2DispatcherTotalBytes = 462U;

// Group 2 pool and pause gate in 64KB Work RAM ($FF0000..$FFFFFF).
// Slot 0 ($FF1AA4) is the companion spirit slot; slots 1..19 ($FF1B60..$FF2898) are general entities.
inline constexpr std::uint32_t kGroup2PauseGateRamAddr = 0x00FF17B8U;
inline constexpr std::uint32_t kGroup2PauseGateRamOffset = 0x17B8U;
inline constexpr std::uint8_t kGroup2PauseGateBit = 0x02U; // bit 1 of $FF17B8
inline constexpr std::uint32_t kGroup2PoolRamAddr = 0x00FF1AA4U;
inline constexpr std::uint32_t kGroup2PoolRamOffset = 0x1AA4U;
inline constexpr std::size_t kGroup2SlotCount = 20U; // moveq #$13, d7 -> 20 dbra iterations
inline constexpr std::size_t kGroup2SlotStride = 0xBCU; // 188 bytes
inline constexpr std::uint8_t kGroup2FlagsSwapXyBit = 0x01U; // +$17 bit 0 (facing odd)

// Dispatch table bounds (89 even types: 0x0016..0x00C6).
inline constexpr std::uint16_t kMinGroup2Type = 0x0016U;
inline constexpr std::uint16_t kMaxGroup2Type = 0x00C6U;
inline constexpr std::size_t kGroup2TableEntryCount = 89U;

// Notable Group 2 action handlers in the 89-entry table.
inline constexpr std::uint32_t kGroup2HandlerBow = 0x0115C8U;            // 0x16
inline constexpr std::uint32_t kGroup2HandlerDitte = 0x011B4EU;          // 0x18
inline constexpr std::uint32_t kGroup2HandlerEfreet = 0x01254CU;         // 0x1A
inline constexpr std::uint32_t kGroup2HandlerShadow = 0x012F1EU;         // 0x1C
inline constexpr std::uint32_t kGroup2HandlerSpiritInteract = 0x01387CU; // 0x1E
inline constexpr std::uint32_t kGroup2HandlerGeneralNpc = 0x00E25CU;     // 0x2E

/// 89-entry 32-bit action handler table at ROM 0x00DF54..0x00E0B8 (even types 0x16..0xC6).
inline constexpr std::array<std::uint32_t, kGroup2TableEntryCount> kGroup2ActionHandlerAddrs = {{
    0x0115C8U, 0x011B4EU, 0x01254CU, 0x012F1EU, 0x01387CU, 0x00E20AU, 0x01A184U, 0x014848U,
    0x021F44U, 0x0194AEU, 0x00E0B8U, 0x0254EAU, 0x00E25CU, 0x015AE2U, 0x018072U, 0x00EA04U,
    0x01C772U, 0x01AC90U, 0x00EDAAU, 0x017D12U, 0x00E0B8U, 0x016BD0U, 0x016C5CU, 0x016CBEU,
    0x0173D6U, 0x01778CU, 0x016D5CU, 0x016DCEU, 0x016F7CU, 0x0170E0U, 0x0171C4U, 0x00E0B8U,
    0x00E0B8U, 0x021B2CU, 0x0178EAU, 0x0178EAU, 0x0178EAU, 0x0178EAU, 0x017908U, 0x0179A8U,
    0x0179D4U, 0x025B32U, 0x00E144U, 0x01F5A4U, 0x01EB54U, 0x01DF1CU, 0x01D140U, 0x027C8AU,
    0x0283E6U, 0x00E9DEU, 0x00E0B8U, 0x00E0B8U, 0x0172C2U, 0x028810U, 0x028D8CU, 0x02903CU,
    0x01CD5AU, 0x020BDAU, 0x020684U, 0x01FA78U, 0x029452U, 0x0269EEU, 0x029E32U, 0x019098U,
    0x01933EU, 0x016EBCU, 0x015316U, 0x02A652U, 0x02A374U, 0x02A9F4U, 0x02AD2CU, 0x02B194U,
    0x02BDF2U, 0x02C27AU, 0x02BF3EU, 0x023D02U, 0x0246CCU, 0x00E0B8U, 0x016ADEU, 0x022CE0U,
    0x017E28U, 0x02B582U, 0x016A64U, 0x016F10U, 0x01906CU, 0x0247AAU, 0x025054U, 0x025114U,
    0x016CBEU,
}};

/// Minimal Group 2 entity fields accessed by 0x00DEEC..0x00DF52.
struct Group2Entity {
    std::int16_t type{0};              // +$00: Entity type ID (<=0 or <0x16 skipped)
    std::uint8_t flags_17{0U};         // +$17: Bit 0 = swap X/Y extents (facing odd)
    std::int16_t effective_half_w{0};  // +$42: Effective collision half-width
    std::int16_t effective_half_h{0};  // +$44: Effective 3/4 isometric half-height
    std::uint8_t raw_extent_x{0U};     // +$48: Raw X half-extent
    std::uint8_t raw_extent_y{0U};     // +$49: Raw Y half-extent

    [[nodiscard]] constexpr bool operator==(const Group2Entity&) const noexcept = default;
};

/// Post-tick projected 2D isometric ground footprint (+$42, +$44).
struct IsometricFootprint {
    std::int16_t half_width{0};
    std::int16_t half_height{0};

    [[nodiscard]] constexpr bool operator==(const IsometricFootprint&) const noexcept = default;
};

/// Per-slot dispatch trace record.
struct Group2DispatchEvent {
    std::uint8_t slot_index{0U};
    std::uint32_t slot_ram_addr{0U};
    std::int16_t entity_type{0};
    std::uint32_t handler_rom_addr{0U};
    bool is_dummy_handler{false};
    IsometricFootprint post_tick_footprint{};

    [[nodiscard]] constexpr bool operator==(const Group2DispatchEvent&) const noexcept = default;
};

/// Summary result of one Group 2 dispatch pass (0x00DEEC..0x00DF52).
struct Group2DispatchResult {
    bool paused{false};
    std::size_t dispatched_count{0U};
    std::size_t dummy_count{0U};
    std::size_t skipped_count{0U};
    std::vector<Group2DispatchEvent> events{};
};

using EntityTickFn = void (*)(Group2Entity&);
using Group2TableFnArray = std::array<EntityTickFn, kGroup2TableEntryCount>;
using Group2TickCallback = std::function<void(
    std::size_t slot_index,
    Group2Entity& entity,
    std::uint32_t handler_rom_addr)>;
using Group2RamTickCallback = std::function<void(
    std::size_t slot_index,
    std::span<std::uint8_t, kGroup2SlotStride> slot_bytes,
    std::uint32_t handler_rom_addr)>;

/// Converts signed entity type (+$00) to jump-table index [0..88], or nullopt if skipped/invalid.
[[nodiscard]] constexpr std::optional<std::size_t> group2_type_to_table_index(
    std::int16_t entity_type) noexcept
{
    if (entity_type <= 0 ||
        static_cast<std::uint16_t>(entity_type) < kMinGroup2Type ||
        static_cast<std::uint16_t>(entity_type) > kMaxGroup2Type ||
        (static_cast<std::uint16_t>(entity_type) & 1U) != 0U) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(
        static_cast<std::uint16_t>(entity_type) - kMinGroup2Type) >> 1U;
}

/// Looks up the ROM action handler address for entity_type in kGroup2ActionHandlerAddrs.
[[nodiscard]] constexpr std::optional<std::uint32_t> lookup_group2_action_handler(
    std::int16_t entity_type) noexcept
{
    const auto idx = group2_type_to_table_index(entity_type);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    return kGroup2ActionHandlerAddrs[*idx];
}

/// Reads the 32-bit big-endian handler address directly from ROM bytes (0x00DF28 + 2 * type).
[[nodiscard]] std::optional<std::uint32_t> lookup_group2_action_handler_in_rom(
    std::span<const std::uint8_t> rom,
    std::int16_t entity_type) noexcept;

/// 0x00DF20..0x00DF49: Projects raw_x (+$48), raw_y (+$49), and swap_xy (+$17 bit 0)
/// into (+$42 half_width, +$44 half_height) using d2 = (d2 + 1) >> 1; eff_h = d2 + (d2 >> 1).
[[nodiscard]] constexpr IsometricFootprint project_isometric_footprint(
    std::uint8_t raw_x,
    std::uint8_t raw_y,
    bool swap_xy) noexcept
{
    std::uint16_t d1 = raw_x;
    std::uint16_t d2 = raw_y;
    if (swap_xy) {
        std::swap(d1, d2);
    }
    const auto eff_w = static_cast<std::int16_t>(d1);
    const auto half = static_cast<std::uint16_t>((d2 + 1U) >> 1U);
    const auto eff_h = static_cast<std::int16_t>(half + (half >> 1U));
    return IsometricFootprint{eff_w, eff_h};
}

/// Applies post-tick isometric footprint projection to a Group2Entity (+$42, +$44).
IsometricFootprint apply_post_tick_footprint(Group2Entity& entity) noexcept;

/// Applies post-tick isometric footprint projection to a raw 188-byte RAM slot.
IsometricFootprint apply_post_tick_footprint_to_slot(
    std::span<std::uint8_t, kGroup2SlotStride> slot_bytes) noexcept;

/// Dispatches 20 Group 2 entities via a caller-provided 89-entry C++ function pointer table.
Group2DispatchResult dispatch_group2_entities(
    std::span<Group2Entity, kGroup2SlotCount> pool,
    bool is_paused,
    const Group2TableFnArray& jump_table);

/// Dispatches Group 2 entities using the ROM handler address table and an optional callback.
Group2DispatchResult dispatch_group2_entities(
    std::span<Group2Entity> pool,
    bool is_paused,
    const Group2TickCallback& tick_cb = {});

/// 64KB RAM window helper modeling 0x00DEEC..0x00DF52 across $FF17B8 bit 1 and $FF1AA4[0..19].
Group2DispatchResult dispatch_group2_entities_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    const Group2RamTickCallback& tick_cb = {},
    std::span<const std::uint8_t> rom = {});

} // namespace thor::world

namespace oasis::game::world {
using namespace thor::world;
} // namespace oasis::game::world
