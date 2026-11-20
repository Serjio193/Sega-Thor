#include "game/combat/bow_arrow_engine.hpp"

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

} // namespace

BowArrowLaunchParams calculate_bow_arrow_launch_params(
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1) noexcept {
    // 0x00F444..0x00F45A: cmpi.w #$10, $10(a6); bcc.b $f45c (unsigned compare on upper u16 word).
    const auto elev_u16 = static_cast<std::uint16_t>(elevation_d1);
    const bool high_or_wrapped = elev_u16 >= kBowArrowElevationThresholdPx;

    BowArrowLaunchParams params{};
    params.subtype_a6 = subtype_d0;
    params.initial_elevation = elevation_d1;
    params.high_or_wrapped_arc = high_or_wrapped;
    if (high_or_wrapped) {
        params.speed_x = kBowArrowHighArcSpeed;
        params.speed_y = kBowArrowHighArcSpeed;
        params.vel_z_fp = kBowArrowHighArcVelZFp;
    } else {
        params.speed_x = kBowArrowLowArcSpeed;
        params.speed_y = kBowArrowLowArcSpeed;
        params.vel_z_fp = kBowArrowLowArcVelZFp;
    }
    return params;
}

BowArrowAnchor compute_bow_arrow_anchor(
    BowArrowSurface surface,
    std::uint16_t owner_facing,
    std::int16_t elevation_d1,
    bool slowmo_double_bit0) noexcept {
    const BowArrowTipOffset tip = get_bow_arrow_tip_offset(surface, owner_facing);
    const auto dx_i16 = static_cast<std::int16_t>(tip.dx);
    const auto dy_i16 = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(static_cast<std::int16_t>(tip.dy)) +
        static_cast<std::uint16_t>(elevation_d1));

    std::int32_t anchor_x = pack_i16_to_16_16(dx_i16);
    std::int32_t anchor_y = pack_i16_to_16_16(dy_i16);
    if (slowmo_double_bit0) {
        anchor_x = static_cast<std::int32_t>(static_cast<std::uint32_t>(anchor_x) << 1U);
        anchor_y = static_cast<std::int32_t>(static_cast<std::uint32_t>(anchor_y) << 1U);
    }
    return BowArrowAnchor{
        .anchor_dx_fp = anchor_x,
        .anchor_dy_fp = anchor_y,
    };
}

BowArrowSlotState spawn_bow_arrow(
    BowArrowSurface surface,
    const BowArrowOwnerState& owner,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2,
    bool slowmo_double_bit0) noexcept {
    const BowArrowAnchor anchor = compute_bow_arrow_anchor(
        surface, owner.facing, elevation_d1, slowmo_double_bit0);
    const BowArrowLaunchParams arc = calculate_bow_arrow_launch_params(subtype_d0, elevation_d1);
    const ProjectedVelocity2D vel = project_8way_velocity_00e2d4(
        arc.speed_x, arc.speed_y, owner.facing);

    BowArrowSlotState out{};
    out.type_id = kBowArrowTypeId;
    out.param_or_lifetime_02 = param_d2;
    out.state_04 = 0U;
    out.pos_x_fp = owner.pos_x_fp;
    out.pos_y_fp = owner.pos_y_fp;
    out.pos_z_fp = pack_i16_to_16_16(elevation_d1);
    out.facing_16 = 0U; // 0x00F3D2: clr.w $16(a6)
    out.sprite_priority_18 = owner.sprite_priority;
    out.anim_id_2a = 0U;
    out.vel_x_fp = vel.vel_x_fp;
    out.vel_y_fp = vel.vel_y_fp;
    out.vel_z_fp = arc.vel_z_fp;
    out.anchor_dx_fp = anchor.anchor_dx_fp;
    out.anchor_dy_fp = anchor.anchor_dy_fp;
    out.subtype_a6 = subtype_d0;
    out.damage_a8 = kBowArrowDamageFlags;
    out.owner_ptr_aa = owner.owner_ram_addr;
    return out;
}

BowArrowSlotState load_bow_arrow_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept {
    BowArrowSlotState state{};
    const auto off = normalize_ram_offset(
        slot_ram_addr, kBowArrowSlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return state;
    }

    state.type_id = read_be16(ram, *off + 0x00U);
    state.param_or_lifetime_02 = read_be16(ram, *off + 0x02U);
    state.state_04 = read_be16(ram, *off + 0x04U);
    state.pos_x_fp = read_be32_signed(ram, *off + 0x08U);
    state.pos_y_fp = read_be32_signed(ram, *off + 0x0CU);
    state.pos_z_fp = read_be32_signed(ram, *off + 0x10U);
    state.facing_16 = read_be16(ram, *off + 0x16U);
    state.sprite_priority_18 = read_be16(ram, *off + 0x18U);
    state.anim_id_2a = read_be16(ram, *off + 0x2AU);
    state.vel_x_fp = read_be32_signed(ram, *off + 0x4EU);
    state.vel_y_fp = read_be32_signed(ram, *off + 0x52U);
    state.vel_z_fp = read_be32_signed(ram, *off + 0x56U);
    state.anchor_dx_fp = read_be32_signed(ram, *off + 0x72U);
    state.anchor_dy_fp = read_be32_signed(ram, *off + 0x76U);
    state.subtype_a6 = read_be16(ram, *off + 0xA6U);
    state.damage_a8 = read_be16(ram, *off + 0xA8U);
    state.owner_ptr_aa = read_be32(ram, *off + 0xAAU);
    return state;
}

void store_bow_arrow_slot_to_ram(
    std::span<std::uint8_t> ram,
    const BowArrowSlotState& state,
    std::uint32_t slot_ram_addr) noexcept {
    const auto off = normalize_ram_offset(
        slot_ram_addr, kBowArrowSlotStrideBytes, ram.size());
    if (!off.has_value()) {
        return;
    }

    write_be16(ram, *off + 0x00U, state.type_id);
    write_be16(ram, *off + 0x02U, state.param_or_lifetime_02);
    write_be16(ram, *off + 0x04U, state.state_04);
    write_be32_signed(ram, *off + 0x08U, state.pos_x_fp);
    write_be32_signed(ram, *off + 0x0CU, state.pos_y_fp);
    write_be32_signed(ram, *off + 0x10U, state.pos_z_fp);
    write_be16(ram, *off + 0x16U, state.facing_16);
    write_be16(ram, *off + 0x18U, state.sprite_priority_18);
    write_be16(ram, *off + 0x2AU, state.anim_id_2a);
    write_be32_signed(ram, *off + 0x4EU, state.vel_x_fp);
    write_be32_signed(ram, *off + 0x52U, state.vel_y_fp);
    write_be32_signed(ram, *off + 0x56U, state.vel_z_fp);
    write_be32_signed(ram, *off + 0x72U, state.anchor_dx_fp);
    write_be32_signed(ram, *off + 0x76U, state.anchor_dy_fp);
    write_be16(ram, *off + 0xA6U, state.subtype_a6);
    write_be16(ram, *off + 0xA8U, state.damage_a8);
    write_be32(ram, *off + 0xAAU, state.owner_ptr_aa);
}

BowArrowSlotState spawn_bow_arrow_from_ram(
    std::span<std::uint8_t> ram,
    BowArrowSurface surface,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept {
    const auto owner_off = normalize_ram_offset(owner_ram_addr, 0x5CU, ram.size());
    const auto slot_off = normalize_ram_offset(
        kBowArrowSlotRamOffset, kBowArrowSlotStrideBytes, ram.size());
    if (!owner_off.has_value() || !slot_off.has_value() ||
        ram.size() <= kBowArrowPauseSlowmoRamOffset) {
        return BowArrowSlotState{};
    }

    BowArrowOwnerState owner{};
    owner.owner_ram_addr = (owner_ram_addr >= 0x00FF0000U)
        ? owner_ram_addr
        : static_cast<std::uint32_t>(0x00FF0000U + *owner_off);
    owner.pos_x_fp = read_be32_signed(ram, *owner_off + 0x08U);
    owner.pos_y_fp = read_be32_signed(ram, *owner_off + 0x0CU);
    owner.facing = read_be16(ram, *owner_off + 0x16U);
    owner.sprite_priority = read_be16(ram, *owner_off + 0x5AU);

    const bool slowmo_bit0 =
        (ram[kBowArrowPauseSlowmoRamOffset] & kBowArrowSlowmoDoubleBitMask) != 0U;

    // 0x00F3C8 (0x008D06): zero projectile slot body (+$00..+$BB) before writing arrow fields.
    std::fill_n(
        ram.begin() + static_cast<std::ptrdiff_t>(*slot_off),
        kBowArrowSlotStrideBytes,
        std::uint8_t{0U});

    const BowArrowSlotState spawned = spawn_bow_arrow(
        surface, owner, subtype_d0, elevation_d1, param_d2, slowmo_bit0);
    store_bow_arrow_slot_to_ram(ram, spawned, kBowArrowSlotRamAddr);
    return spawned;
}

BowArrowSlotState spawn_bow_arrow_ground_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept {
    return spawn_bow_arrow_from_ram(
        ram, BowArrowSurface::Ground, owner_ram_addr, subtype_d0, elevation_d1, param_d2);
}

BowArrowSlotState spawn_bow_arrow_water_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t owner_ram_addr,
    std::uint16_t subtype_d0,
    std::int16_t elevation_d1,
    std::uint16_t param_d2) noexcept {
    return spawn_bow_arrow_from_ram(
        ram, BowArrowSurface::Water, owner_ram_addr, subtype_d0, elevation_d1, param_d2);
}

} // namespace thor::combat
