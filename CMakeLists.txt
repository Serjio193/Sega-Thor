cmake_minimum_required(VERSION 3.20)
project(sega_thor LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(oasis_core
    src/core/rom.cpp
    src/core/rom_identity.cpp
    src/core/mechanical_primitive.cpp
    src/core/table_copy_routine.cpp
    src/core/ram_flag_routine.cpp
    src/core/parent_suffix.cpp
    src/core/runtime.cpp
    src/genesis/memory_bus.cpp
    src/genesis/vdp.cpp
    src/game/translated_routines.cpp
    src/game/graphics_decompress.cpp
    src/game/genesis_graphics.cpp
    src/game/resource_loader.cpp
    src/game/screen_resource_loader.cpp
    src/game/fixed_count_resource_loader.cpp
    src/game/resource_diagnostic.cpp
    src/game/scripts/event_router.cpp
    src/game/entities/entity_pool.cpp
    src/game/entities/raw_record_initializer.cpp
    src/game/entities/sparse_word_store.cpp
    src/game/entities/screen_entity.cpp
    src/game/spirits/spirit_slots.cpp
    src/game/player/player.cpp
    src/game/player/secondary_weapon.cpp
    src/game/controlled_screen.cpp
    src/game/screen_instance.cpp
    src/game/render/framebuffer.cpp
    src/game/render/debug_overlay.cpp
    src/game/render/sat_shadow_records.cpp
    src/game/render/tilemap_plane_upload.cpp
    src/game/world/byte_grid.cpp
    src/game/world/screen_descriptor.cpp
    src/game/world/terrain_collision.cpp
)
target_include_directories(oasis_core PUBLIC src)
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
add_executable(oasis_inspect src/tools/asset_inspector.cpp)
target_link_libraries(oasis_inspect PRIVATE oasis_core)
add_executable(oasis_resource_boundary_scan src/tools/re_resource_boundary_scan.cpp)
target_link_libraries(oasis_resource_boundary_scan PRIVATE oasis_core)
add_executable(oasis_gfx_screen_resource_reference tests/gfx_screen_resource_reference.cpp)
target_link_libraries(oasis_gfx_screen_resource_reference PRIVATE oasis_core)
add_executable(oasis_raw_record_initializer_test tests/raw_record_initializer_test.cpp)
target_link_libraries(oasis_raw_record_initializer_test PRIVATE oasis_core)
add_test(NAME oasis_raw_record_initializer COMMAND oasis_raw_record_initializer_test)
add_executable(oasis_sparse_word_store_test tests/sparse_word_store_test.cpp)
target_link_libraries(oasis_sparse_word_store_test PRIVATE oasis_core)
add_test(NAME oasis_sparse_word_store COMMAND oasis_sparse_word_store_test)
add_executable(oasis_sat_shadow_records_test tests/sat_shadow_records_test.cpp)
target_link_libraries(oasis_sat_shadow_records_test PRIVATE oasis_core)
add_test(NAME oasis_sat_shadow_records COMMAND oasis_sat_shadow_records_test)
add_executable(oasis_tilemap_plane_upload_test tests/tilemap_plane_upload_test.cpp)
target_link_libraries(oasis_tilemap_plane_upload_test PRIVATE oasis_core)
add_test(NAME oasis_tilemap_plane_upload COMMAND oasis_tilemap_plane_upload_test)
add_executable(oasis_fixed_count_resource_reference tests/fixed_count_resource_reference.cpp)
target_link_libraries(oasis_fixed_count_resource_reference PRIVATE oasis_core)
include(tools/bizhawk-native-ring/live_forward.cmake)
# Developer-only reverse-engineering tooling; it is not linked into oasis_core.
add_library(oasis_re_tooling
    src/tools/thor_evidence/runtime_rom_properties.cpp
    src/tools/thor_evidence/runtime_rom_observation.cpp
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
add_executable(oasis_m12_b730_native_differential src/tools/m12_b730_native_differential.cpp)
target_link_libraries(oasis_m12_b730_native_differential PRIVATE oasis_core oasis_re_tooling)
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
add_subdirectory(src/tools/hybrid) # Optional external-emulator experiment.
