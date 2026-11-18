cmake_minimum_required(VERSION 3.20)
project(sega_thor LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(oasis_core
    src/core/rom.cpp
    src/core/rom_identity.cpp
    src/core/sha256.cpp
    src/core/mechanical_primitive.cpp
    src/core/table_copy_routine.cpp
    src/core/ram_flag_routine.cpp
    src/core/parent_suffix.cpp
    src/core/runtime.cpp
    src/genesis/memory_bus.cpp
    src/genesis/vdp.cpp
    src/genesis/vdp_state.cpp
    src/genesis/vdp_rasterizer.cpp
    src/game/translated_routines.cpp
    src/game/graphics_decompress.cpp
    src/game/genesis_graphics.cpp
    src/game/resource_loader.cpp
    src/game/screen_resource_loader.cpp
    src/game/fixed_count_resource_loader.cpp
    src/game/resource_diagnostic.cpp
    src/game/scripts/event_router.cpp
    src/game/entities/entity_pool.cpp
    src/game/entities/entity_struct_layout.cpp
    src/game/entities/velocity_vector_math.cpp
    src/game/math/isqrt_atan2.cpp
    src/game/entities/ai_steering_gravity.cpp
    src/game/entities/viewport_culler_landing.cpp
    src/game/entities/solid_entity_sweep.cpp
    src/game/entities/entity_pool_allocators.cpp
    src/game/entities/prng_lfsr.cpp
    src/game/entities/entity_anim_sequencer.cpp
    src/game/entities/raw_record_initializer.cpp
    src/game/entities/sparse_word_store.cpp
    src/game/entities/screen_entity.cpp
    src/game/entity/entity_physics.cpp
    src/game/entity/town_entity_animation.cpp
    src/game/entity/town_entity_scheduler.cpp
    src/game/entity/rom_entity_tables.cpp
    src/game/combat/elemental_damage.cpp
    src/game/combat/combat_hitbox_kernel.cpp
    src/game/combat/hit_reactions_and_sparks.cpp
    src/game/combat/explosion_spawner.cpp
    src/game/combat/projectile_latch_burst.cpp
    src/game/combat/projectile_hit_knockback.cpp
    src/game/combat/bomb_detonation.cpp
    src/game/combat/enemy_loot_drop.cpp
    src/game/combat/kill_counter_vm_tick.cpp
    src/game/combat/resumable_hitbox_iter.cpp
    src/game/combat/weapon_hitbox_builder.cpp
    src/game/combat/combat_hit_dispatch.cpp
    src/game/combat/combat_hit_apply.cpp
    src/game/combat/combat_overlap_engine.cpp
    src/game/combat/weapon_hitbox_streamer.cpp
    src/game/combat/attack_scanner_engine.cpp
    src/game/combat/combat_hit_response.cpp
    src/game/combat/hit_spark_generator.cpp
    src/game/hud/decimal_formatter.cpp
    src/game/collision/player_rect_collision.cpp
    src/game/font/font_glyph_packer.cpp
    src/game/dialogue/dialogue_vm.cpp
    src/game/dialogue/dialogue_scroll_dma.cpp
    src/game/dialogue/dialogue_task_compositor.cpp
    src/game/map/map_scene_dispatcher.cpp
    src/game/world/room_warp_engine.cpp
    src/game/world/spirit_warp_bgm.cpp
    src/game/world/room_presentation_jingle.cpp
    src/game/save/sram_save_system.cpp
    src/game/items/item_effects.cpp
    src/game/items/item_attribute_tables.cpp
    src/game/ui/ring_menu_and_rank.cpp
    src/game/ui/game_mode_flow.cpp
    src/game/ui/world_map_and_records.cpp
    src/game/ui/file_select_menu.cpp
    src/game/ui/new_game_init.cpp
    src/game/ui/continue_and_checkpoint.cpp
    src/game/ui/hud_gauge_bars.cpp
    src/game/ui/vint_hud_slide.cpp
    src/game/ui/dialogue_sat_builder.cpp
    src/game/ui/menu_sprite_dma.cpp
    src/game/ui/inventory_grid_ui.cpp
    src/game/ui/glyph_outliner.cpp
    src/game/ui/decimal_ascii.cpp
    src/game/ui/ring_menu_loop.cpp
    src/game/ui/ring_menu_drop.cpp
    src/game/ui/food_menu.cpp
    src/game/ui/save_prompt.cpp
    src/game/ui/hud_sat_emitter.cpp
    src/game/ui/debug_warp_menu.cpp
    src/game/video/palette_fade.cpp
    src/game/video/vdp_clear_and_palette_cycle.cpp
    src/game/video/vdp_dma_exec.cpp
    src/game/video/vint_dma_and_input_ring.cpp
    src/game/video/ysort_sat_pipeline.cpp
    src/game/video/sprite_dma_throttle.cpp
    src/game/enemy/enemy_ai_0030_0048.cpp
    src/game/enemy/dungeon_enemy_dispatch.cpp
    src/game/enemy/heavy_enemy_dispatch.cpp
    src/game/audio/sound_command_dispatch.cpp
    src/game/audio/sound_test_ost.cpp
    src/game/audio/sound_channels.cpp
    src/game/audio/spatial_audio.cpp
    src/game/spirits/efreet_controller.cpp
    src/game/spirits/shade_bow_controllers.cpp
    src/game/spirits/all_spirits_registry.cpp
    src/game/spirits/spirit_slots.cpp
    src/game/player/player.cpp
    src/game/player/player_action_machine.cpp
    src/game/player/special_move_inputs.cpp
    src/game/player/secondary_weapon.cpp
    src/game/player/consumable_effects.cpp
    src/game/player/weapon_equip_summon.cpp
    src/game/player/ali_attack_dispatch.cpp
    src/game/player/ali_input_dispatch.cpp
    src/game/player/ali_jump_pickup_probe.cpp
    src/game/player/ali_weapon_durability.cpp
    src/game/player/ali_command_motions.cpp
    src/game/player/ali_chained_super_motion.cpp
    src/game/player/ali_rapid_tripletap_demo.cpp
    src/game/player/master_frame_sequencer.cpp
    src/game/player/entity_constructor_lifecycle.cpp
    src/game/player/anim_palette_entity_stepper.cpp
    src/game/player/entity_sim_anim_preflight.cpp
    src/game/player/horizontal_raycaster.cpp
    src/game/player/vertical_raycaster.cpp
    src/game/player/horizontal_wall_probe.cpp
    src/game/player/vertical_wall_probe.cpp
    src/game/player/flying_raycaster.cpp
    src/game/player/point_tile_integrator.cpp
    src/game/player/slope_elevation_engine.cpp
    src/game/player/bounding_box_pushable_latch.cpp
    src/game/player/world_boundary_clamps.cpp
    src/game/player/horizontal_entity_sweeps.cpp
    src/game/player/vertical_entity_sweeps.cpp
    src/game/input/mash_tripletap_detectors.cpp
    src/game/controlled_screen.cpp
    src/game/screen_instance.cpp
    src/game/render/framebuffer.cpp
    src/game/render/debug_overlay.cpp
    src/game/render/sat_shadow_records.cpp
    src/game/render/sat_pipeline_contract.cpp
    src/game/render/hud_overlay.cpp
    src/game/render/sprite_compositor.cpp
    src/game/render/player_sprite_composer.cpp
    src/game/render/beach_sat_emitter.cpp
    src/game/render/rom_sprite_decoder.cpp
    src/game/render/master_sat_dispatcher.cpp
    src/game/render/dialogue_sprite_subsystem.cpp
    src/game/render/pass_b_sat_dispatcher.cpp
    src/game/render/shadow_sat_pass.cpp
    src/game/render/demo_hud_spark_generator.cpp
    src/game/render/master_sat_compositor.cpp
    src/game/render/static_fast_sat_compositor.cpp
    src/game/render/weapon_overlay_compositor.cpp
    src/game/render/dynamic_fx_renderer_p1.cpp
    src/game/render/dynamic_fx_renderer_p2.cpp
    src/game/render/vdp_dma_queue.cpp
    src/game/render/town_combat_sat.cpp
    src/game/render/scroll_integrator.cpp
    src/game/render/vdp_scroll_builder.cpp
    src/game/render/bg_tile_dma.cpp
    src/game/render/scene_header_dma.cpp
    src/game/render/dual_plane_loader.cpp
    src/game/render/visible_plane_streamer.cpp
    src/game/render/tilemap_rect_ops.cpp
    src/game/camera/camera_controller.cpp
    src/game/camera/camera_follow.cpp
    src/game/world/byte_grid.cpp
    src/game/world/screen_descriptor.cpp
    src/game/world/terrain_collision.cpp
    src/game/world/terrain_probe_and_clamp.cpp
    src/game/world/room_layer_loader.cpp
    src/game/world/vdp_plane_blitter.cpp
    src/game/world/vdp_plane_scroll.cpp
    src/game/world/terrain_elevation_probe.cpp
    src/game/world/room_area_loader.cpp
    src/game/world/room_warp_sequence.cpp
    src/game/world/plane_scroll_sync.cpp
    src/game/world/room_height_tileset.cpp
    src/game/world/player_tile_box.cpp
    src/game/world/aabb_solid_sweep_3axis.cpp
    src/game/world/tilemap_streamer.cpp
    src/game/world/town_scene_loader.cpp
    src/game/world/world_graph.cpp
    src/game/world/entity_pool_alloc.cpp
    src/game/scene_transition_helpers.cpp
    src/game/event/event_vm.cpp
    src/game/events/event_vm.cpp
    src/game/npc_dialogue_handler.cpp
    src/game/dialogue/dialogue_engine.cpp
    src/game/render/dialogue_window.cpp
    src/game/render/dytto_sat_renderer.cpp
    src/game/spirits/spirit_awakening.cpp
    src/game/spirits/dytto_abilities.cpp
    src/game/spirits/dytto_controller.cpp
    src/game/interaction/interaction_system.cpp
    src/game/interaction/interaction_dispatcher.cpp
    src/game/combat/attack_controller.cpp
    src/game/standalone_game_simulation.cpp
)
target_include_directories(oasis_core PUBLIC src include)
get_target_property(oasis_core_links oasis_core LINK_LIBRARIES)
if(oasis_core_links)
    message(FATAL_ERROR "oasis_core must not link to developer-only or platform targets: ${oasis_core_links}")
endif()
add_executable(oasis src/main.cpp)
add_library(oasis_platform src/platform/window.cpp)
target_include_directories(oasis_platform PUBLIC src)
target_link_libraries(oasis_platform PUBLIC oasis_core)
target_link_libraries(oasis PRIVATE oasis_core oasis_platform)
if(WIN32)
    target_link_libraries(oasis PRIVATE user32 gdi32)
endif()
add_executable(oasis_standalone_first_pixel src/tools/standalone_first_pixel.cpp)
target_link_libraries(oasis_standalone_first_pixel PRIVATE oasis_core)

add_executable(oasis_standalone_player src/tools/standalone_player.cpp)
target_link_libraries(oasis_standalone_player PRIVATE oasis_core oasis_platform)
if(WIN32)
    target_link_libraries(oasis_standalone_player PRIVATE user32 gdi32)
endif()

add_executable(oasis_inspect src/tools/asset_inspector.cpp)
target_link_libraries(oasis_inspect PRIVATE oasis_core)
add_executable(oasis_resource_boundary_scan src/tools/re_resource_boundary_scan.cpp)
target_link_libraries(oasis_resource_boundary_scan PRIVATE oasis_core)
include(tools/bizhawk-native-ring/live_forward.cmake)
# Developer-only reverse-engineering tooling; it is not linked into oasis_core.
add_library(oasis_re_tooling
    src/tools/gpgx_import_io.cpp
    src/tools/gpgx_json.cpp
    src/tools/re_slice_decoder.cpp
    src/tools/re_slice_exact.cpp
    src/tools/re_assemble.cpp
    src/tools/re_assemble_format.cpp
    src/tools/re_slice_format.cpp
    src/tools/re_program.cpp
    src/tools/re_program_format.cpp
    src/tools/re_trace.cpp
    src/tools/re_trace_format.cpp
    src/tools/re_diff.cpp
    src/tools/re_diff_detail.cpp
    src/tools/re_diff_format.cpp
    src/tools/re_atlas.cpp
    src/tools/re_atlas_format.cpp
    src/tools/re_atlas_ranking.cpp
    src/tools/re_resolution.cpp
    src/tools/re_resolution_format.cpp
    src/tools/re_cfg_audit.cpp
    src/tools/re_cfg_audit_format.cpp
    src/tools/re_reachable_closure.cpp
    src/tools/re_reachable_closure_format.cpp
    src/tools/re_reachable_stack.cpp
    src/tools/re_callee_effect.cpp
    src/tools/re_callee_effect_format.cpp
    src/tools/re_caller_stack.cpp
    src/tools/re_caller_stack_format.cpp
    src/tools/re_emulator_trace.cpp
    src/tools/re_emulator_trace_format.cpp
    src/tools/re_scenario.cpp
    src/tools/re_candidate_map.cpp
    src/tools/re_candidate_map_parse.cpp
    src/tools/re_candidate_map_format.cpp
    src/tools/re_mass_verify.cpp
    src/tools/re_mass_verify_format.cpp
    src/tools/re_explore.cpp
    src/tools/re_explore_format.cpp
    src/tools/re_explore_dynamic.cpp
    src/tools/re_ant.cpp
    src/tools/re_ant_format.cpp
    src/tools/re_ant_queue.cpp
    src/tools/re_ant_queue_format.cpp
    src/tools/re_static_translation.cpp
)
target_include_directories(oasis_re_tooling PUBLIC src)
add_executable(oasis_re_slice src/tools/re_slice_report.cpp)
target_link_libraries(oasis_re_slice PRIVATE oasis_core oasis_re_tooling)
add_executable(oasis_re_program src/tools/re_program_report.cpp)
target_link_libraries(oasis_re_program PRIVATE oasis_core oasis_re_tooling)
add_executable(oasis_re_trace src/tools/re_trace_report.cpp)
target_link_libraries(oasis_re_trace PRIVATE oasis_core oasis_re_tooling)
add_executable(oasis_re_diff src/tools/re_diff_report.cpp)
target_link_libraries(oasis_re_diff PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_atlas src/tools/re_atlas_report.cpp)
target_link_libraries(oasis_re_atlas PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_atlas_rank src/tools/re_atlas_ranking_report.cpp)
target_link_libraries(oasis_re_atlas_rank PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_resolution src/tools/re_resolution_report.cpp)
target_link_libraries(oasis_re_resolution PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_cfg_audit src/tools/re_cfg_audit_report.cpp)
target_link_libraries(oasis_re_cfg_audit PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_reachable_closure src/tools/re_reachable_closure_report.cpp)
target_link_libraries(oasis_re_reachable_closure PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_callee_effect src/tools/re_callee_effect_report.cpp)
target_link_libraries(oasis_re_callee_effect PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_caller_stack src/tools/re_caller_stack_report.cpp)
target_link_libraries(oasis_re_caller_stack PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_emulator_trace src/tools/re_emulator_trace_report.cpp)
target_link_libraries(oasis_re_emulator_trace PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_candidate_map src/tools/re_candidate_map_report.cpp)
target_link_libraries(oasis_re_candidate_map PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_mass_verify src/tools/re_mass_verify_report.cpp)
target_link_libraries(oasis_re_mass_verify PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_explore src/tools/re_explore_report.cpp)
target_link_libraries(oasis_re_explore PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_ant src/tools/re_ant_report.cpp)
target_link_libraries(oasis_re_ant PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_ant_queue src/tools/re_ant_queue_report.cpp)
target_link_libraries(oasis_re_ant_queue PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_static_translation src/tools/re_static_translation_report.cpp)
target_link_libraries(oasis_re_static_translation PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_gpgx_coverage_report src/tools/gpgx_coverage_report.cpp)
target_link_libraries(oasis_gpgx_coverage_report PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_import_gpgx_coverage src/tools/gpgx_import_gpgx_coverage.cpp)
target_link_libraries(oasis_re_import_gpgx_coverage PRIVATE oasis_re_tooling oasis_core)

include(cmake/project_tests.cmake)

option(ENABLE_HYBRID "Enable external emulator hybrid experiment" OFF)
if(ENABLE_HYBRID)
    add_subdirectory(src/tools/hybrid)
endif()
