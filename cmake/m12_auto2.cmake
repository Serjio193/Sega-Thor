add_executable(oasis_screen_resource_boundary_scan
               ${CMAKE_SOURCE_DIR}/src/tools/re_screen_resource_boundary_scan.cpp)
target_link_libraries(oasis_screen_resource_boundary_scan PRIVATE oasis_core)

add_executable(oasis_m12_pointer_resource_scan
               ${CMAKE_SOURCE_DIR}/src/tools/re_m12_pointer_resource_scan.cpp)
target_link_libraries(oasis_m12_pointer_resource_scan PRIVATE oasis_core)

add_executable(oasis_graphics_stream_census
               ${CMAKE_SOURCE_DIR}/src/tools/re_graphics_stream_census.cpp)
target_link_libraries(oasis_graphics_stream_census PRIVATE oasis_core)

if(Python3_Interpreter_FOUND)
    add_test(NAME oasis_re_m12_screen_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_screen_promote_test.py)
    add_test(NAME oasis_re_m12_gfx_max_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_max_closure_test.py)
    add_test(NAME oasis_re_m12_gfx_loader_census_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_gfx_loader_census_test.py)
    add_test(NAME oasis_re_m12_z80_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_z80_promote_test.py)
    add_test(NAME oasis_re_m12_sound_data_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_sound_data_promote_test.py)
    add_test(NAME oasis_re_m12_consumer_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_consumer_promote_test.py)
    add_test(NAME oasis_re_m12_script_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_script_promote_test.py)
    add_test(NAME oasis_re_m12_level_table_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_level_table_promote_test.py)
    add_test(NAME oasis_re_m12_lookup_table_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_lookup_table_promote_test.py)
    add_test(NAME oasis_re_m12_fixed_stride_table_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_fixed_stride_table_promote_test.py)
    add_test(NAME oasis_re_m12_direct_graphics_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_direct_graphics_promote_test.py)
    add_test(NAME oasis_re_m12_direct_graphics_chain_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_direct_graphics_chain_promote_test.py)
    add_test(NAME oasis_re_m12_padding_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_padding_promote_test.py)
    add_test(NAME oasis_re_m12_record_stream_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_record_stream_promote_test.py)
    add_test(NAME oasis_re_m12_table_graphics_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_table_graphics_promote_test.py)
    add_test(NAME oasis_re_m12_ccb0_target_table_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_ccb0_target_tables_promote_test.py)
    add_test(NAME oasis_re_m12_exact_small_table_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_exact_small_tables_promote_test.py)
    add_test(NAME oasis_re_m12_indexed_offset_table_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_indexed_offset_table_promote_test.py)
    add_test(NAME oasis_re_m12_pc_relative_dispatch_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_pc_relative_dispatch_test.py)
    add_test(NAME oasis_re_m12_descriptor_stream_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_descriptor_stream_promote_test.py)
    add_test(NAME oasis_re_m12_multi_resource_family_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_multi_resource_family_test.py)
    add_test(NAME oasis_re_m12_static_stream_pointer_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_static_stream_pointer_test.py)
    add_test(NAME oasis_re_m12_direct_loader_stream_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_direct_loader_stream_test.py)
    add_test(NAME oasis_re_m12_pc_lookup_family_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_pc_lookup_family_test.py)
    add_test(NAME oasis_re_m12_pc_island_tables_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_pc_island_tables_test.py)
    add_test(NAME oasis_re_m12_table_03b8de_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_table_03b8de_test.py)
    add_test(NAME oasis_re_m12_child_tables_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_child_tables_promote_test.py)
    add_test(NAME oasis_re_m12_pc_word_overlap_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_pc_word_overlap_test.py)
    add_test(NAME oasis_re_m12_bounded_word_transform_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_bounded_word_transform_test.py)
    add_test(NAME oasis_re_m12_static_003260_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_static_003260_test.py)
    add_test(NAME oasis_re_m12_bounded_word_copy_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_bounded_word_copy_test.py)
    add_test(NAME oasis_re_m12_contiguous_island_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_contiguous_islands_test.py)
    add_test(NAME oasis_re_m12_carver_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_carver_test.py)
    add_test(NAME oasis_re_m12_carver_expansion_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_carver_expansion_test.py)
    add_test(NAME oasis_re_m12_carver_global_sweep_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_carver_global_sweep_test.py)
    add_test(NAME oasis_re_m12_carver_static_recovery_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_carver_static_recovery_test.py)
    add_test(NAME oasis_re_m12_carver_format_reconstruction_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_carver_format_reconstruction_test.py)
    add_test(NAME oasis_re_thor_evidence_live_discovery_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_live_discovery_test.py)
    add_test(NAME oasis_re_thor_evidence_followup_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_followup_test.py)
    add_test(NAME oasis_re_thor_evidence_auto64_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto64_test.py)
    add_test(NAME oasis_re_thor_evidence_auto64_analyze_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto64_analyze_test.py)
    add_test(NAME oasis_re_thor_evidence_auto64_coverage_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto64_coverage_test.py)
    add_test(NAME oasis_re_thor_evidence_auto64_static_queue_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto64_static_queue_test.py)
    add_test(NAME oasis_re_m12_sprite_reconstruction_helpers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_reconstruction_test.py)
    add_test(NAME oasis_re_m12_sprite_context_catalog_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_context_catalog_test.py)
    add_test(NAME oasis_re_m12_static_sprite_dispatch_catalog_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_static_sprite_dispatch_catalog_test.py)
    add_test(NAME oasis_re_m12_selector_descriptor_grammar_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_selector_descriptor_grammar_test.py)
    add_test(NAME oasis_re_m12_selector_control_analysis_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_selector_control_analysis_test.py)
    add_test(NAME oasis_re_m12_relative_table_analysis_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_relative_table_analysis_test.py)
    add_test(NAME oasis_re_m12_indirect_body_dispatch_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_indirect_body_dispatch_test.py)
    add_test(NAME oasis_re_m12_b092_tail_dispatch_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_b092_tail_dispatch_test.py)
    add_test(NAME oasis_re_m12_a372_shadow_sat_producer_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_a372_shadow_sat_producer_test.py)
    add_test(NAME oasis_re_m12_a372_caller_join_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_a372_caller_join_test.py)
    add_test(NAME oasis_re_m12_a372_runtime_report_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_a372_runtime_report_test.py)
    add_test(NAME oasis_re_m12_shadow_sat_access_graph_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_shadow_sat_access_graph_test.py)
    add_test(NAME oasis_thor_evidence_v0_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v0_test.py)
    add_test(NAME oasis_thor_evidence_v1_gate_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v1_gate_test.py)
    add_test(NAME oasis_thor_evidence_dense_gate_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_dense_gate_test.py)
    add_test(NAME oasis_thor_evidence_v1_canary_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v1_canary_test.py)
    add_test(NAME oasis_thor_evidence_v2_ram_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v2_ram_test.py)
    add_test(NAME oasis_thor_evidence_v3_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v3_test.py)
    add_test(NAME oasis_thor_evidence_v4_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v4_test.py)
    add_test(NAME oasis_thor_evidence_v5_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v5_test.py)
    add_test(NAME oasis_thor_evidence_v6_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v6_test.py)
    add_test(NAME oasis_thor_evidence_v7_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v7_test.py)
    add_test(NAME oasis_thor_evidence_v8_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v8_test.py)
    add_test(NAME oasis_thor_evidence_v9_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_v9_test.py)
endif()
