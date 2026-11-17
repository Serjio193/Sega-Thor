#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::combat {

inline constexpr std::uint32_t kBombDetonationAddr = 0x00ED04U;

inline constexpr std::uint16_t kBlastEntityType = 0x003AU;
inline constexpr std::int32_t kBlastZDrop16_16 = 0x00040000; // 4.0 px
inline constexpr std::uint16_t kBlastDamageWord = 0x8007U;
inline constexpr std::uint16_t kBlastHitType = 0x0002U;
inline constexpr std::uint16_t kBlastActionState = 8U;
inline constexpr std::uint32_t kBlastSfx1 = 0x00B00225U;
inline constexpr std::uint32_t kBlastSfx2 = 0x00600382U;
inline constexpr std::uint16_t kScreenShakeFrames = 4U;
inline constexpr std::uint32_t kScreenShakeRamAddr = 0x00FF17C4U;
inline constexpr std::uint32_t kDetonationFlagRamAddr = 0x00FF198FU;
inline constexpr std::uint32_t kAliEntityRamAddr = 0x00FF19E8U;
inline constexpr std::uint16_t kBlastTileAttrMask = 0x97FFU;
inline constexpr std::size_t kEntityStructMinSize = 0xBCU;

struct BombDetonationRequest {
    std::int32_t pos_x_16_16{0};
    std::int32_t pos_y_16_16{0};
    std::int32_t pos_z_16_16{0};
    std::uint32_t attacker_payload_aa{kAliEntityRamAddr};
    std::uint16_t ali_tile_attr_5a{0U};
};

struct BombBlastEntityState {
    std::uint16_t entity_type{kBlastEntityType};
    std::uint16_t anim_state_4{kBlastActionState};
    std::int32_t pos_x_16_16{0};
    std::int32_t pos_y_16_16{0};
    std::int32_t pos_z_16_16{0};
    std::uint16_t timer_16{0U};
    std::uint16_t tile_attr_18{0U};
    std::uint16_t state_2a{kBlastActionState};
    std::uint8_t flags_36{0U};
    std::uint8_t flags_38{0U};
    std::uint8_t flags_3c{0U};
    std::uint32_t attacker_payload_aa{0U};
    std::uint16_t hit_type_a6{kBlastHitType};
    std::uint16_t damage_word_a8{kBlastDamageWord};
};

struct BombDetonationResult {
    BombBlastEntityState blast_entity{};
    bool detonation_flag_set{true};
    bool ali_lock_cleared{true};
    std::uint16_t screen_shake_frames{kScreenShakeFrames};
    std::uint32_t sfx_1{kBlastSfx1};
    std::uint32_t sfx_2{kBlastSfx2};
};

/// Models 0x00ED3A..0x00ED44: lowers Z by 0x00040000 (4.0 px) and clamps >= 0.
[[nodiscard]] std::int32_t compute_blast_z(std::int32_t z_16_16) noexcept;

/// Models 0x00ED48, 0x00ED62, 0x00ED7A: tile attribute mask (copy Ali +0x5A, & 0xF7FF & 0x9FFF -> & 0x97FF).
[[nodiscard]] std::uint16_t compute_blast_tile_attr(std::uint16_t ali_tile_attr_5a) noexcept;

/// Models full routine 0x00ED04..0x00EDA8: converts bomb into blast entity (type 0x3A),
/// lowers Z, sets damage 0x8007, hit type 2, state 8, sets $FF198F, clears Ali lock,
/// triggers dual SFX 0x00B00225 + 0x00600382, and triggers 4-frame screen shake ($FF17C4).
[[nodiscard]] BombDetonationResult detonate_bomb(
    const BombDetonationRequest& request) noexcept;

/// Direct entity slot byte writer for 0xBC-byte entity slot.
bool write_bomb_blast_to_entity_slot(
    std::span<std::uint8_t> slot_bytes,
    const BombDetonationRequest& request) noexcept;

} // namespace oasis::game::combat
