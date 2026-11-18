#include "spirit_warp_bgm.hpp"

namespace oasis::game::world {

namespace {

inline std::int16_t read_be16_signed(std::span<const std::uint8_t> data, std::size_t offset) noexcept {
    if (offset + 1U >= data.size()) {
        return 0;
    }
    const auto u = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8) |
        static_cast<std::uint16_t>(data[offset + 1U]));
    return static_cast<std::int16_t>(u);
}

inline std::uint16_t read_be16(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ram[off]) << 8) |
                                      static_cast<std::uint16_t>(ram[off + 1U]));
}

inline void write_be16(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint32_t read_be32(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(ram[off]) << 24) |
           (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
           (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
           static_cast<std::uint32_t>(ram[off + 3U]);
}

inline void write_be32(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint32_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
    ram[off + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint8_t read_u8(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return 0U;
    }
    return ram[off];
}

inline void write_u8(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint8_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return;
    }
    ram[off] = val;
}

} // namespace

void reposition_spirit_on_room_warp_fixed(
    SpiritState& spirit,
    std::int32_t ali_x_fixed,
    std::int32_t ali_y_fixed,
    std::int16_t ali_elevation,
    bool room_forbids_spirits) noexcept {
    // 0x00CC18: clr.b $39(a1)
    spirit.mode_byte = 0U;

    // Room spirit forbid flag: unsummon unconditionally (0x00CCFA..0x00CCFC)
    if (room_forbids_spirits) {
        spirit.type = 0U;
        spirit.active = false;
        return;
    }

    // 0x00CC2C: cmpi.w #$16, $0(a1) -> unsummon Bow (0x00CC66)
    if (spirit.type == SpiritWarpBgmConfig::kTypeUnsummonBow) {
        spirit.type = 0U;
        spirit.active = false;
        return;
    }

    // 0x00CC1C: cmpi.w #$1c, $0(a1) -> Dytto / Efreet state sync (0x00CC6C..0x00CC84)
    if (spirit.type == SpiritWarpBgmConfig::kTypeDyttoEfreet) {
        spirit.state_synced = true;
        return;
    }

    // 0x00CC24: cmpi.w #$1e, $0(a1) -> Shade projection helper with Ali coords (0x00CC86..0x00CCA0)
    if (spirit.type == SpiritWarpBgmConfig::kTypeShade) {
        spirit.projected = true;
        spirit.x_fixed = ali_x_fixed;
        spirit.y_fixed = ali_y_fixed;
        spirit.elevation = ali_elevation;
        return;
    }

    // 0x00CC34..0x00CC40: move.l $8(a0), $8(a1); move.l $c(a0), $c(a1); subq.w #$8, $c(a1)
    spirit.x_fixed = ali_x_fixed;
    spirit.y_fixed = ali_y_fixed - (static_cast<std::int32_t>(8) << 16);

    // 0x00CC44..0x00CC64: cmpi.w #$1a, $0(a1)
    if (spirit.type == SpiritWarpBgmConfig::kTypeResetState) {
        spirit.mode = 0U;
        spirit.facing = 0U;
        spirit.counter_9e = 0U;
        spirit.counter_a2 = 0U;
        spirit.subpixel_vel = 0U;
    }
}

std::uint32_t resolve_collision_table_pointer(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept {
    const auto area_id = static_cast<std::uint8_t>((packed_id >> 8) & 0x1FU);
    const auto scene_id = static_cast<std::uint8_t>(packed_id & 0xFFU);

    if (area_id >= SpiritWarpBgmConfig::kCollisionAreaCount) {
        return 0U;
    }

    std::uint32_t area_base = SpiritWarpBgmConfig::kCollisionAreaTable[area_id];
    constexpr std::size_t kTableOffset = SpiritWarpBgmConfig::kCollisionAreaTableAddr;
    if (rom.size() >= kTableOffset + (SpiritWarpBgmConfig::kCollisionAreaCount * 4U)) {
        const std::uint32_t rom_base =
            (static_cast<std::uint32_t>(rom[kTableOffset + area_id * 4U]) << 24) |
            (static_cast<std::uint32_t>(rom[kTableOffset + area_id * 4U + 1U]) << 16) |
            (static_cast<std::uint32_t>(rom[kTableOffset + area_id * 4U + 2U]) << 8) |
            static_cast<std::uint32_t>(rom[kTableOffset + area_id * 4U + 3U]);
        if (rom_base != 0U) {
            area_base = rom_base;
        }
    }

    const std::size_t scene_entry_addr = static_cast<std::size_t>(area_base + scene_id * 2U);
    if (scene_entry_addr + 2U > rom.size()) {
        return static_cast<std::uint32_t>(scene_entry_addr);
    }

    const std::int16_t scene_offset = read_be16_signed(rom, scene_entry_addr);
    const auto collision_ptr = static_cast<std::int32_t>(scene_entry_addr) + scene_offset;
    return static_cast<std::uint32_t>(collision_ptr);
}

CollisionTableResult load_room_collision_table(
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept {
    CollisionTableResult result;
    result.collision_ptr = resolve_collision_table_pointer(rom, packed_id);
    result.cadence_timer = SpiritWarpBgmConfig::kCollisionTickCadence;
    result.busy_flag_set = true;
    return result;
}

BgmSyncResult synchronize_bgm_track(
    std::uint16_t current_bgm,
    std::uint16_t target_bgm) noexcept {
    BgmSyncResult result;
    if (target_bgm == current_bgm) {
        result.sound_cmd = SpiritWarpBgmConfig::kBgmCmdContinue; // 3
        result.current_bgm = current_bgm;
        result.track_changed = false;
    } else {
        result.sound_cmd = target_bgm;
        result.current_bgm = target_bgm;
        result.track_changed = true;
    }
    return result;
}

std::size_t count_active_group2_entities(
    std::span<const std::uint8_t> ram) noexcept {
    std::size_t active_count = 0U;
    std::uint32_t slot_addr = SpiritWarpBgmConfig::kPlayerAliSlotAddr;

    for (std::size_t i = 0U; i < SpiritWarpBgmConfig::kEntityGroup2Count; ++i) {
        const auto type = static_cast<std::int16_t>(read_be16(ram, slot_addr));
        if (type > 0) {
            ++active_count;
        }
        slot_addr += static_cast<std::uint32_t>(SpiritWarpBgmConfig::kEntitySlotStride);
    }
    return active_count;
}

void reposition_spirit_raw(std::span<std::uint8_t> ram, bool room_forbids_spirits) noexcept {
    constexpr std::uint32_t kSpirit = SpiritWarpBgmConfig::kSpiritSlotAddr;
    constexpr std::uint32_t kAli = SpiritWarpBgmConfig::kPlayerAliSlotAddr;

    SpiritState spirit;
    spirit.type = read_be16(ram, kSpirit + 0x00U);
    spirit.active = (spirit.type != 0U);
    spirit.mode = read_be16(ram, kSpirit + 0x04U);
    spirit.subpixel_vel = read_be16(ram, kSpirit + 0x06U);
    spirit.x_fixed = static_cast<std::int32_t>(read_be32(ram, kSpirit + 0x08U));
    spirit.y_fixed = static_cast<std::int32_t>(read_be32(ram, kSpirit + 0x0CU));
    spirit.elevation = static_cast<std::int16_t>(read_be16(ram, kSpirit + 0x14U));
    spirit.facing = read_be16(ram, kSpirit + 0x2AU);
    spirit.mode_byte = read_u8(ram, kSpirit + 0x39U);
    spirit.counter_9e = read_u8(ram, kSpirit + 0x9EU);
    spirit.counter_a2 = read_u8(ram, kSpirit + 0xA2U);

    const auto ali_x = static_cast<std::int32_t>(read_be32(ram, kAli + 0x08U));
    const auto ali_y = static_cast<std::int32_t>(read_be32(ram, kAli + 0x0CU));
    const auto ali_elev = static_cast<std::int16_t>(read_be16(ram, kAli + 0x14U));

    reposition_spirit_on_room_warp_fixed(spirit, ali_x, ali_y, ali_elev, room_forbids_spirits);

    write_be16(ram, kSpirit + 0x00U, spirit.type);
    write_be16(ram, kSpirit + 0x04U, spirit.mode);
    write_be16(ram, kSpirit + 0x06U, spirit.subpixel_vel);
    write_be32(ram, kSpirit + 0x08U, static_cast<std::uint32_t>(spirit.x_fixed));
    write_be32(ram, kSpirit + 0x0CU, static_cast<std::uint32_t>(spirit.y_fixed));
    write_be16(ram, kSpirit + 0x14U, static_cast<std::uint16_t>(spirit.elevation));
    write_be16(ram, kSpirit + 0x2AU, spirit.facing);
    write_u8(ram, kSpirit + 0x39U, spirit.mode_byte);
    write_u8(ram, kSpirit + 0x9EU, spirit.counter_9e);
    write_u8(ram, kSpirit + 0xA2U, spirit.counter_a2);
}

std::uint32_t load_room_collision_table_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept {
    // 0x00CCA4: bset.b #$5, $ff164d.l
    const std::uint8_t vdp_flag = read_u8(ram, SpiritWarpBgmConfig::kVdpBusyFlagAddr);
    write_u8(ram, SpiritWarpBgmConfig::kVdpBusyFlagAddr, static_cast<std::uint8_t>(vdp_flag | 0x20U));

    const auto collision_res = load_room_collision_table(rom, packed_id);

    // 0x00CCCE: move.l a0, $ff17b2.l
    write_be32(ram, SpiritWarpBgmConfig::kCollisionTablePtrAddr, collision_res.collision_ptr);

    // 0x00CCD4: sf.b $ff17b6.l
    write_u8(ram, SpiritWarpBgmConfig::kCollisionTickCounterAddr, 0U);

    // 0x00CCDA: move.b #$a, $ff17b7.l
    write_u8(ram, SpiritWarpBgmConfig::kCollisionTickCadenceAddr, collision_res.cadence_timer);

    return collision_res.collision_ptr;
}

BgmSyncResult synchronize_bgm_track_raw(std::span<std::uint8_t> ram) noexcept {
    const std::uint16_t target = read_be16(ram, SpiritWarpBgmConfig::kTargetTrackAddr);
    const std::uint16_t current = read_be16(ram, SpiritWarpBgmConfig::kCurrentTrackAddr);

    const auto sync = synchronize_bgm_track(current, target);
    if (sync.track_changed) {
        write_be16(ram, SpiritWarpBgmConfig::kCurrentTrackAddr, sync.current_bgm);
    }
    return sync;
}

std::size_t latch_all_entity_ground_surfaces_raw(std::span<std::uint8_t> ram) noexcept {
    return count_active_group2_entities(ram);
}

BgmSyncResult init_room_environment_and_sync_bgm_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom,
    std::uint16_t packed_id) noexcept {
    // 0x00CCEA: bset.b #$5, $ff164d.l
    const std::uint8_t vdp_flag = read_u8(ram, SpiritWarpBgmConfig::kVdpBusyFlagAddr);
    write_u8(ram, SpiritWarpBgmConfig::kVdpBusyFlagAddr, static_cast<std::uint8_t>(vdp_flag | 0x20U));

    // 0x00CCF2..0x00CCFC: btst.b #$0, $ff16f1.l; beq; clr.w $ff1aa4.l
    const bool forbids_spirits = (read_u8(ram, SpiritWarpBgmConfig::kRoomSpiritForbidAddr) & 0x01U) != 0U;
    if (forbids_spirits) {
        write_be16(ram, SpiritWarpBgmConfig::kSpiritSlotAddr, 0U);
    }

    // 0x00CD0E: clr.b $ff184f.l
    write_u8(ram, SpiritWarpBgmConfig::kStaleEnemyHudTrackerAddr, 0U);

    // 0x00CD14: clr.l $ff188a.l
    write_be32(ram, SpiritWarpBgmConfig::kSatLinkCounterAddr, 0U);

    // 0x00CD1A: clr.l $ff13cc.l
    write_be32(ram, SpiritWarpBgmConfig::kSatBufferBaseAddr, 0U);

    load_room_collision_table_raw(ram, rom, packed_id);
    latch_all_entity_ground_surfaces_raw(ram);

    return synchronize_bgm_track_raw(ram);
}

} // namespace oasis::game::world
