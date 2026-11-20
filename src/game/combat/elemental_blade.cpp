#include "game/combat/elemental_blade.hpp"

#include <algorithm>
#include <optional>

namespace thor::combat {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(buf[offset]) << 24U) |
           (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(buf[offset + 3U]);
}

[[nodiscard]] std::int32_t read_be32_signed(
    std::span<const std::uint8_t> buf,
    std::size_t offset) noexcept {
    return static_cast<std::int32_t>(read_be32(buf, offset));
}

void write_be16(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::uint16_t value) noexcept {
    buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::uint32_t value) noexcept {
    buf[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32_signed(
    std::span<std::uint8_t> buf,
    std::size_t offset,
    std::int32_t value) noexcept {
    write_be32(buf, offset, static_cast<std::uint32_t>(value));
}

[[nodiscard]] std::optional<std::size_t> normalize_ram_offset(
    std::uint32_t addr,
    std::size_t required_bytes,
    std::size_t ram_size) noexcept {
    std::size_t offset = 0U;
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        offset = static_cast<std::size_t>(addr - 0x00FF0000U);
    } else if (addr < 0x10000U) {
        offset = static_cast<std::size_t>(addr);
    } else {
        return std::nullopt;
    }

    if (offset > ram_size || (ram_size - offset) < required_bytes) {
        return std::nullopt;
    }
    return offset;
}

[[nodiscard]] constexpr std::int32_t pack_i16_to_16_16(std::int16_t integer_val) noexcept {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(integer_val)) << 16U);
}

[[nodiscard]] constexpr std::int32_t scale_diagonal_075(std::int32_t value) noexcept {
    return (value >> 1) + (value >> 2);
}

struct ProjectedVelocity2D {
    std::int32_t vel_x_fp{0};
    std::int32_t vel_y_fp{0};
};

/// Self-contained 8-way velocity projection matching ROM 0x00E2D4..0x00E336.
[[nodiscard]] constexpr ProjectedVelocity2D project_8way_velocity_00e2d4(
    std::uint16_t speed_x,
    std::uint16_t speed_y,
    std::uint16_t direction) noexcept {
    const auto sx = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_x) << 8U);
    const auto sy = static_cast<std::int32_t>(static_cast<std::uint32_t>(speed_y) << 8U);

    switch (direction & 7U) {
    case 0U: // N
        return {0, -sy};
    case 1U: // E
        return {sx, 0};
    case 2U: // S
        return {0, sy};
    case 3U: // W
        return {-sx, 0};
    case 4U: // NE
        return {scale_diagonal_075(sx), -scale_diagonal_075(sy)};
    case 5U: // SE
        return {scale_diagonal_075(sx), scale_diagonal_075(sy)};
    case 6U: // SW
        return {-scale_diagonal_075(sx), scale_diagonal_075(sy)};
    case 7U: // NW
        return {-scale_diagonal_075(sx), -scale_diagonal_075(sy)};
    default:
        return {};
    }
}

[[nodiscard]] constexpr SwordWaveType classify_sword_type(std::uint32_t param_d0) noexcept {
    if ((param_d0 & kSwordWaveMaskOmega) != 0U) {
        return SwordWaveType::Omega;
    }
    if ((param_d0 & kSwordWaveMaskHyper) != 0U) {
        return SwordWaveType::Hyper;
    }
    if ((param_d0 & kSwordWaveMaskFire) != 0U) {
        return SwordWaveType::Fire;
    }
    return SwordWaveType::Standard;
}

} // namespace

SwordWaveConfig decode_sword_wave_parameters(std::uint32_t param_d0) noexcept {
    SwordWaveConfig cfg{};
    cfg.param_bits = param_d0 & (kSwordWaveMaskFire | kSwordWaveMaskOmega | kSwordWaveMaskHyper);
    cfg.elevation = static_cast<std::int16_t>(param_d0 & 0xFFFFU);
    cfg.type = classify_sword_type(param_d0);

    // 0x00F2CA..0x00F2D4: defaults before bitmask checks.
    cfg.damage = kSwordWaveDamageStandard;
    cfg.anim_id = kSwordWaveAnimStandard;

    // 0x00F2D6..0x00F2EC: bit 31 (Fire) falls through without branching!
    if ((param_d0 & kSwordWaveMaskFire) != 0U) {
        cfg.damage = kSwordWaveDamageFire;
        cfg.anim_id = kSwordWaveAnimElemental;
        cfg.flag_3c_bit1 = true;
    }

    // 0x00F2EE..0x00F32E: bit 30 (Omega) -> bra $f330; else bit 29 (Hyper) -> bra $f330; else Default.
    if ((param_d0 & kSwordWaveMaskOmega) != 0U) {
        cfg.piercing = true;
        cfg.damage = kSwordWaveDamageOmega;
        cfg.speed_x = kSwordWaveSpeedXOmega;
        cfg.speed_y = kSwordWaveSpeedYOmega;
    } else if ((param_d0 & kSwordWaveMaskHyper) != 0U) {
        cfg.damage = kSwordWaveDamageHyper;
        cfg.anim_id = kSwordWaveAnimElemental;
        cfg.flag_3c_bit1 = true;
        cfg.speed_x = kSwordWaveSpeedXHyper;
        cfg.speed_y = kSwordWaveSpeedYHyper;
    } else {
        cfg.speed_x = kSwordWaveSpeedXStandard;
        cfg.speed_y = kSwordWaveSpeedYStandard;
    }

    return cfg;
}

SwordWaveConfig get_sword_wave_config(
    SwordWaveType type,
    std::int16_t elevation) noexcept {
    const std::uint32_t param =
        static_cast<std::uint32_t>(type) |
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(elevation));
    return decode_sword_wave_parameters(param);
}

SwordWaveAnchor compute_sword_wave_anchor(
    SwordWaveSurface surface,
    std::uint16_t facing,
    std::int16_t elevation,
    bool slowmo_double_bit0) noexcept {
    const SwordTipOffset tip = get_sword_tip_offset(surface, facing);
    const auto dx_i16 = static_cast<std::int16_t>(tip.dx);
    const auto dy_i16 = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(static_cast<std::int16_t>(tip.dy)) +
        static_cast<std::uint16_t>(elevation));

    std::int32_t anchor_x = pack_i16_to_16_16(dx_i16);
    std::int32_t anchor_y = pack_i16_to_16_16(dy_i16);

    if (slowmo_double_bit0) {
        anchor_x = static_cast<std::int32_t>(static_cast<std::uint32_t>(anchor_x) << 1U);
        anchor_y = static_cast<std::int32_t>(static_cast<std::uint32_t>(anchor_y) << 1U);
    }

    return SwordWaveAnchor{
        .anchor_dx_fp = anchor_x,
        .anchor_dy_fp = anchor_y,
    };
}

SwordWaveSlotState spawn_elemental_blade(
    SwordWaveSurface surface,
    const ElementalBladeOwnerState& owner,
    std::uint32_t param_d0,
    bool slowmo_double_bit0,
    std::uint8_t base_flags_3c) noexcept {
    const SwordWaveConfig cfg = decode_sword_wave_parameters(param_d0);
    const ProjectedVelocity2D vel = project_8way_velocity_00e2d4(
        cfg.speed_x, cfg.speed_y, owner.facing);
    const SwordWaveAnchor anchor = compute_sword_wave_anchor(
        surface, owner.facing, cfg.elevation, slowmo_double_bit0);

    SwordWaveSlotState out{};
    out.type_id = kSwordWaveTypeId;
    out.pos_x_fp = owner.pos_x_fp;
    out.pos_y_fp = owner.pos_y_fp;
    out.pos_z_fp = pack_i16_to_16_16(cfg.elevation);
    out.facing = owner.facing;
    out.sprite_priority_18 = kSwordWaveSpritePriority;
    out.anim_id_2a = cfg.anim_id;
    out.flags_38 = kSwordWaveFlags38;
    out.flags_3c = cfg.flag_3c_bit1
        ? static_cast<std::uint8_t>(base_flags_3c | kSwordWaveFlag3CBit1Mask)
        : base_flags_3c;
    out.flags_3d = kSwordWaveFlags3D;
    out.hitbox_radius_46 = kSwordWaveHitboxRadius;
    out.hitbox_rx_48 = kSwordWaveHitboxRx;
    out.hitbox_ry_49 = kSwordWaveHitboxRy;
    out.hitbox_height_4a = kSwordWaveHitboxHeight;
    out.vel_x_fp = vel.vel_x_fp;
    out.vel_y_fp = vel.vel_y_fp;
    out.vel_z_fp = kSwordWaveFloatVelZFp;
    out.anchor_dx_fp = anchor.anchor_dx_fp;
    out.anchor_dy_fp = anchor.anchor_dy_fp;
    out.lifetime_a6 = kSwordWaveLifetimeFrames;
    out.pierce_a7 = cfg.piercing ? kSwordWaveOmegaPierceByte : 0U;
    out.damage_a8 = cfg.damage;
    out.owner_ptr_aa = owner.owner_ram_addr;
    return out;
}

SwordWaveSlotState load_sword_wave_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept {
    SwordWaveSlotState state{};
    const auto off = normalize_ram_offset(
        slot_ram_addr, kSwordWaveSlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return state;
    }

    state.type_id = read_be16(ram, *off + 0x00U);
    state.pos_x_fp = read_be32_signed(ram, *off + 0x08U);
    state.pos_y_fp = read_be32_signed(ram, *off + 0x0CU);
    state.pos_z_fp = read_be32_signed(ram, *off + 0x10U);
    state.facing = read_be16(ram, *off + 0x16U);
    state.sprite_priority_18 = read_be16(ram, *off + 0x18U);
    state.anim_id_2a = read_be16(ram, *off + 0x2AU);
    state.flags_38 = ram[*off + 0x38U];
    state.flags_3c = ram[*off + 0x3CU];
    state.flags_3d = ram[*off + 0x3DU];
    state.hitbox_radius_46 = read_be16(ram, *off + 0x46U);
    state.hitbox_rx_48 = ram[*off + 0x48U];
    state.hitbox_ry_49 = ram[*off + 0x49U];
    state.hitbox_height_4a = read_be16(ram, *off + 0x4AU);
    state.vel_x_fp = read_be32_signed(ram, *off + 0x4EU);
    state.vel_y_fp = read_be32_signed(ram, *off + 0x52U);
    state.vel_z_fp = read_be32_signed(ram, *off + 0x56U);
    state.anchor_dx_fp = read_be32_signed(ram, *off + 0x72U);
    state.anchor_dy_fp = read_be32_signed(ram, *off + 0x76U);
    state.lifetime_a6 = ram[*off + 0xA6U];
    state.pierce_a7 = ram[*off + 0xA7U];
    state.damage_a8 = read_be16(ram, *off + 0xA8U);
    state.owner_ptr_aa = read_be32(ram, *off + 0xAAU);
    return state;
}

void store_sword_wave_slot_to_ram(
    std::span<std::uint8_t> ram,
    const SwordWaveSlotState& state,
    std::uint32_t slot_ram_addr) noexcept {
    const auto off = normalize_ram_offset(
        slot_ram_addr, kSwordWaveSlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return;
    }

    write_be16(ram, *off + 0x00U, state.type_id);
    write_be32_signed(ram, *off + 0x08U, state.pos_x_fp);
    write_be32_signed(ram, *off + 0x0CU, state.pos_y_fp);
    write_be32_signed(ram, *off + 0x10U, state.pos_z_fp);
    write_be16(ram, *off + 0x16U, state.facing);
    write_be16(ram, *off + 0x18U, state.sprite_priority_18);
    write_be16(ram, *off + 0x2AU, state.anim_id_2a);
    ram[*off + 0x38U] = state.flags_38;
    ram[*off + 0x3CU] = state.flags_3c;
    ram[*off + 0x3DU] = state.flags_3d;
    write_be16(ram, *off + 0x46U, state.hitbox_radius_46);
    ram[*off + 0x48U] = state.hitbox_rx_48;
    ram[*off + 0x49U] = state.hitbox_ry_49;
    write_be16(ram, *off + 0x4AU, state.hitbox_height_4a);
    write_be32_signed(ram, *off + 0x4EU, state.vel_x_fp);
    write_be32_signed(ram, *off + 0x52U, state.vel_y_fp);
    write_be32_signed(ram, *off + 0x56U, state.vel_z_fp);
    write_be32_signed(ram, *off + 0x72U, state.anchor_dx_fp);
    write_be32_signed(ram, *off + 0x76U, state.anchor_dy_fp);
    ram[*off + 0xA6U] = state.lifetime_a6;
    ram[*off + 0xA7U] = state.pierce_a7;
    write_be16(ram, *off + 0xA8U, state.damage_a8);
    write_be32(ram, *off + 0xAAU, state.owner_ptr_aa);
}

SwordWaveSlotState spawn_elemental_blade_from_ram(
    std::span<std::uint8_t> ram,
    SwordWaveSurface surface,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c) noexcept {
    const auto owner_off = normalize_ram_offset(owner_ram_addr, 0x18U, ram.size());
    const auto slot_off = normalize_ram_offset(
        kSwordWaveSlotRamOffset, kSwordWaveSlotStrideBytes, ram.size());
    if (!owner_off.has_value() || !slot_off.has_value() ||
        ram.size() <= kSwordWavePauseSlowmoRamOffset) {
        return SwordWaveSlotState{};
    }

    ElementalBladeOwnerState owner{};
    owner.owner_ram_addr = (owner_ram_addr >= 0x00FF0000U)
        ? owner_ram_addr
        : static_cast<std::uint32_t>(0x00FF0000U + *owner_off);
    owner.pos_x_fp = read_be32_signed(ram, *owner_off + 0x08U);
    owner.pos_y_fp = read_be32_signed(ram, *owner_off + 0x0CU);
    owner.facing = read_be16(ram, *owner_off + 0x16U);

    const bool slowmo_bit0 =
        (ram[kSwordWavePauseSlowmoRamOffset] & kSwordWaveSlowmoDoubleBitMask) != 0U;

    // 0x00F28C (0x008D06): zero projectile slot body (+$00..+$BB) before populating wave fields.
    std::fill_n(
        ram.begin() + static_cast<std::ptrdiff_t>(*slot_off),
        kSwordWaveSlotStrideBytes,
        std::uint8_t{0U});

    const SwordWaveSlotState spawned = spawn_elemental_blade(
        surface, owner, param_d0, slowmo_bit0, initial_flags_3c);
    store_sword_wave_slot_to_ram(ram, spawned, kSwordWaveSlotRamAddr);
    return spawned;
}

SwordWaveSlotState spawn_elemental_blade_ground_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c) noexcept {
    return spawn_elemental_blade_from_ram(
        ram, SwordWaveSurface::Ground, owner_ram_addr, param_d0, initial_flags_3c);
}

SwordWaveSlotState spawn_elemental_blade_water_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint32_t param_d0,
    std::uint8_t initial_flags_3c) noexcept {
    return spawn_elemental_blade_from_ram(
        ram, SwordWaveSurface::Water, owner_ram_addr, param_d0, initial_flags_3c);
}

} // namespace thor::combat
