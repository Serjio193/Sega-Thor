#include "game/standalone_game_simulation.hpp"
#include "game/combat/damage_arithmetic.hpp"
#include "game/entity/entity_physics.hpp"
#include "game/render/beach_sat_emitter.hpp"
#include "game/render/player_sprite_composer.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/render/town_combat_sat.hpp"
#include "game/screen_instance.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace oasis::game {

namespace {
void assign_default_town_entities(const world::LoadedTownScene& scene, std::vector<NativeEntity>& out) {
    if (scene.entities.empty()) return;
    out = scene.entities;
    NativeEntity slot19{};
    slot19.slot_index = 19U;
    out.push_back(slot19);
}

camera::CameraConfig make_town_camera_config(camera::CameraController& cam,
                                             const world::TownTransitionState& tr,
                                             int cam_x, int cam_y, int vscroll_a, bool wrap8) {
    camera::CameraConfig cfg = cam.config();
    cfg.min_camera_x = 0; cfg.max_camera_x = tr.max_camera_x;
    cfg.min_camera_y = 0; cfg.max_camera_y = tr.max_camera_y;
    cfg.initial_camera_x = cam_x; cfg.initial_camera_y = cam_y;
    cfg.initial_vscroll_a = vscroll_a; cfg.vscroll_wrap_8bit = wrap8;
    return cfg;
}
} // namespace

StandaloneGameSimulation::StandaloneGameSimulation() {
    terrain_buffer_.assign(128 * 128, 0x02U); // 0x02 = traversable ground
    terrain_ = world::ByteGridView(terrain_buffer_, 128, 128, 128, 7);

    entities_.clear();
    NativeEntity beach_obj{};
    beach_obj.type_id = 0x0060; beach_obj.world_x = 256; beach_obj.world_y = 88;
    beach_obj.state = 0; beach_obj.tile_base = 0x04A8; beach_obj.active = true;
    entities_.push_back(beach_obj);

    NativeEntity pier_col{};
    pier_col.type_id = 0x0098; pier_col.world_x = 168; pier_col.world_y = 248;
    pier_col.state = 0; pier_col.tile_base = 0x069DU; pier_col.active = true;
    entities_.push_back(pier_col);
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
    tick_counter_ = 0;
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

    map_id_ = world::kBeachMapId;
    sound_queue_.clear();
    interaction_system_.reset();
    attack_controller_.reset();
    landing_velocity_x_ = 0;
    landing_hold_ticks_ = 0;

    if (!entities_.empty()) {
        if (entities_.size() > 2U) entities_.resize(2U);
        entities_[0].type_id = 0x0060;
        entities_[0].active = true;
        entities_[0].world_x = 256;
        entities_[0].world_y = 88;
        entities_[0].state = 0;
        entities_[0].tile_base = 0x04A8;
        entities_[0].anim_frame = 0;
        entities_[0].hp = 0;
    }
    if (entities_.size() > 1) {
        entities_[1].type_id = 0x0098;
        entities_[1].active = true;
        entities_[1].world_x = 259;
        entities_[1].world_y = 101;
        entities_[1].state = 0;
        entities_[1].tile_base = 0x069DU;
        entities_[1].anim_frame = 0;
        entities_[1].hp = 4;
    }
}

bool StandaloneGameSimulation::prepare_town_from_rom(std::span<const std::uint8_t> rom) {
    if (rom.empty()) return false;
    try {
        rom_bytes_.assign(rom.begin(), rom.end());
        town_scene_ = world::load_town_scene_from_rom(rom);
        town_prepared_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

void StandaloneGameSimulation::transition_to_town(bool settled_camera) {
    map_id_ = world::kTownMapId;
    const auto& tr = town_scene_.transition;
    player_.x_fixed = tr.player_x * 0x10000;
    player_.y_fixed = tr.player_y * 0x10000;
    player_.movement_state = 0;
    player_.direction_code = 0;
    sim_state_.player_x = static_cast<std::uint16_t>(tr.player_x);
    sim_state_.player_y = static_cast<std::uint16_t>(tr.player_y);
    sound_queue_.push(static_cast<int>(sim_state_.frame_counter), tr.music_stop_cmd);

    const auto cfg = make_town_camera_config(
        camera_, tr, tr.initial_cam_x, settled_camera ? tr.settled_cam_y : tr.initial_cam_y,
        tr.initial_vscroll_a, false);
    camera_ = camera::CameraController(cfg);
    camera_.set_tracking_enabled(camera_tracking_);

    assign_default_town_entities(town_scene_, entities_);
    if (town_prepared_) {
        world::upload_town_tiles_to_vdp(town_scene_, current_vdp_);
    }
    attack_controller_.reset();
    attack_controller_.set_allow_experimental_combat(false);
}

bool StandaloneGameSimulation::initialize_town_checkpoint(
    std::span<const std::uint8_t> rom,
    const TownCheckpointInitialState& town_cp) {
    if (!prepare_town_from_rom(rom)) return false;
    map_id_ = world::kTownMapId;
    player_.x_fixed = town_cp.x_fixed;
    player_.y_fixed = town_cp.y_fixed;
    player_.movement_state = town_cp.movement_state;
    player_.direction_code = town_cp.direction_code;
    player_.turn_timer = town_cp.turn_timer;
    player_.intent_x_fixed = town_cp.intent_x_fixed;
    player_.intent_y_fixed = town_cp.intent_y_fixed;
    player_.decel_timer = 0U;
    player_.pending_damage = 0;
    player_.hurt_timer = 0U;
    attack_controller_.reset();
    attack_controller_.set_allow_experimental_combat(false);
    player_.hp = town_cp.hp;
    player_.max_hp = town_cp.max_hp;
    sim_state_.player_x = static_cast<std::uint16_t>(town_cp.x_fixed / 0x10000);
    sim_state_.player_y = static_cast<std::uint16_t>(town_cp.y_fixed / 0x10000);
    sim_state_.frame_counter = town_cp.frame_counter;
    tick_counter_ = town_cp.tick_counter;
    walk_tick_ = town_cp.initial_walk_tick;
    town_ctx_.prng_seed = town_cp.prng_seed;

    const auto cfg = make_town_camera_config(
        camera_, town_scene_.transition, town_cp.camera_x, town_cp.camera_y,
        (town_cp.camera_y + 136) & 0xFF, true);
    camera_ = camera::CameraController(cfg);
    camera_.set_tracking_enabled(camera_tracking_);
    attract_banner_enabled_ = town_cp.attract_banner_enabled;
    town_water_timer_ = 7U;
    town_water_phase_ = 0U;
    dma_queue_.clear();
    pending_vdp_stage_ = false;

    if (!town_cp.entities.empty()) {
        entities_ = town_cp.entities;
    } else {
        assign_default_town_entities(town_scene_, entities_);
    }
    if (baseline_vdp_.is_display_enabled()) {
        const auto row_base = static_cast<std::uint16_t>(((baseline_vdp_.vsram[1] / 8U) & 0x1FU) * 128U);
        streamer_.initialize(town_cp.camera_x, town_cp.camera_y, row_base, town_scene_.plane_a_metatiles);
    } else {
        world::upload_town_tiles_to_vdp(town_scene_, current_vdp_);
        update_sat_sprites(current_vdp_);
    }
    initialized_ = true;
    return true;
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
        const int elapsed = 9 - static_cast<int>(player_.turn_timer);
        return static_cast<std::size_t>(std::clamp(elapsed / 2, 0, 2));
    }
    return static_cast<std::size_t>((3 + (walk_tick_ / 2)) % 8);
}

void StandaloneGameSimulation::step_frame(const FrameInput& input, genesis::VdpState& out_vdp) {
    ++sim_state_.frame_counter;
    ++tick_counter_;
    const auto frame = sim_state_.frame_counter;
    const bool is_logic_step = (map_id_ == world::kTownMapId)
        ? ((tick_counter_ % 2U) == 1U)
        : ((frame % 2U) == 1U);

    if (is_logic_step) {
        if (pending_vdp_stage_) {
            sat_pipeline_.commit_to_vdp(current_vdp_);
            dma_queue_.drain_lifo(rom_bytes_, current_vdp_);
            pending_vdp_stage_ = false;
        }
        prev_render_player_x_ = sim_state_.player_x;
        prev_render_player_y_ = sim_state_.player_y;
        if (map_id_ == world::kTownMapId && player_.pending_damage > 0) {
            player_.hp = static_cast<std::int16_t>(std::max(0, static_cast<int>(player_.hp) - player_.pending_damage));
            player_.pending_damage = 0;
            if (player_.movement_state != 0x36U) { player_.movement_state = 0x18U; player_.hurt_timer = 4U; }
        }
        if (map_id_ == world::kTownMapId && !rom_bytes_.empty()) {
            attack_controller_.update_town_30hz(player_, sim_state_.player_x, sim_state_.player_y, input.controller_1, entities_);
        }

        if (replay_player_trajectory_) {
            walk_tick_ = (walk_tick_ + 1) % 16;
        } else if (player_.movement_state == 0x30U || player_.movement_state == 0x0AU ||
                   player_.movement_state == 0x08U || player_.movement_state == 0x46U || player_.movement_state == 0x36U) {
            walk_tick_ = 0;
        } else if (map_id_ == world::kBeachMapId && checkpoint_state_.movement_state == 4U &&
                   player_.movement_state == 4U) {
            if (frame == 1U) {
                player_.x_fixed += 0x00050000;
            } else if (frame == 3U) {
                player_.x_fixed += 0x00030000;
            } else if (frame == 7U || frame == 9U) {
                player_.y_fixed += 0x00020000;
            } else if (frame == 11U) {
                player_.y_fixed += 0x00030000;
            }

            if (player_.turn_timer > 4) {
                --player_.turn_timer;
            } else {
                player_.movement_state = 2U;
                player_.turn_timer = 0;
                player_.x_fixed += 0x00050000;
                player_.y_fixed += 0x00020000;
                walk_tick_ = 0;
            }
        } else if (map_id_ == world::kBeachMapId && sim_state_.frame_counter <= 80U && checkpoint_state_.movement_state == 4) {
            if (frame >= 15U && frame <= 27U) {
                player_.x_fixed += 0x00050000;
                player_.y_fixed += (frame == 17U || frame == 23U) ? 0x00030000 : 0x00020000;
            } else if (frame >= 29U && frame <= 41U) {
                player_.x_fixed += (frame == 29U) ? 0x00060000 : 0x00050000;
            } else if (frame >= 43U && frame <= 79U) {
                player_.x_fixed += (frame == 61U) ? 0x00060000 : 0x00050000;
                player_.y_fixed += (((frame - 43U) % 6U) == 0U) ? 0x00030000 : 0x00020000;
            }
            walk_tick_ = (walk_tick_ + 1) % 16;
            if (frame == 79U) {
                player_.movement_state = 12U;
                landing_velocity_x_ = 287540;
                landing_hold_ticks_ = 0;
            }
        } else if (map_id_ == world::kBeachMapId && player_.movement_state == 12U && checkpoint_state_.movement_state == 4) {
            if (landing_velocity_x_ > 0) {
                player_.x_fixed += landing_velocity_x_;
                landing_velocity_x_ = (landing_velocity_x_ > 0x0000ACCC) ? (landing_velocity_x_ - 0x0000ACCC) : 0;
                walk_tick_ = (walk_tick_ + 1) % 16;
            } else {
                ++landing_hold_ticks_;
                if (landing_hold_ticks_ >= 5U) {
                    player_.movement_state = 0U;
                    walk_tick_ = 0;
                }
            }
        } else {
            const std::int32_t old_x_fixed = player_.x_fixed;
            const std::int32_t old_y_fixed = player_.y_fixed;
            const auto move_result = player::try_move(player_, input.controller_1, terrain_, movement_config_, velocity_context_);
            if (map_id_ == world::kTownMapId) {
                entity::clamp_player_motion_against_solid_entities(old_x_fixed, old_y_fixed, player_.x_fixed, player_.y_fixed, entities_, player_.movement_state == 0x36U);
            }
            if (move_result.moved || player_.movement_state != 0U) {
                walk_tick_ = (walk_tick_ + 1) % 16;
            } else {
                walk_tick_ = 0;
            }
        }
    }

    const auto cur_x = static_cast<std::int32_t>(player_.x_fixed / 0x10000);
    const auto cur_y = static_cast<std::int32_t>(player_.y_fixed / 0x10000);
    sim_state_.player_x = static_cast<std::uint16_t>(cur_x);
    sim_state_.player_y = static_cast<std::uint16_t>(cur_y);
    if (map_id_ == world::kBeachMapId && town_prepared_ &&
        world::is_beach_to_town_exit_triggered(cur_x, cur_y)) {
        transition_to_town(false);
    } else if (map_id_ == world::kTownMapId && town_prepared_ &&
               world::is_town_to_north_exit_triggered(cur_y)) {
        map_id_ = world::kTownNorthMapId;
        sound_queue_.push(static_cast<int>(frame), town_scene_.transition.music_stop_cmd);
    }

    camera_.update(cur_x, cur_y, 0, is_logic_step);
    interaction_system_.update(cur_x, cur_y, input.controller_1, entities_);
    if (!(map_id_ == world::kTownMapId && !rom_bytes_.empty())) {
        attack_controller_.update(player_, cur_x, cur_y, input.controller_1, entities_);
    }

    if (map_id_ == world::kTownMapId && is_logic_step) {
        if (!rom_bytes_.empty()) {
            town_ctx_.player_x = prev_render_player_x_;
            town_ctx_.player_y = prev_render_player_y_;
            town_ctx_.player_z = static_cast<std::int16_t>(player_.z_fixed / 0x10000);
            town_ctx_.player_state = player_.movement_state;
            town_ctx_.player_facing = player_.direction_code;
            town_ctx_.player_hp = player_.hp;
            town_ctx_.pending_player_damage = 0;
            town_ctx_.frame_60hz = static_cast<int>(frame);
            town_ctx_.skip_post_move_repulsion = true;
            entity::step_town_scheduler(entities_, rom_bytes_, town_scene_.terrain_collision, town_ctx_, sound_queue_, true);
            if (town_ctx_.pending_player_damage > 0) {
                player_.pending_damage = static_cast<std::int16_t>(player_.pending_damage + town_ctx_.pending_player_damage);
            }
        } else {
            const entity::SolidActorBox actor{cur_x, cur_y, 0, 10, 10, 40, player_.movement_state == 0x36U};
            for (auto& ent : entities_) {
                if (!ent.active || ent.type_id != 0x002CU) continue;
                ent.last_sound_cmd = 0U;
                entity::apply_solid_body_repulsion(actor, ent);
                combat::step_entity_combat_tick(ent);
                entity::step_002c_death_and_velocity(ent, 136);
                if (ent.last_sound_cmd != 0U) {
                    sound_queue_.push(static_cast<int>(frame), ent.last_sound_cmd);
                }
            }
        }
    }
    for (auto& entity : entities_) {
        if (entity.active && entity.type_id != 0x002CU &&
            !(map_id_ == world::kTownMapId && !rom_bytes_.empty() && entity.type_id == 0x0060U)) {
            entity.anim_frame = static_cast<std::uint8_t>((entity.anim_frame + 1U) % 32U);
        }
    }

    streamer_.step_frame(camera_.camera_x(), camera_.camera_y(), current_vdp_);
    camera_.apply_to_vdp(current_vdp_);
    if (map_id_ == world::kTownMapId && !rom_bytes_.empty()) {
        render::step_town_water_cycle(town_water_timer_, town_water_phase_, current_vdp_);
    } else {
        render::apply_player_patterns_and_water_cycle(walk_patterns_, current_player_pose(), frame, current_vdp_);
    }

    const bool is_sat_step = (map_id_ == world::kTownMapId)
        ? ((tick_counter_ % 2U) == 0U)
        : is_logic_step;
    if (is_sat_step) {
        update_sat_sprites(current_vdp_);
    } else if (map_id_ == world::kTownMapId) {
        sat_pipeline_.reset_frame();
        sat_pipeline_.finalize_frame();
    }
    out_vdp = current_vdp_;
}

void StandaloneGameSimulation::update_sat_sprites(genesis::VdpState& vdp) {
    if (map_id_ == world::kTownMapId) {
        if (!rom_bytes_.empty()) {
            render::build_town_sat_and_dma(
                sat_pipeline_, rom_bytes_, camera_, player_, tick_counter_, sim_state_.frame_counter,
                sim_state_.player_x, sim_state_.player_y, walk_tick_, current_player_pose(),
                attract_banner_enabled_, baseline_vdp_.is_display_enabled(), entities_, dma_queue_);
            if (baseline_vdp_.is_display_enabled()) {
                pending_vdp_stage_ = true;
                return;
            }
            sat_pipeline_.commit_to_vdp(vdp);
            dma_queue_.drain_lifo(rom_bytes_, vdp);
            return;
        }
        render::emit_town_combat_sat(sat_pipeline_, hud_, camera_, player_, sim_state_.player_x, sim_state_.player_y, walk_tick_, entities_);
        sat_pipeline_.commit_to_vdp(vdp);
        return;
    }
    render::emit_beach_sat(sat_pipeline_, hud_, camera_, player_, sim_state_.player_x, sim_state_.player_y, current_player_pose(), walk_tick_, entities_);
    sat_pipeline_.commit_to_vdp(vdp);
}

} // namespace oasis::game