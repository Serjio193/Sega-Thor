add_executable(oasis_screen_resource_boundary_scan
               ${CMAKE_SOURCE_DIR}/src/tools/re_screen_resource_boundary_scan.cpp)
target_link_libraries(oasis_screen_resource_boundary_scan PRIVATE oasis_core)

add_executable(oasis_m12_pointer_resource_scan
               ${CMAKE_SOURCE_DIR}/src/tools/re_m12_pointer_resource_scan.cpp)
target_link_libraries(oasis_m12_pointer_resource_scan PRIVATE oasis_core)

add_executable(oasis_graphics_stream_census
               ${CMAKE_SOURCE_DIR}/src/tools/re_graphics_stream_census.cpp)
target_link_libraries(oasis_graphics_stream_census PRIVATE oasis_core)

add_executable(oasis_re_rom_range_decode
               ${CMAKE_SOURCE_DIR}/src/tools/re_rom_range_decode.cpp
               ${CMAKE_SOURCE_DIR}/src/tools/hybrid/address_provenance.cpp)
target_link_libraries(oasis_re_rom_range_decode PRIVATE oasis_re_tooling oasis_core)
add_test(NAME oasis_re_rom_range_decode_self_test
         COMMAND oasis_re_rom_range_decode --self-test)

if(Python3_Interpreter_FOUND)
    add_test(NAME oasis_runtime_chain_fusion
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/runtime_chain_fusion_test.py)
    add_test(NAME oasis_normalized_v2_canonical_adapter
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/normalized_v2_canonical_adapter_test.py)
    add_test(NAME oasis_rom_knowledge_fusion_stream
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_fusion_stream_test.py)
    add_test(NAME oasis_rom_knowledge_pipeline_audit
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_pipeline_audit_test.py)
    add_test(NAME oasis_w3_lineage_bridge
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w3_lineage_bridge_test.py)
    add_test(NAME oasis_experiment_lifecycle
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/experiment_lifecycle_test.py)
    add_test(NAME oasis_raw_elimination
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/raw_elimination_test.py)
    add_test(NAME oasis_raw_event_envelope
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/raw_event_envelope_test.py)
    add_test(NAME oasis_runtime_path_view
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/runtime_path_view_test.py)
    add_test(NAME oasis_live_forward_cartographer
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_cartographer_test.py)
    add_test(NAME oasis_live_forward_rom_link
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_rom_link_test.py)
    add_test(NAME oasis_live_worker_control
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_worker_control_test.py)
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
    add_test(NAME oasis_re_thor_evidence_auto65_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto65_test.py)
    add_test(NAME oasis_re_thor_evidence_auto66_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto66_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_test.py)
    add_test(NAME oasis_re_thor_evidence_control_provenance_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_control_provenance_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_worker_clean_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_worker_clean_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_ring_clean_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_ring_clean_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_predispatch_transport_clean_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_predispatch_transport_clean_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_mailbox_clean_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_mailbox_clean_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_cartographer_seam_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_cartographer_seam_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_ram_session_map_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_ram_session_map_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_snapshot_admission_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_snapshot_admission_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_native_snapshot_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_native_snapshot_test.py)
    add_test(NAME oasis_re_thor_evidence_dispatcher1_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_dispatcher1_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_1_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_1_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_3_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_3_test.py)
    add_test(NAME oasis_re_thor_evidence_auto67_4_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/thor_evidence_auto67_4_test.py)
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
    add_test(NAME oasis_m12_rom_knowledge_map_2d
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_map_test.py)
    add_test(NAME oasis_m14_2b_global_evidence_fusion
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_fusion_test.py)
    add_test(NAME oasis_m14_2c_global_map_closure
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_map_closure_test.py)
    add_test(NAME oasis_m14_3_unknown_range_campaign
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_unknown_range_campaign_test.py)
    add_test(NAME oasis_m12_archivist_knowledge_pipeline_2g
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_pipeline_test.py)
    add_test(NAME oasis_m12_map_driven_executed_asm_closure_2f
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/map_driven_executed_asm_closure_test.py)
    add_test(NAME oasis_m12_map_driven_stage7
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/map_driven_stage7_test.py)
    add_test(NAME oasis_m12_control_provenance_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_control_provenance_stage_test.py)
    add_test(NAME oasis_m12_stage7_preflight
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/map_driven_stage7_preflight_test.py)
    add_test(NAME oasis_m12_absorption_cleanup
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_absorption_cleanup_test.py)
    add_test(NAME oasis_m12_master_v2_shadow
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_v2_shadow_test.py)
    add_test(NAME oasis_m12_master_canonical_view
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_canonical_view_test.py)
    add_test(NAME oasis_m12_w2_active_resource_classification
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w2_active_resource_classification_test.py)
    add_test(NAME oasis_m12_w2_frame_coherent_evidence
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w2_frame_coherent_evidence_test.py)

add_test(NAME oasis_rom_coverage_model
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_coverage_model_test.py)
    add_test(NAME oasis_rom_coverage_evidence
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_coverage_evidence_test.py)
    add_test(NAME oasis_rom_coverage_gui_live
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_coverage_gui_live_test.py)
    add_test(NAME oasis_live_session_progress
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_session_progress_test.py)
    add_test(NAME oasis_evidence_canonical_full_rom_map
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/canonical_full_rom_map_test.py)
    add_test(NAME oasis_evidence_flow_stream
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/flow_stream_test.py)
    add_test(NAME oasis_evidence_generic_flow_normalizer
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/generic_flow_normalizer_test.py)
    add_test(NAME oasis_evidence_generic_recursive_closure
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/generic_recursive_closure_test.py)
    add_test(NAME oasis_evidence_live_forward_audio_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_audio_stage_test.py)
    add_test(NAME oasis_evidence_live_forward_complete_pipeline
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_complete_pipeline_test.py)
    add_test(NAME oasis_evidence_live_forward_controlled_entity_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_controlled_entity_stage_test.py)
    add_test(NAME oasis_evidence_live_forward_gameplay_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_gameplay_stage_test.py)
    add_test(NAME oasis_evidence_live_forward_generic_closure_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_generic_closure_stage_test.py)
    add_test(NAME oasis_evidence_live_forward_postrun_coordinator
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_postrun_coordinator_test.py)
    add_test(NAME oasis_evidence_live_forward_postrun_progress
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_postrun_progress_test.py)
    add_test(NAME oasis_evidence_live_forward_postrun_receipts
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_postrun_receipts_test.py)
    add_test(NAME oasis_evidence_live_forward_rolling_master
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_rolling_master_test.py)
    add_test(NAME oasis_evidence_live_forward_rom_link_runtime
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_rom_link_runtime_test.py)
    add_test(NAME oasis_evidence_live_forward_sprite_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_sprite_stage_test.py)
    add_test(NAME oasis_evidence_live_forward_stage5
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_stage5_test.py)
    add_test(NAME oasis_evidence_live_forward_worker_control_launcher
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_worker_control_launcher_test.py)
    add_test(NAME oasis_evidence_live_stage7_stage8_stage9_contract
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_stage7_stage8_stage9_contract_test.py)
    add_test(NAME oasis_evidence_m12_dynamic_sat_shadow_discovery
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_dynamic_sat_shadow_discovery_test.py)
    add_test(NAME oasis_evidence_m12_sprite_evidence_gap
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_evidence_gap_test.py)
    add_test(NAME oasis_evidence_m12_sprite_persistence
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_persistence_test.py)
    add_test(NAME oasis_evidence_m12_sprite_piece_artifact
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_piece_artifact_test.py)
    add_test(NAME oasis_evidence_m12_sprite_piece_catalog
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_piece_catalog_test.py)
    add_test(NAME oasis_evidence_m12_targeted_dynamic_sat_capture
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_targeted_dynamic_sat_capture_test.py)
    add_test(NAME oasis_evidence_master_outcome_view
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_outcome_view_test.py)
    add_test(NAME oasis_evidence_master_startup_authority
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_startup_authority_test.py)
    add_test(NAME oasis_evidence_master_v2_contribution_boundary
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_v2_contribution_boundary_test.py)
    add_test(NAME oasis_evidence_master_v2_forensic_reconciliation
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/master_v2_forensic_reconciliation_test.py)
    add_test(NAME oasis_evidence_stage5_in_memory
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/stage5_in_memory_test.py)
    add_test(NAME oasis_evidence_stage7_decode
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/stage7_decode_test.py)
    add_test(NAME oasis_evidence_stage7_decoder_contract
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/stage7_decoder_contract_test.py)
    add_test(NAME oasis_evidence_stage7_subprocess
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/stage7_subprocess_test.py)
    add_test(NAME oasis_evidence_w3_z80_evidence
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w3_z80_evidence_test.py)
    add_test(NAME oasis_evidence_w4_audio_analysis
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w4_audio_analysis_test.py)
endif()

execute_process(COMMAND ${Python3_EXECUTABLE} -c "import pytest"
                RESULT_VARIABLE OASIS_PYTEST_IMPORT_RESULT
                OUTPUT_QUIET ERROR_QUIET)
if(OASIS_PYTEST_IMPORT_RESULT EQUAL 0)
    add_test(NAME oasis_evidence_live_forward_vdp_stage
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/live_forward_vdp_stage_test.py)
    add_test(NAME oasis_evidence_m12_sprite_frame_artifact
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_frame_artifact_test.py)
    add_test(NAME oasis_evidence_m12_sprite_scanline_raster
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_sprite_scanline_raster_test.py)
    add_test(NAME oasis_evidence_m12_vdp_frame_artifact
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_vdp_frame_artifact_test.py)
    add_test(NAME oasis_evidence_w5_audio_format
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w5_audio_format_test.py)
    add_test(NAME oasis_evidence_w5_audio_functional
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w5_audio_functional_test.py)
    add_test(NAME oasis_evidence_w5_audio_ownership
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w5_audio_ownership_test.py)
endif()

set(OASIS_RUNTIME_ROM "$ENV{BEYOND_OASIS_ROM}")
if(NOT OASIS_RUNTIME_ROM OR NOT EXISTS "${OASIS_RUNTIME_ROM}")
    set(OASIS_RUNTIME_ROM "${CMAKE_SOURCE_DIR}/local-roms/Beyond Oasis (USA).md")
endif()
if(EXISTS "${OASIS_RUNTIME_ROM}")
    add_test(NAME oasis_evidence_m12_dynamic_sat_producer_backtrace
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/m12_dynamic_sat_producer_backtrace_test.py)
endif()

add_test(NAME oasis_evidence_w6_live_discovery
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/w6_live_discovery_test.py)
set_tests_properties(oasis_evidence_w6_live_discovery
                     PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
include(${CMAKE_SOURCE_DIR}/cmake/m14_4.cmake)
