#include "game/controlled_screen.hpp"

#include "game/render/debug_overlay.hpp"
#include "game/world/terrain_collision.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace oasis::game::screen {
namespace {

constexpr std::uint8_t kFree = 0x02U, kWall = 0x05U;
constexpr std::uint32_t kBackground = 0x00101828U, kFreeCell = 0x001A2A3AU, kWallCell = 0x006D3548U;
constexpr std::uint32_t kGridLine = 0x002A3A4AU, kPlayer = 0x00F2D15CU, kRomPlayerColor = 0xFF30C030U;

void fill_rect(std::span<std::uint32_t> pixels, std::uint32_t color, int x, int y, int width, int height) noexcept {
    const auto max_x = static_cast<int>(render::SoftwareFramebuffer::kWidth), max_y = static_cast<int>(render::SoftwareFramebuffer::kHeight);
    const auto x0 = std::max(0, x), y0 = std::max(0, y), x1 = std::min(max_x, x + width), y1 = std::min(max_y, y + height);
    for (auto row = y0; row < y1; ++row) for (auto col = x0; col < x1; ++col) pixels[static_cast<std::size_t>(row) * max_x + col] = color;
}

std::string hex_str(std::uint32_t value, int width = 0) {
    std::ostringstream ss; ss << std::hex << std::uppercase;
    if (width > 0) ss << std::setw(width) << std::setfill('0');
    ss << value; return ss.str();
}

} // namespace

bool operator==(const ScreenState& l, const ScreenState& r) noexcept {
    const auto& a = l.player, & b = r.player;
    return l.frame_index == r.frame_index && a.x_fixed == b.x_fixed && a.y_fixed == b.y_fixed &&
           a.terrain_state == b.terrain_state && a.movement_state == b.movement_state &&
           a.direction_code == b.direction_code && a.orientation_flags == b.orientation_flags &&
           a.intent_x_fixed == b.intent_x_fixed && a.intent_y_fixed == b.intent_y_fixed &&
           a.accumulated_x_fixed == b.accumulated_x_fixed && a.accumulated_y_fixed == b.accumulated_y_fixed &&
           a.footprint_any_bits == b.footprint_any_bits && a.turn_timer == b.turn_timer &&
           l.movement.vector.direction == r.movement.vector.direction &&
           l.movement.vector.x_fixed == r.movement.vector.x_fixed && l.movement.vector.y_fixed == r.movement.vector.y_fixed &&
           l.movement.moved == r.movement.moved && l.movement.blocked == r.movement.blocked;
}

ControlledScreen::ControlledScreen(std::span<const std::uint8_t> rom)
    : rom_(rom), terrain_(fixture_cells_, kGridWidth, kGridHeight, kGridWidth, 5) {
    if (rom.empty()) {
        movement_config_.footprint_radius = 6; state_.player.x_fixed = 48 * 0x10000;
        state_.player.y_fixed = 48 * 0x10000; state_.player.terrain_state = world::terrain_state_from_code(kFree);
        build_fixture();
    } else { init_rom_backed(rom); }
}

void ControlledScreen::init_rom_backed(std::span<const std::uint8_t> rom) {
    is_rom_backed_ = true; rom_ = rom;
    trigger_screen_transition({1, 0}, false);
    state_.player.x_fixed = 160 * 0x10000; state_.player.y_fixed = 160 * 0x10000;
    state_.player.terrain_state = 0; movement_config_.footprint_radius = 2;
    transition_count_ = 0; previous_screen_setup_id_ = 0; previous_screen_.reset();
    state_.player.secondary_slot = 0; state_.player.secondary_weapon = weapon_inventory_[0].id;
    state_.player.secondary_ammo = weapon_inventory_[0].ammo;
}

bool ControlledScreen::trigger_screen_transition(world::ScreenId target_id, bool explicit_debug) {
    if (!is_rom_backed_ || rom_.empty()) return false;
    auto new_inst = std::make_unique<ScreenInstance>(ScreenInstance::load_from_rom(rom_, target_id));
    previous_screen_setup_id_ = current_screen_ ? current_screen_->setup_id : 0;
    previous_screen_ = std::move(current_screen_); current_screen_ = std::move(new_inst);
    terrain_ = current_screen_->terrain;
    for (auto& entity : current_screen_->entities) {
        if (entity.is_flame() && is_persistent_flag_set(entity.persistent_id())) entity.set_active(false);
    }
    ++transition_count_;
    if (explicit_debug || (target_id.group == 1 && target_id.index == 0)) {
        state_.player.x_fixed = 160 * 0x10000; state_.player.y_fixed = 160 * 0x10000;
    } else if (target_id.group == 1 && target_id.index == 1) {
        state_.player.x_fixed = 160 * 0x10000; state_.player.y_fixed = 156 * 0x10000;
    }
    state_.player.terrain_state = 0;
    const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
    camera_x_ = std::clamp(px - 160, 0, 512 - 320); camera_y_ = std::clamp(py - 112, 0, 512 - 224);
    overlap_detected_ = false; interaction_target_type_ = 0; interaction_state_str_ = "IDLE";
    state_.player.contact_entity = 0; state_.player.damage_gate_open = false;
    state_.player.hurt_timer = 0; state_.player.invuln_timer = 0;
    state_.player.knockback_x_fixed = 0; state_.player.knockback_y_fixed = 0;
    state_.player.attack_state = 0; state_.player.attack_timer = 0;
    state_.player.attack_active = false; state_.player.attack_hit_latched = false;
    if (spirit_.active) spirits::reposition_spirit_on_screen_transition(spirit_, px, py, 0);
    arrow_.active = false;
    return true;
}

std::uint16_t ControlledScreen::current_screen_setup_id() const noexcept { return current_screen_ ? current_screen_->setup_id : 0; }
std::uint32_t ControlledScreen::descriptor_address() const noexcept { return current_screen_ ? current_screen_->descriptor_address : 0; }
std::uint32_t ControlledScreen::primary_resource_address() const noexcept { return current_screen_ ? current_screen_->primary_stream_pointer : 0; }
std::array<std::uint8_t, 4> ControlledScreen::gfx_resource_ids() const noexcept { return current_screen_ ? current_screen_->gfx_resource_ids : std::array<std::uint8_t, 4>{}; }

void ControlledScreen::build_fixture() noexcept {
    fixture_cells_.fill(kFree);
    auto set_cell = [this](std::uint16_t x, std::uint16_t y) { fixture_cells_[static_cast<std::size_t>(y) * kGridWidth + x] = kWall; };
    for (std::uint16_t y = 5; y <= 18; ++y) { set_cell(20, y); set_cell(21, y); }
    for (std::uint16_t x = 8; x <= 21; ++x) { set_cell(x, 18); set_cell(x, 19); }
    for (std::uint16_t y = 8; y <= 10; ++y) for (std::uint16_t x = 29; x <= 31; ++x) set_cell(x, y);
}

void ControlledScreen::update(const core::FrameContext& frame) {
    const bool mode_pressed = frame.input.port1.pressed(core::Button::Mode);
    if (mode_pressed && !mode_button_held_) toggle_debug_overlay();
    mode_button_held_ = mode_pressed;

    const bool x_pressed = frame.input.port1.pressed(core::Button::X);
    if (x_pressed && !button_x_held_ && is_rom_backed_ && current_screen_) {
        if (current_screen_->setup_id == 0x0100) trigger_screen_transition({1, 1}, true);
        else if (current_screen_->setup_id == 0x0101) trigger_screen_transition({1, 0}, true);
    }
    button_x_held_ = x_pressed;

    const auto prev_x_fixed = state_.player.x_fixed;
    const auto prev_y_fixed = state_.player.y_fixed;

    const bool in_hurt = player::update_player_hurt_knockback(
        state_.player, current_screen_ ? &current_screen_->terrain : nullptr);
    const bool a_pressed = frame.input.port1.pressed(core::Button::A);
    if (!spirit_.active) {
        if (a_pressed && !button_a_held_) {
            const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
            const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
            spirits::try_summon_spirit(spirit_, spirit_slots_, 0, px, py, 0, state_.player, rom_);
        }
    } else {
        const auto cmd = spirits::decode_dytto_command_input(spirit_, a_pressed);
        if (cmd > 0) spirits::try_execute_dytto_command(spirit_, cmd, state_.player, rom_);
    }
    button_a_held_ = a_pressed;
    spirits::update_active_spirit(spirit_, state_.player, state_.player);
    spirits::update_dytto_ability(spirit_, rom_, current_screen_ ? &current_screen_->entities : nullptr);

    const bool y_pressed = frame.input.port1.pressed(core::Button::Y);
    if (y_pressed && !button_y_held_) player::cycle_secondary_weapon(weapon_inventory_, state_.player);
    button_y_held_ = y_pressed;

    const bool b_pressed = frame.input.port1.pressed(core::Button::B);
    if (b_pressed && !in_hurt && !state_.player.is_attacking()) player::start_player_attack(state_.player);

    const bool c_pressed = frame.input.port1.pressed(core::Button::C);
    if (c_pressed && !button_c_held_ && !in_hurt && !state_.player.is_attacking() && interaction_state_str_ != "READY") {
        player::try_start_secondary_attack(state_.player, weapon_inventory_);
    }
    button_c_held_ = c_pressed;

    const bool in_attack = state_.player.is_attacking();
    state_.frame_index = frame.frame_index;
    if (!in_hurt && !in_attack) {
        state_.movement = player::try_move(state_.player, frame.input.port1, terrain_, movement_config_);
    } else {
        state_.movement = {};
    }

    overlap_detected_ = false;
    interaction_target_type_ = 0;
    state_.player.contact_entity = 0;
    state_.player.damage_gate_open = false;
    if (interaction_state_str_ != "TRIGGERED (OPENING)" && interaction_state_str_ != "OPENED") {
        interaction_state_str_ = "IDLE";
    }

    if (is_rom_backed_ && current_screen_) {
        if (!in_hurt && state_.movement.moved) {
            const auto cand_hb = entities::player_hitbox(static_cast<std::int32_t>(state_.player.x_fixed / 0x10000), static_cast<std::int32_t>(state_.player.y_fixed / 0x10000));
            for (const auto& entity : current_screen_->entities) {
                if (!entity.active() || entity.is_item() || entity.is_flame()) continue;
                if (cand_hb.overlaps(entity.hitbox())) {
                    state_.player.x_fixed = prev_x_fixed; state_.player.y_fixed = prev_y_fixed;
                    state_.movement.moved = false; state_.movement.blocked = true;
                    overlap_detected_ = true; break;
                }
            }
        }

        const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
        const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
        const auto cur_hitbox = entities::player_hitbox(px, py);
        const bool action_pressed = frame.input.port1.pressed(core::Button::A) || frame.input.port1.pressed(core::Button::C);

        for (auto& entity : current_screen_->entities) {
            if (!entity.active()) continue;
            if (cur_hitbox.overlaps(entity.hitbox())) overlap_detected_ = true;

            const auto ent_box = entity.hitbox();
            const entities::EntityHitbox expanded{
                ent_box.left - 8, ent_box.top - 8, ent_box.right + 8, ent_box.bottom + 8,
                ent_box.z_min, ent_box.z_max
            };

            if (cur_hitbox.overlaps(expanded)) {
                const auto dx = entity.world_x() - px, dy = entity.world_y() - py;
                const bool facing = (std::abs(dx) >= std::abs(dy))
                    ? ((dx > 0 && state_.player.direction_code == 1) || (dx < 0 && state_.player.direction_code == 3))
                    : ((dy > 0 && state_.player.direction_code == 2) || (dy < 0 && state_.player.direction_code == 0));

                if (facing) {
                    interaction_target_type_ = entity.type_id();
                    const auto st = entity.interaction_state();
                    if (st == entities::EntityInteractionState::opened) {
                        interaction_state_str_ = "OPENED";
                    } else if (st == entities::EntityInteractionState::triggered) {
                        interaction_state_str_ = "TRIGGERED (OPENING)";
                    } else {
                        entity.set_interaction_state(entities::EntityInteractionState::ready);
                        interaction_state_str_ = action_pressed ? "TRIGGERED (OPENING)" : "READY";
                        if (action_pressed) entity.trigger_interaction();
                    }
                }
            }

            entity.update(rom_, current_screen_ ? &current_screen_->terrain : nullptr, px, py);
            if (entity.interaction_state() == entities::EntityInteractionState::opened &&
                interaction_target_type_ == entity.type_id()) {
                interaction_state_str_ = "OPENED";
            }

            if (const auto atk = entity.attack_hitbox(); atk && cur_hitbox.overlaps(*atk)) {
                player::apply_enemy_contact_damage(state_.player, entity.type_id(),
                    entity.is_actor() ? entity.actor().direction : 2,
                    entity.is_flame() ? 15 : 10);
            }
        }

        if (state_.player.attack_active && !state_.player.attack_hit_latched) {
            if (const auto weapon_box = player::player_attack_hitbox(state_.player)) {
                for (auto& entity : current_screen_->entities) {
                    if (!entity.active() || !entity.is_actor()) continue;
                    if (weapon_box->overlaps(entity.hitbox()) && entity.apply_damage(10, state_.player.attack_direction)) {
                        state_.player.attack_hit_latched = true; state_.player.last_hit_damage = 10;
                        if (entity.drop_pending()) {
                            current_screen_->entities.push_back(entities::ScreenEntity::spawn_drop_from_rom(
                                rom_, entity.drop_item_id(), entity.world_x(), entity.world_y(), entity.world_z(), entity.drop_count()));
                            entity.clear_drop_pending();
                        }
                        break;
                    }
                }
            }
        }
        for (auto& entity : current_screen_->entities) {
            if (entity.active() && entity.is_item() && cur_hitbox.overlaps(entity.hitbox())) {
                player::apply_item_pickup(state_.player, entity.item_id(), entity.item_count());
                entity.collect();
            } else if (entity.is_flame() && !entity.active() && entity.flame_substate() != 0) {
                set_persistent_flag(entity.persistent_id());
            }
        }
        if (state_.player.attack_state != 0) player::update_player_attack(state_.player);
        if (state_.player.is_secondary_attacking()) {
            player::update_secondary_attack(state_.player, weapon_inventory_, arrow_, rom_);
        }
        if (arrow_.active) {
            player::update_arrow_projectile(arrow_, current_screen_ ? &current_screen_->terrain : nullptr,
                                            current_screen_ ? current_screen_->entities : std::span<entities::ScreenEntity>{},
                                            state_.player);
            if (current_screen_) {
                for (auto& entity : current_screen_->entities) {
                    if (entity.drop_pending()) {
                        current_screen_->entities.push_back(entities::ScreenEntity::spawn_drop_from_rom(
                            rom_, entity.drop_item_id(), entity.world_x(), entity.world_y(), entity.world_z(), entity.drop_count()));
                        entity.clear_drop_pending();
                        break;
                    }
                }
            }
        }

        if (current_screen_->setup_id == 0x0100 && state_.player.y_fixed >= 184 * 0x10000 &&
            state_.player.direction_code == 0x01 /* Down */) {
            trigger_screen_transition({1, 1}, false);
        } else if (current_screen_->setup_id == 0x0101 && state_.player.y_fixed <= 154 * 0x10000 &&
                   state_.player.direction_code == 0x00 /* Up */) {
            trigger_screen_transition({1, 0}, false);
        }
    }
}

void ControlledScreen::render(std::span<std::uint32_t> destination) const noexcept {
    constexpr std::int32_t fb_w = 320;
    constexpr std::int32_t fb_h = 224;

    if (is_rom_backed_ && current_screen_ && !current_screen_->tilemap.empty()) {
        std::fill(destination.begin(), destination.end(), 0xFF000000U);
        const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
        const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
        camera_x_ = std::clamp(px - 160, 0, 512 - fb_w);
        camera_y_ = std::clamp(py - 112, 0, 512 - fb_h);

        const auto first_tx = camera_x_ / 8;
        const auto last_tx = std::min(63, (camera_x_ + fb_w + 7) / 8);
        const auto first_ty = camera_y_ / 8;
        const auto last_ty = std::min(27, (camera_y_ + fb_h + 7) / 8);

        for (auto ty = first_ty; ty <= last_ty; ++ty) {
            for (auto tx = first_tx; tx <= last_tx; ++tx) {
                const auto tile_idx_in_map = ty * 64 + tx;
                const auto word = current_screen_->tilemap[static_cast<std::size_t>(tile_idx_in_map)];
                const auto tile_id = static_cast<std::size_t>(word & 0x07FFU);
                const auto pal_id = static_cast<std::size_t>((word >> 13U) & 0x03U);
                const bool hflip = (word & (1U << 11U)) != 0;
                const bool vflip = (word & (1U << 12U)) != 0;

                if (tile_id < current_screen_->tiles.size()) {
                    const auto& tile = current_screen_->tiles[tile_id];
                    const auto& pal = current_screen_->palettes[pal_id];
                    const auto tile_screen_x = tx * 8 - camera_x_;
                    const auto tile_screen_y = ty * 8 - camera_y_;

                    for (int r = 0; r < 8; ++r) {
                        const auto sy = tile_screen_y + r;
                        if (sy < 0 || sy >= fb_h) continue;
                        const int src_r = vflip ? (7 - r) : r;
                        for (int c = 0; c < 8; ++c) {
                            const auto sx = tile_screen_x + c;
                            if (sx < 0 || sx >= fb_w) continue;
                            const int src_c = hflip ? (7 - c) : c;
                            const auto color_idx = tile[src_r * 8 + src_c];
                            if (color_idx != 0) {
                                const auto& rgb = pal[color_idx];
                                destination[static_cast<std::size_t>(sy * fb_w + sx)] =
                                    0xFF000000U |
                                    (static_cast<std::uint32_t>(rgb.r) << 16U) |
                                    (static_cast<std::uint32_t>(rgb.g) << 8U) |
                                    static_cast<std::uint32_t>(rgb.b);
                            }
                        }
                    }
                }
            }
        }
        const auto render_pieces = [&](std::span<const entities::SpritePiece> pieces, std::int32_t wx, std::int32_t wy) {
            for (const auto& piece : pieces) {
                const auto piece_screen_x = wx + piece.x_offset - camera_x_;
                const auto piece_screen_y = wy + piece.y_offset - camera_y_;
                const auto pal_id = static_cast<std::size_t>(piece.palette_line & 0x03U);
                const auto& pal = current_screen_->palettes[pal_id];
                for (std::uint8_t c = 0; c < piece.width_cells; ++c) {
                    for (std::uint8_t r = 0; r < piece.height_cells; ++r) {
                        const auto tile_id = static_cast<std::size_t>(piece.tile_index + c * piece.height_cells + r);
                        if (tile_id >= current_screen_->tiles.size()) continue;
                        const auto& tile = current_screen_->tiles[tile_id];
                        const auto tile_screen_x = piece_screen_x + c * 8;
                        const auto tile_screen_y = piece_screen_y + r * 8;
                        for (int py_i = 0; py_i < 8; ++py_i) {
                            const auto sy = tile_screen_y + py_i;
                            if (sy < 0 || sy >= fb_h) continue;
                            const int src_r = piece.vflip ? (7 - py_i) : py_i;
                            for (int px_i = 0; px_i < 8; ++px_i) {
                                const auto sx = tile_screen_x + px_i;
                                if (sx < 0 || sx >= fb_w) continue;
                                const int src_c = piece.hflip ? (7 - px_i) : px_i;
                                const auto color_idx = tile[src_r * 8 + src_c];
                                if (color_idx != 0) {
                                    const auto& rgb = pal[color_idx];
                                    destination[static_cast<std::size_t>(sy * fb_w + sx)] =
                                        0xFF000000U | (static_cast<std::uint32_t>(rgb.r) << 16U) |
                                        (static_cast<std::uint32_t>(rgb.g) << 8U) | rgb.b;
                                }
                            }
                        }
                    }
                }
            }
        };

        for (const auto& entity : current_screen_->entities) {
            if (entity.active()) {
                if (entity.is_flame() && !entity.flame_visible()) continue;
                render_pieces(entity.pieces(), entity.world_x(), entity.world_y());
            }
        }
        if (spirit_.active) render_pieces(spirit_.pieces, spirit_.world_x, spirit_.world_y + spirit_.world_z);
        if (spirit_.projectile_active) render_pieces(spirit_.projectile_pieces, spirit_.projectile_x, spirit_.projectile_y);
        if (arrow_.active) render_pieces(arrow_.pieces, arrow_.world_x, arrow_.world_y + arrow_.world_z);

        const auto player_screen_x = px - camera_x_, player_screen_y = py - camera_y_;
        for (std::int32_t dy = -3; dy <= 3; ++dy) {
            const auto y = player_screen_y + dy;
            if (y >= 0 && y < fb_h) {
                for (std::int32_t dx = -3; dx <= 3; ++dx) {
                    const auto x = player_screen_x + dx;
                    if (x >= 0 && x < fb_w) destination[static_cast<std::size_t>(y * fb_w + x)] = kRomPlayerColor;
                }
            }
        }
        if (show_debug_overlay_) render_debug_overlay(destination);
        return;
    }

    const auto expected = static_cast<std::size_t>(render::SoftwareFramebuffer::kWidth) *
                          render::SoftwareFramebuffer::kHeight;
    if (destination.size() < expected) return;
    destination = destination.first(expected);
    std::fill(destination.begin(), destination.end(), kBackground);
    for (std::uint16_t y = 0; y < kGridHeight; ++y) {
        for (std::uint16_t x = 0; x < kGridWidth; ++x) {
            fill_rect(destination, fixture_cells_[static_cast<std::size_t>(y) * kGridWidth + x] == kWall ? kWallCell : kFreeCell, x * 8, y * 8, 8, 8);
        }
    }
    for (std::uint16_t y = 0; y <= kGridHeight; ++y) fill_rect(destination, kGridLine, 0, y * 8, render::SoftwareFramebuffer::kWidth, 1);
    for (std::uint16_t x = 0; x <= kGridWidth; ++x) fill_rect(destination, kGridLine, x * 8, 0, 1, render::SoftwareFramebuffer::kHeight);
    fill_rect(destination, kPlayer, (state_.player.x_fixed / 0x10000) - 5, (state_.player.y_fixed / 0x10000) - 5, 11, 11);
}

void ControlledScreen::render_debug_overlay(std::span<std::uint32_t> destination) const noexcept {
    const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
    const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
    const auto raw_val = terrain_.sample_world(px, py).value_or(0xFFU);

    std::string enemy_hp = "NONE", enemy_hurt = "NONE", enemy_st = "NONE";
    std::string drop_t = "NONE", drop_st = "NONE", drop_xy = "NONE", drop_act = "NO", pk_tgt = "NONE";
    std::string env_type = "NONE", env_st = "NONE", env_xy = "NONE", env_react = "NO", wflag = "0";
    if (current_screen_) {
        for (const auto& e : current_screen_->entities) {
            if (e.is_actor()) {
                enemy_hp = std::to_string(e.actor().hp) + "/" + std::to_string(e.actor().max_hp);
                enemy_hurt = (e.actor().is_hurt() ? "YES (" : "NO (") + std::to_string(e.actor().timer) + ")";
                enemy_st = (e.actor().halted ? "HALT" : std::to_string(e.actor().state)) + " D=" + std::to_string(e.actor().direction);
            } else if (e.is_item()) {
                drop_t = "0x" + hex_str(e.item_id(), 4); drop_st = std::to_string(e.actor().state);
                drop_xy = std::to_string(e.world_x()) + "/" + std::to_string(e.world_y());
                drop_act = e.active() ? "YES" : "NO"; if (e.active()) pk_tgt = "0x0008";
            } else if (e.is_flame()) {
                env_type = "0x004C";
                env_st = (e.flame_substate() == 0) ? "BURNING" : (e.active() ? ("EXTINGUISHING(" + std::to_string(e.flame_timer()) + ")") : "EXTINGUISHED");
                env_xy = std::to_string(e.world_x()) + "/" + std::to_string(e.world_y());
                env_react = "YES";
                wflag = is_persistent_flag_set(e.persistent_id()) ? "SET" : "CLR";
            }
        }
    }

    std::string atk_box = "NONE";
    if (const auto ab = player::player_attack_hitbox(state_.player)) {
        atk_box = "L=" + std::to_string(ab->left) + " T=" + std::to_string(ab->top) + " R=" + std::to_string(ab->right) + " B=" + std::to_string(ab->bottom);
    }
    const std::string tgt = (interaction_target_type_ != 0) ? ("0x" + hex_str(interaction_target_type_, 4)) : "NONE";
    const std::string last_pk = (state_.player.last_pickup_type != 0)
        ? ("0x" + hex_str(state_.player.last_pickup_type, 4) + " (+" + std::to_string(state_.player.last_pickup_heal) + ")") : "NONE";
    const std::string b_state = spirit_.projectile_active ? (std::to_string(spirit_.projectile_state) + (spirit_.projectile_state == 16 ? " (FLY)" : " (POP)")) : "NONE";
    const std::string elem_hit = (spirit_.target_entity_hit == 0x004C) ? "WATER" : "NONE";

    const std::array<std::string, 21> lines{{
        "CURRENT SETUP: " + hex_str(current_screen_setup_id(), 4) + " PREV: " + (previous_screen_setup_id_ != 0 ? hex_str(previous_screen_setup_id_, 4) : "NONE"),
        "PLAYER: HP=" + std::to_string(state_.player.hp) + "/" + std::to_string(state_.player.max_hp) + " CURR=" + std::to_string(state_.player.currency) + " ITEMS=" + std::to_string(state_.player.item_count),
        "SPIRIT: ACT=" + std::string(spirit_.active ? "YES" : "NO") + " T=" + (spirit_.active ? ("0x" + hex_str(spirit_.type_id, 4)) : "NONE") + " ST=" + std::to_string(spirit_.state) + " SP=" + std::to_string(state_.player.sp) + "/" + std::to_string(state_.player.max_sp),
        "SPIRIT_CMD: C=" + std::to_string(spirit_.last_command_code) + " ST=" + std::to_string(spirit_.command_state) + " T=" + std::to_string(spirit_.command_timer) + " SP_COST=" + std::to_string(spirit_.last_command_sp_cost),
        "SPIRIT_EFFECT: BUBBLE_STATE=" + b_state + " LAST_ELEMENT_HIT=" + elem_hit + " TGT=" + (spirit_.target_entity_hit != 0 ? ("0x" + hex_str(spirit_.target_entity_hit, 4)) : "NONE"),
        "ENV_TARGET: T=" + env_type + " ST=" + env_st + " XY=" + env_xy + " REACT=" + env_react + " WFLAG=" + wflag,
        "SEC_WEAPON: 0x" + hex_str(state_.player.secondary_weapon, 2) + " AMMO=" + std::to_string(state_.player.secondary_ammo) + " ST=" + std::to_string(state_.player.secondary_attack_state) + " T=" + std::to_string(state_.player.secondary_attack_timer),
        "SEC_PROJ: T=" + (arrow_.active ? ("0x" + hex_str(arrow_.type_id, 4)) : "NONE") + " XY=" + (arrow_.active ? (std::to_string(arrow_.world_x) + "/" + std::to_string(arrow_.world_y)) : "NONE") + " HIT=" + (state_.player.last_weapon_hit != 0 ? ("0x" + hex_str(state_.player.last_weapon_hit, 4)) : "NONE"),
        "DROP: T=" + drop_t + " S=" + drop_st + " XY=" + drop_xy + " ACT=" + drop_act + " PK=" + pk_tgt,
        "LAST_PICKUP: " + last_pk,
        "PLAYER_STATE: " + std::to_string(state_.player.movement_state) + (state_.player.is_hurt() ? " (HURT)" : " (NORM)") + " INVULN=" + std::to_string(state_.player.invuln_timer),
        "PLAYER_ATK: ST=" + std::to_string(state_.player.attack_state) + " T=" + std::to_string(state_.player.attack_timer) + " ACT=" + (state_.player.attack_active ? "YES" : "NO"),
        "PLAYER_ATK_BOX: " + atk_box,
        "ENEMY: HP=" + enemy_hp + " HURT=" + enemy_hurt + " " + enemy_st,
        "LAST_HIT_DAMAGE: " + std::to_string(state_.player.last_hit_damage),
        "KNOCKBACK_X/Y: " + std::to_string(state_.player.knockback_x_fixed / 0x10000) + "/" + std::to_string(state_.player.knockback_y_fixed / 0x10000),
        "CONTACT_ENTITY: " + (state_.player.contact_entity != 0 ? ("0x" + hex_str(state_.player.contact_entity, 4)) : "NONE"),
        "DAMAGE_GATE: " + std::string(state_.player.damage_gate_open ? "OPEN (DAMAGE)" : "CLOSED"),
        "ENTITY COUNT: " + std::to_string(current_screen_ ? current_screen_->entities.size() : 0U) + " TGT: " + tgt,
        "INTERACTION_STATE: " + interaction_state_str_,
        "WATER_REACTIVE: " + env_react + " WORLD_FLAG: " + wflag
    }};

    render::fill_debug_rect(destination, 320, 224, 6, 2, 224, 220, 0xCC101018U);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        render::draw_debug_text(destination, 320, 224, 10, 4 + static_cast<std::int32_t>(i) * 9, lines[i], 0xFF00FFCCU);
    }
}

} // namespace oasis::game::screen
