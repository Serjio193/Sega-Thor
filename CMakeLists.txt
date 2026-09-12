cmake_minimum_required(VERSION 3.20)
project(sega_thor LANGUAGES CXX)
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
    src/game/resource_diagnostic.cpp
    src/game/scripts/event_router.cpp
    src/game/entities/entity_pool.cpp
    src/game/spirits/spirit_slots.cpp
    src/game/player/player.cpp
    src/game/controlled_screen.cpp
    src/game/render/framebuffer.cpp
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
enable_testing()
add_executable(oasis_re_assemble src/tools/re_assemble_report.cpp)
target_link_libraries(oasis_re_assemble PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_assemble_range src/tools/re_assemble_range_report.cpp)
target_link_libraries(oasis_re_assemble_range PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_assemble_test tests/re_assemble_test.cpp)
target_link_libraries(oasis_re_assemble_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_assemble COMMAND oasis_re_assemble_test)
add_test(NAME oasis_re_import_gpgx_coverage_self_test
         COMMAND oasis_re_import_gpgx_coverage --self-test)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME oasis_re_full_split_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_full_split_test.py)
    add_test(NAME oasis_re_auto_promote_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_auto_promote_test.py)
    add_test(NAME oasis_re_evidence_audit_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_evidence_audit_test.py)
    add_test(NAME oasis_re_dynamic_confirm_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_dynamic_confirm_test.py)
    add_test(NAME oasis_re_structured_data_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_structured_data_test.py)
    add_test(NAME oasis_re_m12_1_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_1_test.py)
    add_test(NAME oasis_re_m12_2_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_2_test.py)
    add_test(NAME oasis_re_m12_3_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_3_test.py)
    add_test(NAME oasis_re_m12_4_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_4_test.py)
    add_test(NAME oasis_re_m12_5_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_5_test.py)
    add_test(NAME oasis_re_m12_auto_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_auto_test.py)
    add_test(NAME oasis_gpgx_persistent_coverage_semantics
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/gpgx_persistent_coverage_test.py)
    add_test(NAME oasis_gpgx_unknown_priority_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/tools/gpgx_unknown_priority.py --self-test)
    add_test(NAME oasis_gpgx_bounded_classification
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/gpgx_bounded_classification_test.py)
    add_test(NAME oasis_gpgx_rom_reader_correlation
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/gpgx_rom_reader_correlation_test.py)
    add_test(NAME oasis_re_m12_ccb0_group_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_ccb0_group_table_promote_test.py)
    add_test(NAME oasis_re_m12_ccb0_selected_slots_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_ccb0_selected_slots_promote_test.py)
    add_test(NAME oasis_re_m12_exact_probe_slices_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_exact_probe_slices_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_graphics_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_graphics_promote_test.py)
    add_test(NAME oasis_re_m12_save_slots_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_save_slots_promote_test.py)
    add_test(NAME oasis_re_m12_static_code_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_static_code_promote_test.py)
    add_test(NAME oasis_re_m12_pc_tables_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_pc_tables_promote_test.py)
    add_test(NAME oasis_re_m12_callback_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_callback_promote_test.py)
    add_test(NAME oasis_re_m12_event_dispatch_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_event_dispatch_promote_test.py)
    add_test(NAME oasis_re_m12_nibble_lookup_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_nibble_lookup_promote_test.py)
    add_test(NAME oasis_re_candidate_map_to_ghidra_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_candidate_map_to_ghidra_test.py)
    add_test(NAME oasis_re_m12_dispatch_pointer_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_dispatch_pointer_table_promote_test.py)
    add_test(NAME oasis_re_m12_threshold_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_threshold_table_promote_test.py)
    add_test(NAME oasis_re_m12_label_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_label_table_promote_test.py)
    add_test(NAME oasis_re_m12_item_label_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_item_label_table_promote_test.py)
    add_test(NAME oasis_re_m12_resource_pointer_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_resource_pointer_table_promote_test.py)
    add_test(NAME oasis_re_m12_bit7_lookup_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_bit7_lookup_promote_test.py)
    add_test(NAME oasis_re_m12_menu_graphics_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_menu_graphics_promote_test.py)
    add_test(NAME oasis_re_m12_enum_lookup_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_enum_lookup_promote_test.py)
    add_test(NAME oasis_re_m12_menu_record_stream_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_menu_record_stream_promote_test.py)
    add_test(NAME oasis_re_m12_relative_selector_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_relative_selector_table_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_code_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_code_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_code_range_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_code_range_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_code_caller_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_code_caller_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_code_small_caller_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_code_small_caller_promote_test.py)
    add_test(NAME oasis_re_m12_runtime_record_regions_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_runtime_record_regions_promote_test.py)
    add_test(NAME oasis_re_m12_gfx_sweep_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_gfx_sweep_test.py)
    add_test(NAME oasis_re_m12_gfx_caller_closure_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_gfx_caller_closure_test.py)
    add_test(NAME oasis_re_m12_screen_descriptor_candidate_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_screen_descriptor_candidate_promote_test.py)
    add_test(NAME oasis_re_m12_gfx_37d2_census_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_37d2_census_test.py)
    add_test(NAME oasis_re_m12_gfx_d3b2_census_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_d3b2_census_test.py)
    add_test(NAME oasis_re_m12_gfx_36d4_census_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_36d4_census_test.py)
    add_test(NAME oasis_re_m12_gfx_sibling_census_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_sibling_census_test.py)
    add_test(NAME oasis_re_m12_gfx_d406_residual_census_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_d406_residual_census_test.py)
endif()
include(cmake/m12_auto2.cmake)
add_executable(oasis_smoke tests/smoke.cpp)
target_link_libraries(oasis_smoke PRIVATE oasis_core)
add_test(NAME oasis_smoke COMMAND oasis_smoke)
add_executable(oasis_mechanical_primitive_test tests/mechanical_primitive_test.cpp)
target_link_libraries(oasis_mechanical_primitive_test PRIVATE oasis_core)
add_test(NAME oasis_mechanical_primitive COMMAND oasis_mechanical_primitive_test)
add_executable(oasis_table_copy_routine_test tests/table_copy_routine_test.cpp)
target_link_libraries(oasis_table_copy_routine_test PRIVATE oasis_core)
add_test(NAME oasis_table_copy_routine COMMAND oasis_table_copy_routine_test)
add_executable(oasis_ram_flag_routine_test tests/ram_flag_routine_test.cpp)
target_link_libraries(oasis_ram_flag_routine_test PRIVATE oasis_core)
add_test(NAME oasis_ram_flag_routine COMMAND oasis_ram_flag_routine_test)
add_test(
    NAME oasis_core_dependency_boundary
    COMMAND ${CMAKE_COMMAND}
        -DCORE_SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/core
        -P ${CMAKE_SOURCE_DIR}/tests/check_core_boundary.cmake
)
add_executable(oasis_rom_identity_test tests/rom_identity_test.cpp)
target_link_libraries(oasis_rom_identity_test PRIVATE oasis_core)
add_test(NAME oasis_rom_identity COMMAND oasis_rom_identity_test)
add_executable(oasis_graphics_decompress_test tests/graphics_decompress_test.cpp)
target_link_libraries(oasis_graphics_decompress_test PRIVATE oasis_core)
add_test(NAME oasis_graphics_decompress COMMAND oasis_graphics_decompress_test)
add_executable(oasis_genesis_graphics_test tests/genesis_graphics_test.cpp)
target_link_libraries(oasis_genesis_graphics_test PRIVATE oasis_core)
add_test(NAME oasis_genesis_graphics COMMAND oasis_genesis_graphics_test)
add_executable(oasis_vdp_test tests/vdp_test.cpp)
target_link_libraries(oasis_vdp_test PRIVATE oasis_core)
add_test(NAME oasis_vdp COMMAND oasis_vdp_test)
add_executable(oasis_runtime_test tests/runtime_test.cpp)
target_link_libraries(oasis_runtime_test PRIVATE oasis_core)
add_test(NAME oasis_runtime COMMAND oasis_runtime_test)
add_executable(oasis_screen_descriptor_test tests/screen_descriptor_test.cpp)
target_link_libraries(oasis_screen_descriptor_test PRIVATE oasis_core)
add_test(NAME oasis_screen_descriptor COMMAND oasis_screen_descriptor_test)

add_executable(oasis_byte_grid_test tests/byte_grid_test.cpp)
target_link_libraries(oasis_byte_grid_test PRIVATE oasis_core)
add_test(NAME oasis_byte_grid COMMAND oasis_byte_grid_test)

add_executable(oasis_terrain_collision_test tests/terrain_collision_test.cpp)
target_link_libraries(oasis_terrain_collision_test PRIVATE oasis_core)
add_test(NAME oasis_terrain_collision COMMAND oasis_terrain_collision_test)

add_executable(oasis_player_test tests/player_test.cpp)
target_link_libraries(oasis_player_test PRIVATE oasis_core)
add_test(NAME oasis_player COMMAND oasis_player_test)

add_executable(oasis_native_vertical_slice_test tests/native_vertical_slice_test.cpp)
target_link_libraries(oasis_native_vertical_slice_test PRIVATE oasis_core)
add_test(NAME oasis_native_vertical_slice COMMAND oasis_native_vertical_slice_test)

add_executable(oasis_entity_pool_test tests/entity_pool_test.cpp)
target_link_libraries(oasis_entity_pool_test PRIVATE oasis_core)
add_test(NAME oasis_entity_pool COMMAND oasis_entity_pool_test)

add_executable(oasis_spirit_slots_test tests/spirit_slots_test.cpp)
target_link_libraries(oasis_spirit_slots_test PRIVATE oasis_core)
add_test(NAME oasis_spirit_slots COMMAND oasis_spirit_slots_test)

add_executable(oasis_event_router_test tests/event_router_test.cpp)
target_link_libraries(oasis_event_router_test PRIVATE oasis_core)
add_test(NAME oasis_event_router COMMAND oasis_event_router_test)

add_executable(oasis_re_slice_decoder_test tests/re_slice_decoder_test.cpp)
target_link_libraries(oasis_re_slice_decoder_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_slice_decoder COMMAND oasis_re_slice_decoder_test)
add_executable(oasis_raw_data_provenance_test tests/raw_data_provenance_test.cpp)
target_link_libraries(oasis_raw_data_provenance_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_raw_data_provenance COMMAND oasis_raw_data_provenance_test)
add_executable(oasis_table_copy_routine_contract_test tests/table_copy_routine_contract_test.cpp)
target_link_libraries(oasis_table_copy_routine_contract_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_table_copy_routine_contract COMMAND oasis_table_copy_routine_contract_test)

add_executable(oasis_re_program_test tests/re_program_test.cpp)
target_link_libraries(oasis_re_program_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_program COMMAND oasis_re_program_test)

add_executable(oasis_re_trace_test tests/re_trace_test.cpp)
target_link_libraries(oasis_re_trace_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_trace COMMAND oasis_re_trace_test)

add_executable(oasis_re_diff_test tests/re_diff_test.cpp)
target_link_libraries(oasis_re_diff_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_diff COMMAND oasis_re_diff_test)

add_executable(oasis_re_atlas_test tests/re_atlas_test.cpp)
target_link_libraries(oasis_re_atlas_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_atlas COMMAND oasis_re_atlas_test)

add_executable(oasis_re_atlas_ranking_test tests/re_atlas_ranking_test.cpp)
target_link_libraries(oasis_re_atlas_ranking_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_atlas_ranking COMMAND oasis_re_atlas_ranking_test)

add_executable(oasis_re_resolution_test tests/re_resolution_test.cpp)
target_link_libraries(oasis_re_resolution_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_resolution COMMAND oasis_re_resolution_test)

add_executable(oasis_re_cfg_audit_test tests/re_cfg_audit_test.cpp)
target_link_libraries(oasis_re_cfg_audit_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_cfg_audit COMMAND oasis_re_cfg_audit_test)

add_executable(oasis_re_reachable_closure_test tests/re_reachable_closure_test.cpp)
target_link_libraries(oasis_re_reachable_closure_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_reachable_closure COMMAND oasis_re_reachable_closure_test)

add_executable(oasis_re_callee_effect_test tests/re_callee_effect_test.cpp)
target_link_libraries(oasis_re_callee_effect_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_callee_effect COMMAND oasis_re_callee_effect_test)

add_executable(oasis_re_caller_stack_test tests/re_caller_stack_test.cpp)
target_link_libraries(oasis_re_caller_stack_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_caller_stack COMMAND oasis_re_caller_stack_test)

add_executable(oasis_re_emulator_trace_test tests/re_emulator_trace_test.cpp)
target_link_libraries(oasis_re_emulator_trace_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_emulator_trace COMMAND oasis_re_emulator_trace_test)

add_executable(oasis_re_scenario_test tests/re_scenario_test.cpp)
target_link_libraries(oasis_re_scenario_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_scenario COMMAND oasis_re_scenario_test)

add_executable(oasis_re_candidate_map_test tests/re_candidate_map_test.cpp)
target_link_libraries(oasis_re_candidate_map_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_candidate_map COMMAND oasis_re_candidate_map_test)

add_executable(oasis_re_mass_verify_test tests/re_mass_verify_test.cpp)
target_link_libraries(oasis_re_mass_verify_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_mass_verify COMMAND oasis_re_mass_verify_test)

add_executable(oasis_re_static_translation_test tests/re_static_translation_test.cpp)
target_link_libraries(oasis_re_static_translation_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_static_translation COMMAND oasis_re_static_translation_test)

add_executable(oasis_re_explore_test tests/re_explore_test.cpp)
target_link_libraries(oasis_re_explore_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_explore COMMAND oasis_re_explore_test)

add_executable(oasis_re_ant_test tests/re_ant_test.cpp)
target_link_libraries(oasis_re_ant_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_ant COMMAND oasis_re_ant_test)

add_executable(oasis_re_ant_queue_test tests/re_ant_queue_test.cpp)
target_link_libraries(oasis_re_ant_queue_test PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_ant_queue COMMAND oasis_re_ant_queue_test)

# These executables use a locally supplied verified ROM and are intentionally not CTest tests.
add_executable(oasis_graphics_reference tests/graphics_decompress_reference.cpp)
target_link_libraries(oasis_graphics_reference PRIVATE oasis_core)
add_executable(oasis_resource_id3_reference tests/resource_id3_reference.cpp)
target_link_libraries(oasis_resource_id3_reference PRIVATE oasis_core)
add_executable(oasis_screen_descriptor_reference tests/screen_descriptor_reference.cpp)
target_link_libraries(oasis_screen_descriptor_reference PRIVATE oasis_core)
add_executable(oasis_player_reference tests/player_reference.cpp)
target_link_libraries(oasis_player_reference PRIVATE oasis_core)

add_executable(oasis_entity_pool_reference tests/entity_pool_reference.cpp)
target_link_libraries(oasis_entity_pool_reference PRIVATE oasis_core)

add_executable(oasis_spirit_slots_reference tests/spirit_slots_reference.cpp)
target_link_libraries(oasis_spirit_slots_reference PRIVATE oasis_core)

add_executable(oasis_event_router_reference tests/event_router_reference.cpp)
target_link_libraries(oasis_event_router_reference PRIVATE oasis_core)

add_executable(oasis_re_slice_reference tests/re_slice_reference.cpp)
target_link_libraries(oasis_re_slice_reference PRIVATE oasis_core oasis_re_tooling)

add_executable(oasis_re_program_reference tests/re_program_reference.cpp)
target_link_libraries(oasis_re_program_reference PRIVATE oasis_core oasis_re_tooling)

add_executable(oasis_re_trace_reference tests/re_trace_reference.cpp)
target_link_libraries(oasis_re_trace_reference PRIVATE oasis_core oasis_re_tooling)

add_executable(oasis_re_diff_reference tests/re_diff_reference.cpp)
target_link_libraries(oasis_re_diff_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_atlas_reference tests/re_atlas_reference.cpp)
target_link_libraries(oasis_re_atlas_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_resolution_reference tests/re_resolution_reference.cpp)
target_link_libraries(oasis_re_resolution_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_cfg_audit_reference tests/re_cfg_audit_reference.cpp)
target_link_libraries(oasis_re_cfg_audit_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_reachable_closure_reference tests/re_reachable_closure_reference.cpp)
target_link_libraries(oasis_re_reachable_closure_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_callee_effect_reference tests/re_callee_effect_reference.cpp)
target_link_libraries(oasis_re_callee_effect_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_caller_stack_reference tests/re_caller_stack_reference.cpp)
target_link_libraries(oasis_re_caller_stack_reference PRIVATE oasis_re_tooling oasis_core)

add_executable(oasis_re_natural_reference tests/re_natural_reference.cpp)
target_link_libraries(oasis_re_natural_reference PRIVATE oasis_core)

# Keep assert-based test bodies active in Release builds as well.
set(oasis_test_targets
    oasis_re_assemble_test
    oasis_smoke
    oasis_mechanical_primitive_test
    oasis_rom_identity_test
    oasis_graphics_decompress_test
    oasis_genesis_graphics_test
    oasis_vdp_test
    oasis_runtime_test
    oasis_screen_descriptor_test
    oasis_byte_grid_test
    oasis_terrain_collision_test
    oasis_player_test
    oasis_native_vertical_slice_test
    oasis_entity_pool_test
    oasis_spirit_slots_test
    oasis_event_router_test
    oasis_re_slice_decoder_test
    oasis_raw_data_provenance_test
    oasis_re_program_test
    oasis_re_trace_test
    oasis_re_diff_test
    oasis_re_atlas_test
    oasis_re_atlas_ranking_test
    oasis_re_resolution_test
    oasis_re_cfg_audit_test
    oasis_re_reachable_closure_test
    oasis_re_callee_effect_test
    oasis_re_caller_stack_test
    oasis_re_emulator_trace_test
    oasis_re_scenario_test
    oasis_re_candidate_map_test
    oasis_re_mass_verify_test
    oasis_re_static_translation_test
    oasis_re_explore_test
    oasis_re_ant_test
    oasis_re_ant_queue_test
)
foreach(test_target IN LISTS oasis_test_targets)
    target_compile_options(${test_target} PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
endforeach()
add_test(
    NAME project_file_line_limit
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_file_limits.cmake
)
# LLVM-MinGW links the test executables to runtime DLLs. Keep CTest runnable
# from a clean shell without requiring the user to edit PATH manually.
if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    get_filename_component(oasis_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(oasis_runtime_test_names
        oasis_re_assemble
        oasis_re_import_gpgx_coverage_self_test
        oasis_smoke
        oasis_mechanical_primitive
        oasis_core_dependency_boundary
        oasis_rom_identity
        oasis_graphics_decompress
        oasis_genesis_graphics
        oasis_vdp
        oasis_runtime
        oasis_screen_descriptor
        oasis_byte_grid
        oasis_terrain_collision
        oasis_player
        oasis_native_vertical_slice
        oasis_entity_pool
        oasis_spirit_slots
        oasis_event_router
        oasis_re_slice_decoder
        oasis_raw_data_provenance
        oasis_re_program
        oasis_re_trace
        oasis_re_diff
        oasis_re_atlas
        oasis_re_atlas_ranking
        oasis_re_resolution
        oasis_re_cfg_audit
        oasis_re_reachable_closure
        oasis_re_callee_effect
        oasis_re_caller_stack
        oasis_re_emulator_trace
        oasis_re_candidate_map
        oasis_re_mass_verify
        oasis_re_static_translation
        oasis_re_explore
        oasis_gpgx_unknown_priority_self_test
        oasis_re_ant
        project_file_line_limit
    )
    set_tests_properties(${oasis_runtime_test_names}
        PROPERTIES ENVIRONMENT "PATH=${oasis_compiler_bin}"
    )
endif()
add_subdirectory(src/tools/hybrid) # Optional external-emulator experiment.
