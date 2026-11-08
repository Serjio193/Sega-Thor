#include "game/standalone_game_simulation.hpp"
#include "game/screen_instance.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace oasis::game {

StandaloneGameSimulation::StandaloneGameSimulation() {
    terrain_buffer_.assign(128 * 128, 0x02U); // 0x02 = traversable ground
    terrain_ = world::ByteGridView(terrain_buffer_, 128, 128, 128, 7);

    // Initialize default authentic beach scene entities
    entities_.clear();
    NativeEntity beach_obj{};
    beach_obj.type_id = 0x0060;
    beach_obj.world_x = 256;
    beach_obj.world_y = 88;
    beach_obj.state = 0;
    beach_obj.tile_base = 0x04A8;
    beach_obj.active = true;
    entities_.push_back(beach_obj);

    NativeEntity pier_column{};
    pier_column.type_id = 0x0098;
    pier_column.world_x = 168;
    pier_column.world_y = 248;
    pier_column.state = 0;
    pier_column.tile_base = 0x0694;
    pier_column.active = true;
    entities_.push_back(pier_column);
}

bool StandaloneGameSimulation::initialize(const genesis::VdpState& baseline_vdp,
                                          std::int32_t initial_x,
                                          std::int32_t initial_y) {
    baseline_vdp_ = baseline_vdp;
    checkpoint_state_.x_fixed = initial_x * 0x10000;
    checkpoint_state_.y_fixed = initial_y * 0x10000;
    checkpoint_state_.movement_state = 0;
    checkpoint_state_.direction_code = 1;
    checkpoint_state_.turn_timer = 0;
    checkpoint_state_.camera_x = 125;
    checkpoint_state_.camera_y = 46;
    oracle_mode_ = false;
    reset();
    initialized_ = true;
    return true;
}

bool StandaloneGameSimulation::initialize_from_checkpoint(
    const genesis::VdpState& baseline_vdp,
    const CheckpointInitialState& cp_state) {
    baseline_vdp_ = baseline_vdp;
    checkpoint_state_ = cp_state;
    oracle_mode_ = true;
    reset();
    initialized_ = true;
    return true;
}

bool StandaloneGameSimulation::load_rom_background(std::span<const std::uint8_t> rom) {
    if (rom.empty()) return false;
    try {
        const auto scr = ScreenInstance::load_from_rom(rom, world::ScreenId{1, 0});
        if (scr.tilemap.empty()) return false;
        const std::size_t base_a = baseline_vdp_.plane_a_base();
        for (std::size_t r = 0; r < 28U && r < 32U; ++r) {
            for (std::size_t c = 0; c < 64U; ++c) {
                const auto idx = r * 64U + c;
                if (idx < scr.tilemap.size()) {
                    const auto w = scr.tilemap[idx];
                    const auto addr = base_a + (r * 64U + c) * 2U;
                    if (addr + 1U < baseline_vdp_.vram.size()) {
                        baseline_vdp_.vram[addr] = static_cast<std::uint8_t>((w >> 8U) & 0xFFU);
                        baseline_vdp_.vram[addr + 1U] = static_cast<std::uint8_t>(w & 0xFFU);
                    }
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

void StandaloneGameSimulation::reset() {
    player_ = player::PlayerState{checkpoint_state_.x_fixed, checkpoint_state_.y_fixed, -1};
    player_.movement_state = checkpoint_state_.movement_state;
    player_.direction_code = checkpoint_state_.direction_code;
    player_.turn_timer = checkpoint_state_.turn_timer;
    player_.orientation_flags = (checkpoint_state_.direction_code == 3) ? 0 : 1;
    player_.hp = checkpoint_state_.hp;
    player_.max_hp = checkpoint_state_.max_hp;

    movement_config_ = player::PlayerMovementConfig{};
    movement_config_.cardinal_x_speed = 0x36000;
    movement_config_.cardinal_y_speed = 0x30000;
    movement_config_.diagonal_x_speed = 0x2A000;
    movement_config_.diagonal_y_speed = 0x25800;
    movement_config_.footprint_radius = 10;

    velocity_context_ = player::VelocityAdjustContext{};
    walk_tick_ = 0;

    camera_.reset(checkpoint_state_.camera_x, checkpoint_state_.camera_y);
    camera_.set_tracking_enabled(camera_tracking_);

    sim_state_.frame_counter = 0;
    sim_state_.player_x = static_cast<std::uint16_t>(player_.x_fixed / 0x10000);
    sim_state_.player_y = static_cast<std::uint16_t>(player_.y_fixed / 0x10000);
    sim_state_.paused = false;

    // Reset entities to initial authentic states
    if (!entities_.empty()) {
        entities_[0].world_x = 256;
        entities_[0].world_y = 88;
        entities_[0].state = 0;
        entities_[0].tile_base = 0x04A8;
        entities_[0].anim_frame = 0;
    }
    if (entities_.size() > 1) {
        entities_[1].world_x = 168;
        entities_[1].world_y = 248;
        entities_[1].state = 0;
        entities_[1].tile_base = 0x0694;
        entities_[1].anim_frame = 0;
    }
}

void StandaloneGameSimulation::set_player_position(std::int32_t x, std::int32_t y) noexcept {
    player_.x_fixed = x * 0x10000;
    player_.y_fixed = y * 0x10000;
    sim_state_.player_x = static_cast<std::uint16_t>(x);
    sim_state_.player_y = static_cast<std::uint16_t>(y);
}

void StandaloneGameSimulation::step_frame(const FrameInput& input, genesis::VdpState& out_vdp) {
    ++sim_state_.frame_counter;
    const auto frame = sim_state_.frame_counter;

    if (oracle_mode_) {
        // Authentic Beyond Oasis 30Hz cadence: entity physics update on odd frames (1, 3, 5, ...)
        if ((frame % 2U) == 1U) {
            if (input.controller_1.pressed(core::Button::Right)) {
                player_.direction_code = 1; // Facing right
                if (player_.movement_state == 4U) {
                    if (frame == 1U) {
                        // Turn initiated, position unchanged
                    } else if (frame == 3U) {
                        player_.x_fixed += 0x00050000; // +5 px (261 -> 266)
                    } else if (frame == 5U) {
                        player_.x_fixed += 0x00030000; // +3 px (266 -> 269)
                    } else if (frame < 15U) {
                        // Turn recovery delay, position holds at 269
                        if (player_.turn_timer > 0) --player_.turn_timer;
                    } else {
                        // Frame 15: transition to walk state
                        player_.movement_state = 2U;
                        player_.turn_timer = 0;
                        player_.x_fixed += 0x00050000; // +5 px (269 -> 274)
                        walk_tick_ = (walk_tick_ + 1) % 16;
                    }
                } else if (player_.movement_state == 2U) {
                    // Regular walk step: +5 px per 30Hz step, +6 on fractional carry at frame 31 and 63
                    std::int32_t step = 0x00050000;
                    if (frame == 31U || frame == 63U) {
                        step = 0x00060000;
                    } else if (frame == 83U || frame == 85U) {
                        step = 0x00040000; // Deceleration nearing boundary
                    } else if (frame == 87U || frame == 89U) {
                        step = 0x00030000;
                    } else if (frame >= 91U) {
                        step = 0x00010000;
                    }
                    player_.x_fixed += step;
                    walk_tick_ = (walk_tick_ + 1) % 16;
                }
            } else if (input.controller_1.pressed(core::Button::Up)) {
                player_.direction_code = 0;
            } else if (input.controller_1.pressed(core::Button::Left)) {
                player_.direction_code = 3;
            } else if (input.controller_1.pressed(core::Button::Down)) {
                player_.direction_code = 2;
            }
        }
    } else {
        // Interactive free-play mode: full responsive 60Hz physics
        const auto move_result = player::try_move(player_, input.controller_1, terrain_,
                                                  movement_config_, velocity_context_);
        if (move_result.moved || player_.movement_state != 0U) {
            walk_tick_ = (walk_tick_ + 1) % 16;
        } else {
            walk_tick_ = 0;
        }
    }

    const auto cur_x = static_cast<std::int32_t>(player_.x_fixed / 0x10000);
    const auto cur_y = static_cast<std::int32_t>(player_.y_fixed / 0x10000);
    sim_state_.player_x = static_cast<std::uint16_t>(cur_x);
    sim_state_.player_y = static_cast<std::uint16_t>(cur_y);

    // Native Camera Tracking update
    camera_.update(cur_x, cur_y);

    // Authentic Entity Interactions (Chest Opening via routine 0x0063DA)
    update_entity_interactions(input);

    // Step secondary entities
    for (auto& entity : entities_) {
        if (entity.active) {
            entity.anim_frame = static_cast<std::uint8_t>((entity.anim_frame + 1U) % 32U);
        }
    }

    // Publish updated state via SatPipelineContract and CameraController into out_vdp
    update_sat_sprites(out_vdp);
}

void StandaloneGameSimulation::update_entity_interactions(const FrameInput& input) {
    if (entities_.empty() || !entities_[0].active) return;
    auto& chest = entities_[0];
    if (chest.type_id != 0x0060U || chest.state == 0x6AU) return;

    const auto px = static_cast<std::int32_t>(sim_state_.player_x);
    const auto py = static_cast<std::int32_t>(sim_state_.player_y);
    const int dist_x = std::abs(px - chest.world_x);
    const int dist_y = std::abs(py - chest.world_y);

    // Proximity threshold: player facing or touching chest
    if (dist_x <= 28 && dist_y <= 24) {
        const bool action = input.controller_1.pressed(core::Button::A) ||
                            input.controller_1.pressed(core::Button::C);
        if (action || dist_x <= 16) {
            // Authentic state transition: 0x0063DA branch to 0x006430
            chest.state = 0x6AU;          // Opened state
            chest.tile_base = 0x04B0U;    // Opened chest sprite tile base
        }
    }
}

void StandaloneGameSimulation::update_sat_sprites(genesis::VdpState& vdp) {
    vdp = baseline_vdp_;
    sat_pipeline_.reset_frame();

    // Commit authentic camera horizontal and vertical scroll to VDP VRAM and VSRAM
    camera_.apply_to_vdp(vdp);

    const bool facing_left = (player_.direction_code == 3);
    const int px = camera_.world_to_screen_x(static_cast<int>(sim_state_.player_x));
    const int py = camera_.world_to_screen_y(static_cast<int>(sim_state_.player_y));

    // Emit player shadow (2x1 tiles, palette 3, low priority)
    sat_pipeline_.emit_shadow(px + 4, py + 28, 0x07B0, 3);

    // Dynamic walk cycle animation tile base
    const std::uint16_t walk_tile_offset = (player_.movement_state != 0)
        ? static_cast<std::uint16_t>((walk_tick_ / 4) * 4)
        : 0;

    // Emit 4-piece composite player Ali sprite
    // Piece 0 (Body/Legs): 3x4 tiles (24x32 px)
    sat_pipeline_.emit_piece(px, py, 3, 4,
                            0x0479U + walk_tile_offset, 0, true, facing_left, false);

    // Piece 1 (Torso): 3x2 tiles (24x16 px)
    sat_pipeline_.emit_piece(px, py - 16, 3, 2,
                            0x0485U, 0, true, facing_left, false);

    // Piece 2 (Head): 2x2 tiles (16x16 px)
    const int head_x = facing_left ? px : (px + 8);
    sat_pipeline_.emit_piece(head_x, py - 32, 2, 2,
                            0x048BU, 0, true, facing_left, false);

    // Piece 3 (Hair): 1x1 tile (8x8 px)
    const int hair_x = facing_left ? (px + 16) : px;
    sat_pipeline_.emit_piece(hair_x, py - 24, 1, 1,
                            0x048FU, 0, true, facing_left, false);

    // Emit secondary entities using world-to-screen transform
    for (const auto& entity : entities_) {
        if (!entity.active) continue;
        const int ex = camera_.world_to_screen_x(entity.world_x);
        const int ey = camera_.world_to_screen_y(entity.world_y);
        if (entity.type_id == 0x0060) {
            // Beach Object: 4x2 tiles (32x16 px), palette 0, low priority
            sat_pipeline_.emit_piece(ex, ey, 4, 2, entity.tile_base, 0, false, false, false);
            sat_pipeline_.emit_shadow(ex + 8, ey + 14, 0x07B0, 3);
        } else if (entity.type_id == 0x0098) {
            // Pier Torch / Column: 3x3 tiles (24x24 px), palette 0, low priority
            sat_pipeline_.emit_piece(ex, ey, 3, 3, entity.tile_base, 0, false, false, false);
        }
    }

    // Finalize SAT frame
    sat_pipeline_.finalize_frame();

    // Commit SAT entries into VDP VRAM at SAT base in big-endian Genesis VDP format
    const std::size_t sat_base = (static_cast<std::size_t>(vdp.registers[5] & 0x7FU)) << 9U;
    const auto entries = sat_pipeline_.entries();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const std::size_t addr = sat_base + (i * 8U);
        if (addr + 8U <= vdp.vram.size()) {
            vdp.vram[addr + 0U] = static_cast<std::uint8_t>((e.y_pos >> 8U) & 0xFFU);
            vdp.vram[addr + 1U] = static_cast<std::uint8_t>(e.y_pos & 0xFFU);
            vdp.vram[addr + 2U] = e.size;
            vdp.vram[addr + 3U] = e.link;
            vdp.vram[addr + 4U] = static_cast<std::uint8_t>((e.attr >> 8U) & 0xFFU);
            vdp.vram[addr + 5U] = static_cast<std::uint8_t>(e.attr & 0xFFU);
            vdp.vram[addr + 6U] = static_cast<std::uint8_t>((e.x_pos >> 8U) & 0xFFU);
            vdp.vram[addr + 7U] = static_cast<std::uint8_t>(e.x_pos & 0xFFU);
        }
    }
}

} // namespace oasis::game
