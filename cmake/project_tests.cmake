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
    add_test(NAME oasis_re_m12_gfx_runtime_provenance_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_runtime_provenance_test.py)
    add_test(NAME oasis_re_m12_gfx_runtime_provenance_validator_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_runtime_provenance_validator_test.py)
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

add_executable(oasis_vdp_rasterizer_test tests/vdp_rasterizer_test.cpp)
target_link_libraries(oasis_vdp_rasterizer_test PRIVATE oasis_core)
add_test(NAME oasis_vdp_rasterizer COMMAND oasis_vdp_rasterizer_test)
add_executable(oasis_standalone_sequencer_test tests/standalone_sequencer_test.cpp)
target_link_libraries(oasis_standalone_sequencer_test PRIVATE oasis_core)
add_test(NAME oasis_standalone_sequencer COMMAND oasis_standalone_sequencer_test)
add_executable(oasis_standalone_gameplay_test tests/standalone_gameplay_test.cpp)
target_link_libraries(oasis_standalone_gameplay_test PRIVATE oasis_core)
add_test(NAME oasis_standalone_gameplay COMMAND oasis_standalone_gameplay_test)
add_executable(oasis_interaction_event_test tests/interaction_event_test.cpp)
target_link_libraries(oasis_interaction_event_test PRIVATE oasis_core)
add_test(NAME oasis_interaction_event COMMAND oasis_interaction_event_test)
add_executable(oasis_attack_journal_differential_test tests/attack_journal_differential_test.cpp)
target_link_libraries(oasis_attack_journal_differential_test PRIVATE oasis_core)
add_test(NAME oasis_attack_journal_differential COMMAND oasis_attack_journal_differential_test)
add_executable(oasis_m16_21_combat_oracle_test tests/m16_21_combat_oracle_test.cpp)
target_link_libraries(oasis_m16_21_combat_oracle_test PRIVATE oasis_core)
add_test(NAME oasis_m16_21_combat_oracle COMMAND oasis_m16_21_combat_oracle_test)
add_executable(oasis_m16_22_town_combat_test tests/m16_22_town_combat_test.cpp)
target_link_libraries(oasis_m16_22_town_combat_test PRIVATE oasis_core)
add_test(NAME oasis_m16_22_town_combat COMMAND oasis_m16_22_town_combat_test)
add_executable(oasis_differential_oracle_comparison_test tests/differential_oracle_comparison_test.cpp)
target_link_libraries(oasis_differential_oracle_comparison_test PRIVATE oasis_core)
add_test(NAME oasis_differential_oracle_comparison COMMAND oasis_differential_oracle_comparison_test)
add_executable(oasis_sat_pipeline_contract_test tests/sat_pipeline_contract_test.cpp)
target_link_libraries(oasis_sat_pipeline_contract_test PRIVATE oasis_core)
add_test(NAME oasis_sat_pipeline_contract COMMAND oasis_sat_pipeline_contract_test)
add_executable(oasis_camera_controller_test tests/camera_controller_test.cpp)
target_link_libraries(oasis_camera_controller_test PRIVATE oasis_core)
add_test(NAME oasis_camera_controller COMMAND oasis_camera_controller_test)
add_executable(oasis_pixel_differential_test tests/pixel_differential_test.cpp)
target_link_libraries(oasis_pixel_differential_test PRIVATE oasis_core)
add_test(NAME oasis_pixel_differential COMMAND oasis_pixel_differential_test)
add_executable(oasis_alternative_trace_test tests/alternative_trace_test.cpp)
target_link_libraries(oasis_alternative_trace_test PRIVATE oasis_core)
add_test(NAME oasis_alternative_trace COMMAND oasis_alternative_trace_test)
add_executable(oasis_live_player_journal_test tests/live_player_journal_test.cpp)
target_link_libraries(oasis_live_player_journal_test PRIVATE oasis_core)
add_test(NAME oasis_live_player_journal COMMAND oasis_live_player_journal_test)
add_executable(oasis_holdout_trace_test tests/holdout_trace_test.cpp)
target_link_libraries(oasis_holdout_trace_test PRIVATE oasis_core)
add_test(NAME oasis_holdout_trace COMMAND oasis_holdout_trace_test)
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
    oasis_vdp_rasterizer_test
    oasis_standalone_sequencer_test
    oasis_standalone_gameplay_test
    oasis_interaction_event_test
    oasis_attack_journal_differential_test
    oasis_m16_21_combat_oracle_test
    oasis_m16_22_town_combat_test
    oasis_differential_oracle_comparison_test
    oasis_sat_pipeline_contract_test
    oasis_camera_controller_test
    oasis_pixel_differential_test
    oasis_alternative_trace_test
    oasis_live_player_journal_test
    oasis_holdout_trace_test
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
        oasis_vdp_rasterizer
        oasis_standalone_sequencer
        oasis_standalone_gameplay
        oasis_interaction_event
        oasis_attack_journal_differential
        oasis_m16_21_combat_oracle
        oasis_m16_22_town_combat
        oasis_differential_oracle_comparison
        oasis_sat_pipeline_contract
        oasis_camera_controller
        oasis_pixel_differential
        oasis_alternative_trace
        oasis_holdout_trace
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
    )
    set_tests_properties(${oasis_runtime_test_names}
        PROPERTIES ENVIRONMENT "PATH=${oasis_compiler_bin}"
    )
endif()
