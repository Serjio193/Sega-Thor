#include "bomb_detonation.hpp"

namespace oasis::game::combat {

namespace {

void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void write_be32(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    const auto uval = static_cast<std::uint32_t>(val);
    buf[offset] = static_cast<std::uint8_t>((uval >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((uval >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((uval >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(uval & 0xFFU);
}

} // namespace

std::int32_t compute_blast_z(std::int32_t z_16_16) noexcept {
    if (z_16_16 <= kBlastZDrop16_16) {
        return 0;
    }
    return z_16_16 - kBlastZDrop16_16;
}

std::uint16_t compute_blast_tile_attr(std::uint16_t ali_tile_attr_5a) noexcept {
    return ali_tile_attr_5a & kBlastTileAttrMask;
}

BombDetonationResult detonate_bomb(
    const BombDetonationRequest& request) noexcept {
    BombDetonationResult result{};

    result.blast_entity.entity_type = kBlastEntityType;
    result.blast_entity.anim_state_4 = kBlastActionState;
    result.blast_entity.pos_x_16_16 = request.pos_x_16_16;
    result.blast_entity.pos_y_16_16 = request.pos_y_16_16;
    result.blast_entity.pos_z_16_16 = compute_blast_z(request.pos_z_16_16);
    result.blast_entity.timer_16 = 0U;
    result.blast_entity.tile_attr_18 = compute_blast_tile_attr(request.ali_tile_attr_5a);
    result.blast_entity.state_2a = kBlastActionState;
    result.blast_entity.flags_36 = 0x08U;
    result.blast_entity.flags_38 = 0x08U;
    result.blast_entity.flags_3c = 0x02U;
    result.blast_entity.attacker_payload_aa = request.attacker_payload_aa;
    result.blast_entity.hit_type_a6 = kBlastHitType;
    result.blast_entity.damage_word_a8 = kBlastDamageWord;

    result.detonation_flag_set = true;
    result.ali_lock_cleared = true;
    result.screen_shake_frames = kScreenShakeFrames;
    result.sfx_1 = kBlastSfx1;
    result.sfx_2 = kBlastSfx2;

    return result;
}

bool write_bomb_blast_to_entity_slot(
    std::span<std::uint8_t> slot_bytes,
    const BombDetonationRequest& request) noexcept {
    if (slot_bytes.size() < kEntityStructMinSize) {
        return false;
    }

    const auto blast_z = compute_blast_z(request.pos_z_16_16);
    const auto tile_attr = compute_blast_tile_attr(request.ali_tile_attr_5a);

    write_be16(slot_bytes, 0x00U, kBlastEntityType);
    write_be16(slot_bytes, 0x04U, kBlastActionState);
    write_be32(slot_bytes, 0x08U, request.pos_x_16_16);
    write_be32(slot_bytes, 0x0CU, request.pos_y_16_16);
    write_be32(slot_bytes, 0x10U, blast_z);
    write_be16(slot_bytes, 0x16U, 0U);
    write_be16(slot_bytes, 0x18U, tile_attr);
    write_be16(slot_bytes, 0x2AU, kBlastActionState);

    slot_bytes[0x36U] |= 0x08U;
    slot_bytes[0x38U] |= 0x08U;
    slot_bytes[0x3CU] |= 0x02U;

    write_be32(slot_bytes, 0xAAU, static_cast<std::int32_t>(request.attacker_payload_aa));
    write_be16(slot_bytes, 0xA6U, kBlastHitType);
    write_be16(slot_bytes, 0xA8U, kBlastDamageWord);

    return true;
}

} // namespace oasis::game::combat
