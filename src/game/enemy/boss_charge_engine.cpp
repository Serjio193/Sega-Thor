#include "game/enemy/boss_charge_engine.hpp"

#include "game/entities/viewport_culler_landing.hpp"
#include "game/physics/prng_vector_engine.hpp"

namespace thor::enemy {

namespace {

[[nodiscard]] std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept
{
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void write_be16(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

void write_be32(
    std::span<std::uint8_t> data,
    std::size_t offset,
    std::uint32_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[offset + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[offset + 3U] = static_cast<std::uint8_t>(value & 0xFFU);
}

[[nodiscard]] std::optional<std::size_t> normalize_ram_offset(
    std::uint32_t addr,
    std::size_t required_bytes,
    std::size_t ram_size) noexcept
{
    std::uint32_t offset = addr;
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        offset = addr - 0x00FF0000U;
    } else if (addr >= ram_size) {
        return std::nullopt;
    }
    const auto off = static_cast<std::size_t>(offset);
    if (off > ram_size || (ram_size - off) < required_bytes) {
        return std::nullopt;
    }
    return off;
}

[[nodiscard]] constexpr std::uint32_t to_canonical_ram_addr(std::uint32_t addr) noexcept {
    if (addr < 0x10000U) {
        return 0x00FF0000U + addr;
    }
    return addr;
}

void clear_horizontal_and_vertical_velocities(BossChargeEntityState& ent) noexcept {
    ent.vel_x_4e = 0;
    ent.vel_y_52 = 0;
    ent.vel_z_56 = 0;
}

[[nodiscard]] BossChargeViewport read_viewport_from_ram(
    std::span<const std::uint8_t, 0x10000> ram) noexcept
{
    const bool use_plane_b = (ram[kBossChargeScrollModeRamOffset] & 0x80U) != 0U;
    const std::size_t cam_off =
        use_plane_b ? kBossChargePlaneBCamRamOffset : kBossChargePlaneACamRamOffset;
    return BossChargeViewport{
        static_cast<std::int16_t>(read_be16(ram, cam_off + 0x00U)),
        static_cast<std::int16_t>(read_be16(ram, cam_off + 0x04U)),
        kBossChargeViewportMarginPx,
    };
}

[[nodiscard]] std::optional<bool> probe_water_if_aux_present(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const bool use_plane_b = (ram[kBossChargeScrollModeRamOffset] & 0x80U) != 0U;
    const std::size_t aux_ctx_off = use_plane_b
        ? thor::physics::kWaterProbeAuxPlaneBRamOffset
        : thor::physics::kWaterProbeAuxPlaneARamOffset;
    const std::uint32_t aux_ptr = read_be32(ram, aux_ctx_off);
    if (aux_ptr == 0U) {
        return std::nullopt;
    }
    return thor::physics::probe_entity_water_surface_from_ram(ram, entity_ram_addr);
}

template <typename StepFn>
BossChargeStepResult execute_state_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr,
    StepFn&& step_fn)
{
    const auto ent_off = normalize_ram_offset(
        entity_ram_addr, kBossChargeEntitySlotBytes, ram.size());
    if (!ent_off.has_value()) {
        return BossChargeStepResult{};
    }

    const std::optional<bool> water_immersed =
        probe_water_if_aux_present(ram, entity_ram_addr);
    BossChargeEntityState ent = load_boss_charge_entity_from_ram(ram, entity_ram_addr);
    const BossChargeViewport viewport = read_viewport_from_ram(ram);

    const BossChargeStepResult res = step_fn(ent, viewport, water_immersed);
    store_boss_charge_entity_to_ram(ram, entity_ram_addr, ent);

    if (res.spawner_notified) {
        const auto spawner_off = normalize_ram_offset(
            res.notified_spawner_addr, 0x08U, ram.size());
        if (spawner_off.has_value()) {
            write_be16(ram, *spawner_off + 0x06U, res.spawner_notify_msg);
        }
    }
    if (res.screen_shake_frames != 0U) {
        write_be16(ram, kBossChargeScreenShakeRamOffset, res.screen_shake_frames);
    }
    return res;
}

} // namespace

void boss_charge_damping_step(BossChargeEntityState& ent) noexcept {
    // 0x00EF5E..0x00EF84: 0.75x damping on X and Y, added to +$72 / +$76
    ent.vel_x_4e = damp_velocity_75_percent(ent.vel_x_4e);
    ent.accum_x_72 += ent.vel_x_4e;
    ent.vel_y_52 = damp_velocity_75_percent(ent.vel_y_52);
    ent.accum_y_76 += ent.vel_y_52;

    // 0x00EF86..0x00EFA0: 0.75x damping stored in +$56, 0.5x (`vel_z >> 1`) added to +$10
    const std::int32_t half_z = ent.vel_z_56 >> 1;
    ent.vel_z_56 = half_z + (half_z >> 1);
    ent.elev_z_10 += half_z;
    if (ent.elev_z_10 < 0) {
        ent.elev_z_10 = 0;
    }
}

BossChargeStepResult boss_charge_despawn_and_notify(BossChargeEntityState& ent) noexcept {
    BossChargeStepResult res{};
    res.despawned = true;

    // 0x00F038..0x00F052: notify +$06(a0) = 0x0010 unless a0 == a6 or a0 == $FF19E8
    const std::uint32_t spawner = ent.spawner_ptr_aa;
    const std::uint32_t self = to_canonical_ram_addr(ent.self_ram_addr);
    if (spawner != 0U && spawner != self && spawner != kBossChargePlayerAliRamAddr) {
        res.spawner_notified = true;
        res.notified_spawner_addr = spawner;
        res.spawner_notify_msg = kBossChargeSpawnerNotifyMsg;
    }
    ent.type_00 = 0;
    return res;
}

BossChargeStepResult boss_charge_trajectory_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport,
    std::optional<bool> water_immersed)
{
    BossChargeStepResult res{};

    // 0x00EFA2: bsr.w $e36e (update bit 3 of +$36 when water probe outcome is provided)
    if (water_immersed.has_value()) {
        ent.flags_36 = static_cast<std::uint8_t>(ent.flags_36 & ~kBossChargeFlagWaterBit);
        if (*water_immersed) {
            ent.flags_36 = static_cast<std::uint8_t>(ent.flags_36 | kBossChargeFlagWaterBit);
        }
    }

    // 0x00EFA6..0x00EFBA: horizontal subpixel advance & gravity
    ent.accum_x_72 += ent.vel_x_4e;
    ent.accum_y_76 += ent.vel_y_52;
    std::int32_t d0 = ent.vel_z_56 - kBossChargeGravityAccel;

    bool skip_to_cull = false;
    if (ent.elev_z_10 == 0) {
        // 0x00EFC6..0x00EFD0: on ground -> water check, then stop if falling downward
        if ((ent.flags_36 & kBossChargeFlagWaterBit) != 0U) {
            return boss_charge_despawn_and_notify(ent);
        }
        if (d0 < 0) {
            clear_horizontal_and_vertical_velocities(ent);
            res.velocity_stopped = true;
            skip_to_cull = true;
        }
    }

    if (!skip_to_cull) {
        // 0x00EFD2..0x00F028: integrate vertical motion & handle floor collision bounce
        ent.vel_z_56 = d0;
        ent.elev_z_10 += d0;
        if (ent.elev_z_10 < 0) {
            ent.elev_z_10 = 0;
            if ((ent.flags_36 & kBossChargeFlagWaterBit) != 0U) {
                return boss_charge_despawn_and_notify(ent);
            }

            d0 = (-d0) >> 1;
            ent.vel_z_56 = d0;
            ent.vel_x_4e >>= 1;
            ent.vel_y_52 >>= 1;
            ent.render_attr_18 ^= kBossChargeRenderHFlipBit;
            res.bounced = true;
            res.sfx_emitted.push_back(kBossChargeSfxBounce);

            // 0x00F018: cmpi.l #$4000, d0; bcc.b $f02c (unsigned check)
            if (static_cast<std::uint32_t>(d0) <
                static_cast<std::uint32_t>(kBossChargeBounceRestThreshold)) {
                clear_horizontal_and_vertical_velocities(ent);
                res.velocity_stopped = true;
            }
        }
    }

    // 0x00F02C..0x00F036: 48px camera margin check via 0x00E610
    if (oasis::game::entities::is_offscreen_ground(
            ent.world_x_08,
            ent.world_y_0c,
            viewport.cam_x,
            viewport.cam_y,
            viewport.margin_px)) {
        BossChargeStepResult despawn = boss_charge_despawn_and_notify(ent);
        despawn.bounced = res.bounced;
        despawn.velocity_stopped = res.velocity_stopped;
        despawn.offscreen_culled = true;
        despawn.sfx_emitted = std::move(res.sfx_emitted);
        return despawn;
    }

    return res;
}

BossChargeStepResult boss_charge_state0_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport,
    std::optional<bool> water_immersed)
{
    BossChargeStepResult res = boss_charge_trajectory_step(ent, viewport, water_immersed);
    if (ent.timer_02 == 0U || --ent.timer_02 == 0U) {
        ent.anim_2a = 2U;
        ent.state_04 = 2U;
        res.state_transitioned = true;
    }
    return res;
}

BossChargeStepResult boss_charge_state2_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport,
    std::optional<bool> water_immersed)
{
    BossChargeStepResult res = boss_charge_trajectory_step(ent, viewport, water_immersed);
    if ((ent.flags_37 & kBossChargeFlagAnimDoneBit) != 0U) {
        ent.anim_2a = 4U;
        ent.state_04 = 4U;
        res.state_transitioned = true;
    }
    return res;
}

BossChargeStepResult boss_charge_state4_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport,
    std::optional<bool> water_immersed)
{
    BossChargeStepResult res = boss_charge_trajectory_step(ent, viewport, water_immersed);
    if ((ent.flags_37 & kBossChargeFlagAnimDoneBit) == 0U) {
        return res;
    }

    // 0x00EEA2..0x00EEC0: clear bits 11/13/14 on +$18, set flags, enter state 6
    ent.render_attr_18 = static_cast<std::uint16_t>(ent.render_attr_18 & kBossChargeRenderState4Mask);
    ent.flags_36 = static_cast<std::uint8_t>(ent.flags_36 | kBossChargeFlagWaterBit);
    ent.flags_38 = static_cast<std::uint8_t>(ent.flags_38 | kBossChargeFlag38Bit3);
    ent.flags_3c = static_cast<std::uint8_t>(ent.flags_3c | kBossChargeFlag3cBit1);
    ent.state_04 = 6U;
    res.state_transitioned = true;

    // 0x00EEC6..0x00EF1C: subtype +$A6 dispatch (0 -> anim 6 + swing, 1 -> anim 10 + swing, else -> state/anim 8 + roar/quake + shake)
    if (ent.subtype_a6 == 0U) {
        ent.anim_2a = 6U;
        res.sfx_emitted.push_back(kBossChargeSfxSwing);
    } else if (ent.subtype_a6 == 1U) {
        ent.anim_2a = 10U;
        res.sfx_emitted.push_back(kBossChargeSfxSwing);
    } else {
        ent.state_04 = 8U;
        ent.anim_2a = 8U;
        res.sfx_emitted.push_back(kBossChargeSfxRoar);
        res.sfx_emitted.push_back(kBossChargeSfxQuake);
        res.screen_shake_frames = kBossChargeScreenShakeFrames;
    }
    return res;
}

BossChargeEntityState load_boss_charge_entity_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    BossChargeEntityState ent{};
    const auto off = normalize_ram_offset(entity_ram_addr, kBossChargeEntitySlotBytes, ram.size());
    if (!off.has_value()) {
        return ent;
    }
    const auto slot = std::span<const std::uint8_t>(ram).subspan(*off, kBossChargeEntitySlotBytes);
    ent.type_00 = static_cast<std::int16_t>(read_be16(slot, 0x00U));
    ent.timer_02 = read_be16(slot, 0x02U);
    ent.state_04 = read_be16(slot, 0x04U);
    ent.world_x_08 = static_cast<std::int16_t>(read_be16(slot, 0x08U));
    ent.world_y_0c = static_cast<std::int16_t>(read_be16(slot, 0x0CU));
    ent.elev_z_10 = static_cast<std::int32_t>(read_be32(slot, 0x10U));
    ent.render_attr_18 = read_be16(slot, 0x18U);
    ent.anim_2a = read_be16(slot, 0x2AU);
    ent.flags_36 = slot[0x36U];
    ent.flags_37 = slot[0x37U];
    ent.flags_38 = slot[0x38U];
    ent.flags_3c = slot[0x3CU];
    ent.vel_x_4e = static_cast<std::int32_t>(read_be32(slot, 0x4EU));
    ent.vel_y_52 = static_cast<std::int32_t>(read_be32(slot, 0x52U));
    ent.vel_z_56 = static_cast<std::int32_t>(read_be32(slot, 0x56U));
    ent.accum_x_72 = static_cast<std::int32_t>(read_be32(slot, 0x72U));
    ent.accum_y_76 = static_cast<std::int32_t>(read_be32(slot, 0x76U));
    ent.subtype_a6 = read_be16(slot, 0xA6U);
    ent.spawner_ptr_aa = read_be32(slot, 0xAAU);
    ent.self_ram_addr = 0x00FF0000U + static_cast<std::uint32_t>(*off);
    return ent;
}

void store_boss_charge_entity_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr,
    const BossChargeEntityState& ent) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, kBossChargeEntitySlotBytes, ram.size());
    if (!off.has_value()) {
        return;
    }
    const auto slot = std::span<std::uint8_t>(ram).subspan(*off, kBossChargeEntitySlotBytes);
    write_be16(slot, 0x00U, static_cast<std::uint16_t>(ent.type_00));
    write_be16(slot, 0x02U, ent.timer_02);
    write_be16(slot, 0x04U, ent.state_04);
    write_be16(slot, 0x08U, static_cast<std::uint16_t>(ent.world_x_08));
    write_be16(slot, 0x0CU, static_cast<std::uint16_t>(ent.world_y_0c));
    write_be32(slot, 0x10U, static_cast<std::uint32_t>(ent.elev_z_10));
    write_be16(slot, 0x18U, ent.render_attr_18);
    write_be16(slot, 0x2AU, ent.anim_2a);
    slot[0x36U] = ent.flags_36;
    slot[0x37U] = ent.flags_37;
    slot[0x38U] = ent.flags_38;
    slot[0x3CU] = ent.flags_3c;
    write_be32(slot, 0x4EU, static_cast<std::uint32_t>(ent.vel_x_4e));
    write_be32(slot, 0x52U, static_cast<std::uint32_t>(ent.vel_y_52));
    write_be32(slot, 0x56U, static_cast<std::uint32_t>(ent.vel_z_56));
    write_be32(slot, 0x72U, static_cast<std::uint32_t>(ent.accum_x_72));
    write_be32(slot, 0x76U, static_cast<std::uint32_t>(ent.accum_y_76));
    write_be16(slot, 0xA6U, ent.subtype_a6);
    write_be32(slot, 0xAAU, ent.spawner_ptr_aa);
}

BossChargeStepResult boss_charge_damping_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, kBossChargeEntitySlotBytes, ram.size());
    if (!off.has_value()) {
        return BossChargeStepResult{};
    }
    BossChargeEntityState ent = load_boss_charge_entity_from_ram(ram, entity_ram_addr);
    boss_charge_damping_step(ent);
    store_boss_charge_entity_to_ram(ram, entity_ram_addr, ent);
    return BossChargeStepResult{};
}

BossChargeStepResult boss_charge_trajectory_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr)
{
    return execute_state_from_ram(ram, entity_ram_addr, boss_charge_trajectory_step);
}

BossChargeStepResult boss_charge_state0_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr)
{
    return execute_state_from_ram(ram, entity_ram_addr, boss_charge_state0_step);
}

BossChargeStepResult boss_charge_state2_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr)
{
    return execute_state_from_ram(ram, entity_ram_addr, boss_charge_state2_step);
}

BossChargeStepResult boss_charge_state4_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr)
{
    return execute_state_from_ram(ram, entity_ram_addr, boss_charge_state4_step);
}

BossChargeStepResult boss_charge_despawn_and_notify_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept
{
    const auto off = normalize_ram_offset(entity_ram_addr, kBossChargeEntitySlotBytes, ram.size());
    if (!off.has_value()) {
        return BossChargeStepResult{};
    }
    BossChargeEntityState ent = load_boss_charge_entity_from_ram(ram, entity_ram_addr);
    const BossChargeStepResult res = boss_charge_despawn_and_notify(ent);
    store_boss_charge_entity_to_ram(ram, entity_ram_addr, ent);
    if (res.spawner_notified) {
        const auto spawner_off = normalize_ram_offset(res.notified_spawner_addr, 0x08U, ram.size());
        if (spawner_off.has_value()) {
            write_be16(ram, *spawner_off + 0x06U, res.spawner_notify_msg);
        }
    }
    return res;
}

} // namespace thor::enemy
