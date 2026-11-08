#include "game/entities/screen_entity.hpp"
#include "game/world/byte_grid.hpp"
#include "game/world/terrain_collision.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game::entities {
namespace {

std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 2U > data.size()) throw std::out_of_range("be16 read out of range");
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
}

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (offset + 4U > data.size()) throw std::out_of_range("be32 read out of range");
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

} // namespace

EntityDescriptor load_entity_descriptor_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id) {
    if (type_id < 2U || (type_id & 1U) != 0U) {
        throw std::invalid_argument("invalid entity type_id");
    }
    const auto row = static_cast<std::size_t>((type_id / 2U) - 1U);
    const auto entry_offset = ScreenEntity::kEntityTypeTableAddress + row * 32U;
    if (entry_offset + 32U > rom.size()) {
        throw std::out_of_range("entity descriptor table entry outside ROM");
    }

    EntityDescriptor desc{};
    desc.type_id = type_id;
    desc.dim_x = rom[entry_offset + 2U];
    desc.dim_y = rom[entry_offset + 3U];
    desc.gfx_descriptor_pointer = read_be32(rom, entry_offset + 12U);
    const auto seq_ptr = read_be32(rom, entry_offset + 20U);
    desc.anim_descriptor_pointer = (seq_ptr != 0U) ? seq_ptr : read_be32(rom, entry_offset + 16U);
    desc.update_callback_pointer = read_be32(rom, entry_offset + 24U);
    return desc;
}

std::vector<SpritePiece> decode_sprite_frame_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint32_t frame_address) {
    std::vector<SpritePiece> pieces;
    if (frame_address == 0U || frame_address + 2U > rom.size()) {
        return pieces;
    }

    const auto count_word = read_be16(rom, frame_address);
    if ((count_word & 0x8000U) != 0U) {
        return pieces;
    }

    const auto declared_count = static_cast<std::size_t>(count_word) + 1U;
    const auto max_records = std::min(declared_count, std::size_t{16});

    for (std::size_t i = 0U; i < max_records; ++i) {
        const auto rec_offset = frame_address + 2U + i * 6U;
        if (rec_offset + 6U > rom.size()) break;

        const auto w0 = read_be16(rom, rec_offset);
        const auto w1 = read_be16(rom, rec_offset + 2U);
        const auto w2 = read_be16(rom, rec_offset + 4U);

        const auto raw_size = static_cast<std::uint8_t>((w0 >> 8U) & 0xFFU);
        const auto raw_y = static_cast<std::uint8_t>(w0 & 0xFFU);
        const auto signed_y = (raw_y < 0x80U)
            ? static_cast<std::int16_t>(raw_y)
            : static_cast<std::int16_t>(static_cast<std::int32_t>(raw_y) - 0x100);

        const auto signed_x = static_cast<std::int16_t>(w1);

        // Sanity check coordinate bounds for active screen sprite pieces
        if (std::abs(signed_x) > 256 || std::abs(signed_y) > 256) {
            continue;
        }

        SpritePiece piece{};
        piece.width_cells = static_cast<std::uint8_t>(((raw_size >> 2U) & 0x03U) + 1U);
        piece.height_cells = static_cast<std::uint8_t>((raw_size & 0x03U) + 1U);
        piece.x_offset = signed_x;
        piece.y_offset = signed_y;
        piece.tile_index = static_cast<std::uint16_t>(w2 & 0x07FFU);
        piece.palette_line = static_cast<std::uint8_t>((w2 >> 13U) & 0x03U);
        piece.hflip = (w2 & (1U << 11U)) != 0U;
        piece.vflip = (w2 & (1U << 12U)) != 0U;

        pieces.push_back(piece);
    }

    return pieces;
}

ScreenEntity ScreenEntity::spawn_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id,
    std::int32_t world_x,
    std::int32_t world_y,
    std::int32_t world_z,
    std::uint8_t initial_direction) {
    ScreenEntity entity;
    entity.type_id_ = type_id;
    entity.world_x_ = world_x;
    entity.world_y_ = world_y;
    entity.world_z_ = world_z;
    entity.active_ = true;
    entity.rom_ = rom;

    if (!rom.empty()) {
        entity.descriptor_ = load_entity_descriptor_from_rom(rom, type_id);
        if (type_id == 0x0048) {
            entity.actor_.is_actor = true;
            entity.actor_.direction = initial_direction;
            entity.actor_.anchor_x = world_x;
            entity.actor_.anchor_y = world_y;
            entity.actor_.state = 1;
            entity.actor_.anim_selector = 0;
            entity.actor_.wait_duration = 30;
            entity.actor_.speed_fixed = 0x18000;
            entity.apply_actor_anim_frame();
            return entity;
        }

        const auto script_addr = entity.descriptor_.anim_descriptor_pointer;

        bool anim_initialized = false;
        if (script_addr != 0U && script_addr + 2U <= rom.size()) {
            const auto rel = read_be16(rom, script_addr);
            if (rel >= 2U && rel <= 0x100U && script_addr + rel + 4U <= rom.size()) {
                const auto seq_base = script_addr + rel;
                const auto f_idx = read_be16(rom, seq_base);
                const auto raw_dur = read_be16(rom, seq_base + 2U);
                if (f_idx <= 0x100U) {
                    const auto duration = std::max(1U, static_cast<unsigned>((raw_dur & 0x7FFFU) >> 1U));
                    entity.anim_state_.script_address = script_addr;
                    entity.anim_state_.cursor = 0;
                    entity.anim_state_.current_frame_index = f_idx;
                    entity.anim_state_.ticks_remaining = static_cast<std::uint16_t>(duration);
                    entity.anim_state_.loop_pending = (raw_dur & 0x8000U) != 0U;
                    entity.anim_state_.loop_count = 0;
                    entity.anim_state_.halted = false;

                    const auto gfx_base = entity.descriptor_.gfx_descriptor_pointer;
                    const auto entry_addr = gfx_base + static_cast<std::uint32_t>(f_idx) * 2U;
                    if (entry_addr + 2U <= rom.size()) {
                        const auto rel_frame = read_be16(rom, entry_addr);
                        const auto frame_addr = entry_addr + rel_frame;
                        entity.anim_state_.current_frame_pointer = frame_addr;
                        entity.pieces_ = decode_sprite_frame_from_rom(rom, frame_addr);
                        if (!entity.pieces_.empty()) {
                            entity.tile_base_ = entity.pieces_.front().tile_index;
                            anim_initialized = true;
                        }
                    }
                }
            }
        }

        if (!anim_initialized) {
            entity.anim_state_.script_address = script_addr;
            entity.anim_state_.current_frame_pointer = entity.descriptor_.gfx_descriptor_pointer;
            entity.anim_state_.halted = true;
            entity.pieces_ = decode_sprite_frame_from_rom(
                rom, entity.descriptor_.gfx_descriptor_pointer);
            if (!entity.pieces_.empty()) {
                entity.tile_base_ = entity.pieces_.front().tile_index;
            }
        }
    }

    return entity;
}

ScreenEntity ScreenEntity::spawn_drop_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint16_t item_id,
    std::int32_t world_x,
    std::int32_t world_y,
    std::int32_t world_z,
    std::uint8_t count) {
    ScreenEntity entity;
    entity.type_id_ = 0x0008; // Item Entity in ROM
    entity.world_x_ = world_x;
    entity.world_y_ = world_y;
    entity.world_z_ = world_z + 1; // 0x00F4F4: addq.w #1, d0
    entity.active_ = true;
    entity.is_item_ = true;
    entity.item_id_ = item_id;
    entity.item_count_ = count;
    entity.rom_ = rom;
    entity.actor_.state = 2; // Ground / idle state (ROM 0x00F52E)
    if (!rom.empty()) {
        entity.descriptor_ = load_entity_descriptor_from_rom(rom, 0x0008);
        entity.pieces_ = decode_sprite_frame_from_rom(rom, 0x155B76);
        if (!entity.pieces_.empty()) entity.tile_base_ = entity.pieces_.front().tile_index;
    }
    return entity;
}

void ScreenEntity::apply_actor_anim_frame() noexcept {
    if (rom_.empty()) return;
    const std::uint16_t f_idx = (actor_.anim_selector == 2) ? 3 : ((actor_.anim_selector == 4) ? 1 : 0);
    const auto gfx_base = descriptor_.gfx_descriptor_pointer;
    if (gfx_base == 0 || gfx_base + static_cast<std::size_t>(f_idx) * 2U + 2U > rom_.size()) return;
    const auto rel_frame = read_be16(rom_, gfx_base + static_cast<std::size_t>(f_idx) * 2U);
    const auto frame_addr = gfx_base + static_cast<std::size_t>(f_idx) * 2U + rel_frame;
    auto new_pieces = decode_sprite_frame_from_rom(rom_, frame_addr);
    if (!new_pieces.empty()) {
        pieces_ = std::move(new_pieces);
        tile_base_ = pieces_.front().tile_index;
    }
}

void ScreenEntity::update_actor_state_machine(
    const world::ByteGridView* terrain,
    std::int32_t px,
    std::int32_t py) noexcept {
    if (!actor_.is_actor || actor_.halted) return;
    if (actor_.invuln_timer > 0) --actor_.invuln_timer;

    switch (actor_.state) {
    case 0: {
        actor_.anchor_x = world_x_;
        actor_.anchor_y = world_y_;
        actor_.anim_selector = 0;
        actor_.state = 1;
        apply_actor_anim_frame();
        break;
    }
    case 1: {
        const auto dx = px - world_x_;
        const auto dy = py - world_y_;
        bool player_detected = false;
        if (actor_.direction == 0 && std::abs(dx) <= 12 && dy >= -192 && dy <= -8) player_detected = true;
        else if (actor_.direction == 1 && std::abs(dy) <= 12 && dx >= 8 && dx <= 192) player_detected = true;
        else if (actor_.direction == 2 && std::abs(dx) <= 12 && dy >= 8 && dy <= 192) player_detected = true;
        else if (actor_.direction == 3 && std::abs(dy) <= 12 && dx >= -192 && dx <= -8) player_detected = true;

        if (player_detected) {
            actor_.velocity_x_fixed = (actor_.direction == 1) ? actor_.speed_fixed
                : ((actor_.direction == 3) ? -actor_.speed_fixed : 0);
            actor_.velocity_y_fixed = (actor_.direction == 2) ? actor_.speed_fixed
                : ((actor_.direction == 0) ? -actor_.speed_fixed : 0);
            actor_.timer = actor_.wait_duration;
            actor_.anim_selector = 2;
            actor_.state = 2;
            apply_actor_anim_frame();
        }
        break;
    }
    case 2: {
        if (actor_.timer == 0) {
            actor_.direction ^= 2;
            actor_.anim_selector = 4;
            actor_.state = 3;
            apply_actor_anim_frame();
            break;
        }
        --actor_.timer;

        const auto next_x = world_x_ + (actor_.velocity_x_fixed / 0x10000);
        const auto next_y = world_y_ + (actor_.velocity_y_fixed / 0x10000);

        bool blocked = false;
        if (terrain != nullptr) {
            const auto code = terrain->terrain_code_world(next_x, next_y);
            if (!code) {
                blocked = true;
            } else {
                const auto st = world::terrain_state_from_code(*code);
                const auto gate = world::evaluate_terrain_gate({
                    .entity_flags = 0,
                    .current_state = 0,
                    .prospective_state = st,
                    .prospective_any_bits = 0
                });
                if (gate == world::TerrainGateResult::blocked) {
                    blocked = true;
                }
            }
        }

        if (blocked) {
            actor_.direction ^= 2;
            actor_.anim_selector = 4;
            actor_.state = 3;
            apply_actor_anim_frame();
        } else {
            world_x_ = next_x;
            world_y_ = next_y;
        }
        break;
    }
    case 3: {
        const auto dx = actor_.anchor_x - world_x_;
        const auto dy = actor_.anchor_y - world_y_;
        if (dx == 0 && dy == 0) {
            actor_.direction ^= 2;
            actor_.anim_selector = 0;
            actor_.state = 1;
            apply_actor_anim_frame();
        } else {
            if (dx > 0) ++world_x_; else if (dx < 0) --world_x_;
            if (dy > 0) ++world_y_; else if (dy < 0) --world_y_;
            actor_.anim_selector = 2;
            apply_actor_anim_frame();
        }
        break;
    }
    case 4: {
        if (actor_.knockback_timer > 0) {
            --actor_.knockback_timer;
            world_x_ += (actor_.velocity_x_fixed / 0x10000);
            world_y_ += (actor_.velocity_y_fixed / 0x10000);
        }
        if (actor_.timer > 0) --actor_.timer;
        if (actor_.timer == 0) {
            actor_.state = 1;
            actor_.velocity_x_fixed = 0;
            actor_.velocity_y_fixed = 0;
            actor_.anim_selector = 0;
            apply_actor_anim_frame();
        }
        break;
    }
    default: {
        actor_.halted = true;
        break;
    }
    }
}

bool ScreenEntity::apply_damage(std::int16_t damage, std::uint8_t hit_direction) noexcept {
    if (!active_ || !actor_.is_actor || actor_.invuln_timer > 0 || actor_.state == 2 || actor_.state == 4) {
        return false;
    }
    const auto effective = static_cast<std::int16_t>(std::max(1, static_cast<int>(damage - actor_.defense)));
    actor_.hp = static_cast<std::int16_t>(std::max(0, actor_.hp - effective));
    if (actor_.hp > 0) {
        actor_.state = 4;
        actor_.timer = 20; actor_.invuln_timer = 20; actor_.knockback_timer = 6;
        constexpr std::int32_t kSpeed = 0x30000;
        actor_.velocity_x_fixed = (hit_direction == 1) ? kSpeed : ((hit_direction == 3) ? -kSpeed : 0);
        actor_.velocity_y_fixed = (hit_direction == 2) ? kSpeed : ((hit_direction == 0) ? -kSpeed : 0);
    } else {
        actor_.state = 2; actor_.timer = 1; active_ = false; drop_pending_ = true;
        drop_item_id_ = 0x001E; // ROM drop table 0x017F6C (Item 0x1E, qty 1)
        drop_count_ = 1;
    }
    return true;
}

void ScreenEntity::update(std::span<const std::uint8_t> rom,
                          const world::ByteGridView* terrain,
                          std::int32_t player_x,
                          std::int32_t player_y) noexcept {
    if (!rom.empty()) rom_ = rom;
    if (rom_.empty() || is_item_) return;

    if (actor_.is_actor) {
        update_actor_state_machine(terrain, player_x, player_y);
        return;
    }

    if (is_flame()) {
        if (flame_substate_ != 0) {
            ++flame_timer_;
            flame_visible_ = ((flame_timer_ & 1U) == 0);
            if (flame_timer_ >= 15) active_ = false;
            return;
        }
    }

    if (anim_state_.halted) {
        return;
    }

    if (anim_state_.ticks_remaining > 1U) {
        --anim_state_.ticks_remaining;
        return;
    }

    const auto script_addr = anim_state_.script_address;
    if (script_addr == 0U || script_addr + 2U > rom_.size()) {
        anim_state_.halted = true;
        return;
    }

    const auto rel = read_be16(rom_, script_addr);
    if (rel < 2U || rel > 0x100U || script_addr + rel + 4U > rom_.size()) {
        anim_state_.halted = true;
        return;
    }

    const auto seq_base = script_addr + rel;
    std::uint32_t next_cursor = anim_state_.cursor;

    if (anim_state_.loop_pending) {
        next_cursor = 0U;
        ++anim_state_.loop_count;
        anim_state_.loop_pending = false;
    } else {
        next_cursor += 4U;
    }

    const auto rec_addr = seq_base + next_cursor;
    if (rec_addr + 4U > rom_.size()) {
        anim_state_.halted = true;
        return;
    }

    const auto f_idx = read_be16(rom_, rec_addr);
    const auto raw_dur = read_be16(rom_, rec_addr + 2U);
    if (f_idx > 0x100U) {
        anim_state_.halted = true;
        return;
    }

    const auto duration = std::max(1U, static_cast<unsigned>((raw_dur & 0x7FFFU) >> 1U));
    const auto gfx_base = descriptor_.gfx_descriptor_pointer;
    const auto entry_addr = gfx_base + static_cast<std::uint32_t>(f_idx) * 2U;
    if (entry_addr + 2U > rom_.size()) {
        anim_state_.halted = true;
        return;
    }

    const auto rel_frame = read_be16(rom_, entry_addr);
    const auto frame_addr = entry_addr + rel_frame;
    const auto new_pieces = decode_sprite_frame_from_rom(rom_, frame_addr);
    if (new_pieces.empty()) {
        anim_state_.halted = true;
        return;
    }

    anim_state_.cursor = next_cursor;
    anim_state_.current_frame_index = f_idx;
    anim_state_.ticks_remaining = static_cast<std::uint16_t>(duration);
    anim_state_.loop_pending = (raw_dur & 0x8000U) != 0U;
    anim_state_.current_frame_pointer = frame_addr;
    pieces_ = new_pieces;
    tile_base_ = pieces_.front().tile_index;

    if (interaction_state_ == EntityInteractionState::triggered && f_idx >= 2U) {
        interaction_state_ = EntityInteractionState::opened;
        anim_state_.halted = true;
    }
}

void ScreenEntity::trigger_interaction() noexcept {
    if (interaction_state_ == EntityInteractionState::opened) return;
    interaction_state_ = EntityInteractionState::triggered;
    anim_state_.cursor = 0U;
    anim_state_.current_frame_index = 0U;
    anim_state_.ticks_remaining = 4U;
    anim_state_.loop_count = 0U;
    anim_state_.loop_pending = false;
    anim_state_.halted = false;
}

std::optional<EntityHitbox> ScreenEntity::attack_hitbox() const noexcept {
    if (type_id_ == 0x004C) {
        if (!active_ || flame_substate_ != 0) return std::nullopt;
        const auto base_y = world_y_ + world_z_;
        return EntityHitbox{world_x_ - 14, base_y - 8, world_x_ + 14, base_y + 8, world_z_ - 8, world_z_ + 8};
    }
    if (type_id_ != 0x0048 || actor_.state != 2) return std::nullopt;
    const auto base_y = world_y_ + world_z_;
    switch (actor_.direction & 3U) {
    case 0: return EntityHitbox{world_x_ - 8, base_y - 16, world_x_ + 8, base_y - 4, world_z_ - 8, world_z_ + 8};
    case 1: return EntityHitbox{world_x_ + 4, base_y - 6, world_x_ + 16, base_y + 6, world_z_ - 8, world_z_ + 8};
    case 2: return EntityHitbox{world_x_ - 8, base_y + 4, world_x_ + 8, base_y + 16, world_z_ - 8, world_z_ + 8};
    case 3:
    default: return EntityHitbox{world_x_ - 16, base_y - 6, world_x_ - 4, base_y + 6, world_z_ - 8, world_z_ + 8};
    }
}

} // namespace oasis::game::entities
