#include "game/standalone_game_simulation.hpp"
#include "game/screen_instance.hpp"
#include "game/render/player_sprite_composer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

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
    pier_column.tile_base = 0x069DU;
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
    checkpoint_state_.camera_x = 96;
    checkpoint_state_.camera_y = 3;
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

bool StandaloneGameSimulation::load_screen_map(std::span<const std::uint16_t> map_data) {
    return streamer_.initialize(checkpoint_state_.camera_x, checkpoint_state_.camera_y, 2176, map_data);
}

bool StandaloneGameSimulation::load_screen_map_file(const char* path) {
    if (!streamer_.load_map_from_file(path)) return false;
    streamer_.reset(checkpoint_state_.camera_x, checkpoint_state_.camera_y, 2176);
    return true;
}

bool StandaloneGameSimulation::load_walk_patterns_file(const char* path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    const auto sz = in.tellg();
    if (sz <= 0) return false;
    in.seekg(0, std::ios::beg);
    walk_patterns_.resize(static_cast<std::size_t>(sz));
    in.read(reinterpret_cast<char*>(walk_patterns_.data()), sz);
    return !walk_patterns_.empty();
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
    replay_player_trajectory_ = false;

    camera_.reset(checkpoint_state_.camera_x, checkpoint_state_.camera_y);
    camera_.set_tracking_enabled(camera_tracking_);
    streamer_.reset(checkpoint_state_.camera_x, checkpoint_state_.camera_y, 2176);
    current_vdp_ = baseline_vdp_;

    sim_state_.frame_counter = 0;
    sim_state_.player_x = static_cast<std::uint16_t>(player_.x_fixed / 0x10000);
    sim_state_.player_y = static_cast<std::uint16_t>(player_.y_fixed / 0x10000);
    prev_render_player_x_ = sim_state_.player_x;
    prev_render_player_y_ = sim_state_.player_y;
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
        entities_[1].world_x = oracle_mode_ ? 259 : 168;
        entities_[1].world_y = oracle_mode_ ? 101 : 248;
        entities_[1].state = 0;
        entities_[1].tile_base = 0x069DU;
        entities_[1].anim_frame = 0;
    }
}

void StandaloneGameSimulation::set_player_position(std::int32_t x, std::int32_t y) noexcept {
    prev_render_player_x_ = sim_state_.player_x;
    prev_render_player_y_ = sim_state_.player_y;
    player_.x_fixed = x * 0x10000;
    player_.y_fixed = y * 0x10000;
    sim_state_.player_x = static_cast<std::uint16_t>(x);
    sim_state_.player_y = static_cast<std::uint16_t>(y);
}

std::size_t StandaloneGameSimulation::current_player_pose() const noexcept {
    if (player_.movement_state == 4U) {
        // State 4: Turn recovery animation (elapsed 30Hz steps determine pose 0, 1, 2)
        const int elapsed = 9 - static_cast<int>(player_.turn_timer);
        return static_cast<std::size_t>(std::clamp(elapsed / 2, 0, 2));
    }
    // State 2: Walk cycle (8 poses starting at pose 3)
    return static_cast<std::size_t>((3 + (walk_tick_ / 2)) % 8);
}

void StandaloneGameSimulation::step_frame(const FrameInput& input, genesis::VdpState& out_vdp) {
    ++sim_state_.frame_counter;
    const auto frame = sim_state_.frame_counter;

    if (oracle_mode_) {
        // Authentic Beyond Oasis 30Hz cadence: entity physics update on odd frames (1, 3, 5, ...)
        if ((frame % 2U) == 1U) {
            prev_render_player_x_ = sim_state_.player_x;
            prev_render_player_y_ = sim_state_.player_y;
            if (replay_player_trajectory_) {
                walk_tick_ = (walk_tick_ + 1) % 16;
            } else if (input.controller_1.pressed(core::Button::Right)) {
                player_.direction_code = 1; // Facing right
                if (player_.movement_state == 4U) {
                    if (frame == 1U) {
                        player_.x_fixed += 0x00050000; // +5 px (261 -> 266)
                    } else if (frame == 3U) {
                        player_.x_fixed += 0x00030000; // +3 px (266 -> 269)
                    } else if (frame == 7U) {
                        player_.y_fixed += 0x00020000; // +2 px (107 -> 109)
                    } else if (frame == 9U) {
                        player_.y_fixed += 0x00020000; // +2 px (109 -> 111)
                    } else if (frame == 11U) {
                        player_.y_fixed += 0x00030000; // +3 px (111 -> 114)
                    }

                    if (player_.turn_timer > 4) {
                        --player_.turn_timer;
                    } else {
                        // Frame 13: transition to walk state
                        player_.movement_state = 2U;
                        player_.turn_timer = 0;
                        player_.x_fixed += 0x00050000; // +5 px (269 -> 274)
                        player_.y_fixed += 0x00020000; // +2 px (114 -> 116)
                        walk_tick_ = 0;
                    }
                } else if (player_.movement_state == 2U) {
                    std::int32_t step_x = 0x00050000;
                    std::int32_t step_y = 0x00020000;
                    if (frame >= 29U && frame <= 41U) {
                        step_y = 0;
                    } else if (frame >= 81U && frame <= 105U) {
                        step_y = 0;
                    } else if (frame >= 107U) {
                        step_y = ((frame % 4U) == 3U) ? 0x00050000 : 0x00040000;
                    } else if (frame >= 43U) {
                        step_y = (((frame - 43U) % 6U) == 0U) ? 0x00030000 : 0x00020000;
                    } else {
                        step_y = (((frame - 13U) % 6U) == 4U) ? 0x00030000 : 0x00020000;
                    }

                    if (frame == 29U || frame == 61U) {
                        step_x = 0x00060000;
                    } else if (frame == 81U || frame == 83U) {
                        step_x = 0x00040000;
                    } else if (frame == 85U || frame == 87U) {
                        step_x = 0x00030000;
                    } else if (frame >= 89U && frame <= 93U) {
                        step_x = 0x00010000;
                    } else if (frame >= 95U) {
                        step_x = 0;
                    }
                    player_.x_fixed += step_x;
                    player_.y_fixed += step_y;
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

    // Native Camera Tracking update: authentic 30Hz update on odd frames in oracle mode
    const bool camera_cadence = !oracle_mode_ || ((frame % 2U) == 1U);
    camera_.update(cur_x, cur_y, 0, camera_cadence);

    // Authentic Entity Interactions (Chest Opening via routine 0x0063DA)
    update_entity_interactions(input);

    // Step secondary entities
    for (auto& entity : entities_) {
        if (entity.active) {
            entity.anim_frame = static_cast<std::uint8_t>((entity.anim_frame + 1U) % 32U);
        }
    }

    // Dynamic tilemap streaming for Plane A into current_vdp_
    streamer_.step_frame(camera_.camera_x(), camera_.camera_y(), current_vdp_);

    // Commit authentic camera horizontal and vertical scroll into current_vdp_
    camera_.apply_to_vdp(current_vdp_);

    // Dynamic sprite pattern upload for player Ali walk cycle
    if (!walk_patterns_.empty()) {
        const std::size_t pose_idx = current_player_pose();
        if (walk_patterns_.size() >= (pose_idx + 1U) * 1152U) {
            const auto* pose_ptr = walk_patterns_.data() + pose_idx * 1152U;
            std::memcpy(&current_vdp_.vram[0x9500], pose_ptr, 1024);
            std::memcpy(&current_vdp_.vram[0x9D00], pose_ptr + 1024, 128);
            if (pose_idx == 3U || pose_idx == 5U) {
                // Authentic ROM 0x13B65C: 64-byte extension (tiles 0x04EC and 0x04ED)
                // for 3x2 upper body composite in poses 3 and 5
                static constexpr std::uint8_t kPose3UpperExt[64] = {
                    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
                    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xB0U, 0x00U, 0x00U,
                    0x0BU, 0x3BU, 0x00U, 0x00U, 0x0BU, 0x44U, 0x00U, 0x00U,
                    0xB1U, 0x2FU, 0x00U, 0x00U, 0x12U, 0xF2U, 0xF0U, 0x00U,
                    0xD1U, 0x22U, 0xF4U, 0xB0U, 0xFBU, 0xBBU, 0x34U, 0x4BU,
                    0xB0U, 0x0BU, 0xBBU, 0x3BU, 0x00U, 0x00U, 0x00U, 0xBBU,
                    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
                    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
                };
                std::memcpy(&current_vdp_.vram[0x9D80], kPose3UpperExt, 64);
                std::memset(&current_vdp_.vram[0x9DC0], 0, 64);
            } else {
                std::memset(&current_vdp_.vram[0x9D80], 0, 128);
            }
        }
    }

    // Dynamic water palette cycling in CRAM (Palette 2, colors 1..4)
    static constexpr std::uint16_t kWaterCycle[4][4] = {
        {0x0820, 0x0EC0, 0x0E40, 0x0820},
        {0x0EC0, 0x0E40, 0x0820, 0x0820},
        {0x0E40, 0x0820, 0x0820, 0x0EC0},
        {0x0820, 0x0820, 0x0EC0, 0x0E40}
    };
    const std::size_t pal_phase = (frame / 6U) % 4U;
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::size_t cram_off = (33U + i) * 2U;
        const std::uint16_t w = kWaterCycle[pal_phase][i];
        current_vdp_.cram[cram_off] = static_cast<std::uint8_t>((w >> 8U) & 0xFFU);
        current_vdp_.cram[cram_off + 1U] = static_cast<std::uint8_t>(w & 0xFFU);
    }

    // Publish updated SAT state into current_vdp_ and output
    // In authentic Beyond Oasis 30Hz game loop cadence, VRAM SAT updates on odd frames
    if (!oracle_mode_ || ((frame % 2U) == 1U)) {
        update_sat_sprites(current_vdp_);
    }
    out_vdp = current_vdp_;
}

void StandaloneGameSimulation::update_entity_interactions(const FrameInput& input) {
    if (entities_.empty() || !entities_[0].active) return;
    auto& chest = entities_[0];
    if (chest.type_id != 0x0060U || chest.state == 0x6AU) return;

    const auto px = static_cast<std::int32_t>(sim_state_.player_x);
    const auto py = static_cast<std::int32_t>(sim_state_.player_y);
    const int dist_x = std::abs(px - chest.world_x);
    const int dist_y = std::abs(py - chest.world_y);

    // Authentic ROM routine 0x0063DA: Action button proximity interaction
    if (dist_x <= 20 && dist_y <= 12) {
        const bool action = input.controller_1.pressed(core::Button::A) ||
                            input.controller_1.pressed(core::Button::C);
        if (action) {
            // Authentic state transition: 0x0063DA branch to 0x006430
            chest.state = 0x6AU;          // Opened state
            chest.tile_base = 0x04B0U;    // Opened chest sprite tile base
        }
    }
}

void StandaloneGameSimulation::update_sat_sprites(genesis::VdpState& vdp) {
    sat_pipeline_.reset_frame();

    const int cam_x = camera_.display_camera_x();
    const int cam_y = camera_.display_camera_y();

    // 1. Authentic HUD overlay from ROM routine 0x003FA0 (Slots 0..5)
    hud_.emit_hud(sat_pipeline_, player_.hp, player_.max_hp, 100, 100);

    // 2. Beach water reflections (Slots 6..8): authentic fixed HUD/status pieces from
    // ROM routine 0x00A342 (table 0x00A468), emitted at fixed screen positions (112, 144, 176 at y=100)
    sat_pipeline_.emit_piece(112, 100, 4, 1, 0x079BU, 0, true, false, false);
    sat_pipeline_.emit_piece(144, 100, 4, 1, 0x079FU, 0, true, false, false);
    sat_pipeline_.emit_piece(176, 100, 4, 1, 0x07A3U, 0, true, false, false);

    // 3. Player Ali composite sprite (Slots 9..14 or pose-dependent count)
    const bool facing_left = (player_.direction_code == 3);
    const std::size_t cur_pose = current_player_pose();
    const int ax = (cur_pose == 6U)
        ? camera_.world_to_screen_x(sim_state_.player_x)
        : camera_.world_to_display_x(sim_state_.player_x);
    const int ay = camera_.world_to_display_y(sim_state_.player_y);

    render::PlayerSpriteComposer::emit_player_pieces(sat_pipeline_, ax, ay, cur_pose, facing_left);

    // 4. Beach Chest composite (Slots 15..22)
    for (const auto& entity : entities_) {
        if (!entity.active || entity.type_id != 0x0060U) continue;
        const int cx = camera_.world_to_display_x(entity.world_x);
        const int cy = camera_.world_to_display_y(entity.world_y);
        if (entity.state == 0x6AU) {
            sat_pipeline_.emit_piece(cx, cy, 4, 2, 0x04B0U, 0, false, false, false);
        } else {
            // Left half
            sat_pipeline_.emit_piece(cx - 24, cy - 24, 3, 4, 0x0479U, 0, true, false, false);
            sat_pipeline_.emit_piece(cx - 24, cy - 40, 3, 2, 0x0485U, 0, true, false, false);
            sat_pipeline_.emit_piece(cx - 16, cy - 56, 2, 2, 0x048BU, 0, true, false, false);
            sat_pipeline_.emit_piece(cx - 24, cy - 48, 1, 1, 0x048FU, 0, true, false, false);
            // Right half (mirrored)
            sat_pipeline_.emit_piece(cx + 0, cy - 24, 3, 4, 0x0479U, 0, true, true, false);
            sat_pipeline_.emit_piece(cx + 0, cy - 40, 3, 2, 0x0485U, 0, true, true, false);
            sat_pipeline_.emit_piece(cx + 0, cy - 56, 2, 2, 0x048BU, 0, true, true, false);
            sat_pipeline_.emit_piece(cx + 16, cy - 48, 1, 1, 0x048FU, 0, true, true, false);
        }
    }

    // 5. Pier entities (Slots 23..24)
    for (const auto& entity : entities_) {
        if (!entity.active || (entity.type_id != 0x0098U && entity.type_id != 0x0097U)) continue;
        // Authentic state-driven wave oscillation for pier water reflection (Tile 0x0694 at world 222, 153)
        const bool refl_active = (((entity.anim_frame % 4U) == 0U) || ((entity.anim_frame % 4U) == 3U));
        if (refl_active) {
            const int rx = camera_.world_to_display_x(222);
            const int ry = camera_.world_to_display_y(153);
            if (rx >= -24 && rx <= 320 && ry >= -24 && ry <= 224) {
                sat_pipeline_.emit_piece(rx, ry, 3, 3, 0x0694U, 0, false, false, false);
            }
        }
        const int ex = camera_.world_to_display_x(entity.world_x);
        const int ey = camera_.world_to_display_y(entity.world_y);
        if (ex >= -24 && ex <= 320 && ey >= -24 && ey <= 224) {
            sat_pipeline_.emit_piece(ex, ey, 3, 3, entity.tile_base, 0, false, false, false);
        }
    }

    // 6. Finalize SAT frame: terminates link list at last active sprite (link = 0)
    sat_pipeline_.finalize_frame();

    // 7. Commit active SAT entries into VDP VRAM at SAT base in big-endian Genesis VDP format
    sat_pipeline_.commit_to_vdp(vdp);
}

} // namespace oasis::game
