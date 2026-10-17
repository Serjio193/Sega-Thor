add_library(oasis_hybrid_contract STATIC contract.cpp dispatch.cpp replacement.cpp basic_block.cpp
    mechanical_primitive.cpp
    runner_report.cpp
    basic_block_reference.cpp
    interpreter_profile.cpp
    generated_block_runtime.cpp generated_blocks.cpp generated_blocks_m1137.cpp
    generated_blocks_m1138.cpp generated_blocks_m1139.cpp generated_blocks_m1144.cpp
    generated_blocks_m1147.cpp generated_blocks_m1147_registry.cpp
    generated_blocks_m1145_0.cpp generated_blocks_m1145_1.cpp generated_blocks_m1145_2.cpp
    generated_blocks_m1145_3.cpp generated_blocks_m1145_4.cpp generated_blocks_m1145_5.cpp
    generated_blocks_m1145_6.cpp generated_blocks_m1145_7.cpp generated_blocks_m1145_8.cpp
    generated_blocks_m1145_9.cpp generated_blocks_m1145_10.cpp generated_blocks_m1145_11.cpp
    generated_blocks_m1145_12.cpp generated_blocks_m1145_13.cpp generated_blocks_m1145_14.cpp
    generated_blocks_m1145_15.cpp generated_blocks_m1145_16.cpp generated_blocks_m1145_17.cpp
    generated_blocks_m1145_18.cpp generated_blocks_m1145_19.cpp generated_blocks_m1145_20.cpp
    generated_blocks_m1145_21.cpp generated_blocks_m1145_22.cpp generated_blocks_m1145_23.cpp
    generated_blocks_m1145_24.cpp generated_blocks_m1145_25.cpp generated_blocks_m1145_26.cpp
    generated_blocks_m1145_27.cpp generated_blocks_m1145_28.cpp generated_blocks_m1145_29.cpp
    generated_blocks_m1145_30.cpp generated_blocks_m1145_31.cpp
    generated_blocks_m1145_registry_0.cpp generated_blocks_m1145_registry_1.cpp
    generated_blocks_m1145_registry_2.cpp generated_blocks_m1145_registry_3.cpp
    generated_block_registry.cpp
    recomp_generator.cpp checkpoint_evidence.cpp gpgx_checkpoint_layout.cpp address_provenance.cpp runner_support.cpp
    e268_state_transform.cpp m68k_e268_instruction_step.cpp m68k_613b2_instruction_step.cpp
    m68k_61e48_instruction_step.cpp m68k_9d_family_instruction_step.cpp
    m68k_61934_step_ops.cpp m68k_61934_step_moves.cpp m68k_61934_step_tail_ops.cpp m68k_61934_step_tail.cpp m68k_61934_instruction_step.cpp
    m68k_6147e_step_ops.cpp m68k_6147e_step_moves.cpp m68k_6147e_step_math.cpp m68k_6147e_instruction_step.cpp m68k_6147e_table1.cpp m68k_6147e_component_step.cpp m68k_6147e_table2.cpp
    m68k_623ac_step_ops.cpp m68k_623ac_step_moves.cpp m68k_623ac_step_math.cpp m68k_623ac_step_logic.cpp m68k_623ac_instruction_step.cpp m68k_623ac_block1.cpp m68k_623ac_block2.cpp m68k_623ac_subs.cpp m68k_623ac_table.cpp
    m68k_6273a_instruction_step.cpp m68k_62788_instruction_step.cpp
    m68k_60000_step_ops.cpp m68k_60000_step_mem.cpp m68k_60000_step_alu.cpp
    m68k_60000_block1.cpp m68k_60000_block2.cpp m68k_60000_instruction_step.cpp
    m68k_60286_step_ops.cpp m68k_60286_instruction_step.cpp m68k_62ae0_step_ops.cpp m68k_62ae0_instruction_step.cpp
    m68k_626bc_instruction_step.cpp m68k_0027fc_instruction_step.cpp m68k_002992_step_ops.cpp m68k_002992_instruction_step.cpp
    m68k_6138e_instruction_step.cpp m68k_00e682_instruction_step.cpp m68k_0613f8_instruction_step.cpp m68k_016ebc_instruction_step.cpp
    m68k_009ad6_step_slope.cpp
    m68k_009ad6_instruction_step.cpp
    m68k_009bf2_instruction_step.cpp
    m68k_0027a6_instruction_step.cpp
    m68k_00b15e_step_ops.cpp
    m68k_00b15e_step_setup.cpp
    m68k_00b15e_step_piece.cpp
    m68k_00b15e_step_dma.cpp
    m68k_00b15e_instruction_step.cpp
    m68k_00a8da_instruction_step.cpp
    m68k_017d12_step_ops.cpp
    m68k_017d12_step_init.cpp
    m68k_017d12_step_update.cpp
    m68k_017d12_instruction_step.cpp
    m68k_6281a_step_tail.cpp
    m68k_6281a_instruction_step.cpp
    m68k_6282e_instruction_step.cpp
    m68k_01ac90_instruction_step.cpp
    m68k_01b102_step_hit.cpp
    m68k_01b102_instruction_step.cpp
    m68k_00350e_instruction_step.cpp
    m68k_008e22_step_cycle.cpp
    m68k_008e22_instruction_step.cpp
    m68k_00a7e6_step_mode_a.cpp
    m68k_00a7e6_step_mode_b.cpp
    m68k_00a7e6_instruction_step.cpp
    m68k_008e90_step_ops.cpp m68k_008e90_step_scan.cpp m68k_008e90_step_anim.cpp
    m68k_008e90_step_move_x.cpp m68k_008e90_step_move_y.cpp m68k_008e90_step_move_alt.cpp m68k_008e90_instruction_step.cpp
    m68k_00f788_step_ops.cpp m68k_00f788_step_seq.cpp m68k_00f788_instruction_step.cpp
    m68k_00a196_step_ops.cpp m68k_00a196_step_seq.cpp m68k_00a196_instruction_step.cpp
    m68k_00a342_step_ops.cpp m68k_00a342_step_seq.cpp m68k_00a342_instruction_step.cpp
    m68k_00b34c_step_ops.cpp m68k_00b34c_step_seq.cpp m68k_00b34c_instruction_step.cpp
    m68k_00c980_step_ops.cpp m68k_00c980_instruction_step.cpp
    m68k_00c998_step_ops.cpp m68k_00c998_instruction_step.cpp
    m68k_008dd4_step_ops.cpp m68k_008dd4_instruction_step.cpp
    m68k_00557a_step_ops.cpp m68k_00557a_step_block1.cpp m68k_00557a_step_block2.cpp m68k_00557a_instruction_step.cpp
    m68k_005670_step_ops1.cpp m68k_005670_step_ops2.cpp m68k_005670_step_block1.cpp m68k_005670_step_block2.cpp m68k_005670_step_block3.cpp m68k_005670_instruction_step.cpp
    coordinate_bound_model.cpp
    negative_bound_model.cpp
    positive_bound_model.cpp
    subtraction_bound_model.cpp
    native_island_runtime.cpp
    hybrid_registry.cpp player_dispatch_region.cpp first_divergence_oracle.cpp main_loop_tick_region.cpp entity_oscillation_region.cpp
    caller_attribution.cpp caller_continuation.cpp a5_lifetime_observer.cpp callee_62ae0_observer.cpp callee_61934_observer.cpp callee_623ac_observer.cpp a5_lifetime_runtime.cpp candidate_2d66.cpp candidate_604bc.cpp candidate_parent_suffix.cpp candidate_61032.cpp)
target_include_directories(oasis_hybrid_contract PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(oasis_hybrid_contract PUBLIC oasis_re_tooling oasis_core)

add_executable(oasis_hybrid_contract_test ${PROJECT_SOURCE_DIR}/tests/hybrid_contract_test.cpp)
target_link_libraries(oasis_hybrid_contract_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_contract COMMAND oasis_hybrid_contract_test)
add_executable(oasis_gameplay_session_test gameplay_session.cpp gameplay_session_identity.cpp
    gameplay_session_receipts.cpp
    grid_query_native_model.cpp m68k_call_census.cpp
    ${PROJECT_SOURCE_DIR}/tests/gameplay_session_test.cpp)
target_include_directories(oasis_gameplay_session_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(oasis_gameplay_session_test PRIVATE oasis_core)
add_test(NAME oasis_gameplay_session COMMAND oasis_gameplay_session_test)
add_executable(oasis_native_island_runtime_test native_island_runtime.cpp subtraction_bound_model.cpp
    ${PROJECT_SOURCE_DIR}/tests/native_island_runtime_test.cpp)
target_include_directories(oasis_native_island_runtime_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME oasis_native_island_runtime COMMAND oasis_native_island_runtime_test)
add_executable(oasis_hybrid_dispatch_test ${PROJECT_SOURCE_DIR}/tests/hybrid_dispatch_test.cpp)
target_link_libraries(oasis_hybrid_dispatch_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_dispatch COMMAND oasis_hybrid_dispatch_test)
add_executable(oasis_hybrid_candidate_2d66_test ${PROJECT_SOURCE_DIR}/tests/hybrid_candidate_2d66_test.cpp)
target_link_libraries(oasis_hybrid_candidate_2d66_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_candidate_2d66 COMMAND oasis_hybrid_candidate_2d66_test)
add_executable(oasis_hybrid_candidate_604bc_test ${PROJECT_SOURCE_DIR}/tests/hybrid_candidate_604bc_test.cpp)
target_link_libraries(oasis_hybrid_candidate_604bc_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_candidate_604bc COMMAND oasis_hybrid_candidate_604bc_test)
add_executable(oasis_hybrid_e268_state_transform_test ${PROJECT_SOURCE_DIR}/tests/hybrid_e268_state_transform_test.cpp)
target_link_libraries(oasis_hybrid_e268_state_transform_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_e268_state_transform COMMAND oasis_hybrid_e268_state_transform_test)
add_executable(oasis_hybrid_coordinate_bound_test ${PROJECT_SOURCE_DIR}/tests/coordinate_bound_model_test.cpp)
target_link_libraries(oasis_hybrid_coordinate_bound_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_coordinate_bound COMMAND oasis_hybrid_coordinate_bound_test)
add_executable(oasis_hybrid_negative_bound_test ${PROJECT_SOURCE_DIR}/tests/negative_bound_model_test.cpp)
target_link_libraries(oasis_hybrid_negative_bound_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_negative_bound COMMAND oasis_hybrid_negative_bound_test)
add_executable(oasis_hybrid_positive_bound_test ${PROJECT_SOURCE_DIR}/tests/positive_bound_model_test.cpp)
target_link_libraries(oasis_hybrid_positive_bound_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_positive_bound COMMAND oasis_hybrid_positive_bound_test)
add_executable(oasis_hybrid_subtraction_bound_test ${PROJECT_SOURCE_DIR}/tests/subtraction_bound_model_test.cpp)
target_link_libraries(oasis_hybrid_subtraction_bound_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_subtraction_bound COMMAND oasis_hybrid_subtraction_bound_test)
add_executable(oasis_hybrid_replacement_test ${PROJECT_SOURCE_DIR}/tests/hybrid_replacement_test.cpp)
target_link_libraries(oasis_hybrid_replacement_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_replacement COMMAND oasis_hybrid_replacement_test)
add_executable(oasis_hybrid_registry_test ${PROJECT_SOURCE_DIR}/tests/hybrid_registry_test.cpp)
target_link_libraries(oasis_hybrid_registry_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_registry COMMAND oasis_hybrid_registry_test)
add_executable(oasis_hybrid_recomp_generator_test ${PROJECT_SOURCE_DIR}/tests/hybrid_recomp_generator_test.cpp)
target_link_libraries(oasis_hybrid_recomp_generator_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_recomp_generator COMMAND oasis_hybrid_recomp_generator_test)
add_executable(oasis_hybrid_generated_block_test ${PROJECT_SOURCE_DIR}/tests/hybrid_generated_block_test.cpp)
target_link_libraries(oasis_hybrid_generated_block_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_generated_block COMMAND oasis_hybrid_generated_block_test)
add_executable(oasis_m68k_61e48_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_61e48_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_61e48_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_61e48_step COMMAND oasis_m68k_61e48_step_test)
add_executable(oasis_m68k_61934_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_61934_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_61934_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_61934_step COMMAND oasis_m68k_61934_step_test)
add_executable(oasis_m68k_6147e_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_6147e_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_6147e_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_6147e_step COMMAND oasis_m68k_6147e_step_test)
add_executable(oasis_m68k_623ac_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_623ac_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_623ac_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_623ac_step COMMAND oasis_m68k_623ac_step_test)
add_executable(oasis_m68k_6273a_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_6273a_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_6273a_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_6273a_step COMMAND oasis_m68k_6273a_step_test)
add_executable(oasis_m68k_62788_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_62788_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_62788_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_62788_step COMMAND oasis_m68k_62788_step_test)
add_executable(oasis_m68k_60000_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_60000_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_60000_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_60000_step COMMAND oasis_m68k_60000_step_test)
add_executable(oasis_m68k_60286_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_60286_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_60286_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_60286_step COMMAND oasis_m68k_60286_step_test)
add_executable(oasis_m68k_62ae0_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_62ae0_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_62ae0_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_62ae0_step COMMAND oasis_m68k_62ae0_step_test)
add_executable(oasis_m68k_626bc_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_626bc_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_626bc_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_626bc_step COMMAND oasis_m68k_626bc_step_test)
add_executable(oasis_m68k_0027fc_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_0027fc_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_0027fc_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_0027fc_step COMMAND oasis_m68k_0027fc_step_test)
add_executable(oasis_m68k_002992_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_002992_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_002992_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_002992_step COMMAND oasis_m68k_002992_step_test)
add_executable(oasis_m68k_6138e_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_6138e_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_6138e_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_6138e_step COMMAND oasis_m68k_6138e_step_test)
add_executable(oasis_m68k_00e682_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00e682_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00e682_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00e682_step COMMAND oasis_m68k_00e682_step_test)
add_executable(oasis_m68k_0613f8_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_0613f8_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_0613f8_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_0613f8_step COMMAND oasis_m68k_0613f8_step_test)
add_executable(oasis_m68k_016ebc_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_016ebc_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_016ebc_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_016ebc_step COMMAND oasis_m68k_016ebc_step_test)
add_executable(oasis_m68k_009ad6_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_009ad6_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_009ad6_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_009ad6_step COMMAND oasis_m68k_009ad6_step_test)
add_executable(oasis_m68k_009bf2_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_009bf2_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_009bf2_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_009bf2_step COMMAND oasis_m68k_009bf2_step_test)
add_executable(oasis_m68k_0027a6_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_0027a6_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_0027a6_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_0027a6_step COMMAND oasis_m68k_0027a6_step_test)
add_executable(oasis_m68k_00b15e_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00b15e_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00b15e_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00b15e_step COMMAND oasis_m68k_00b15e_step_test)
add_executable(oasis_m68k_00a8da_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00a8da_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00a8da_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00a8da_step COMMAND oasis_m68k_00a8da_step_test)
add_executable(oasis_m68k_017d12_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_017d12_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_017d12_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_017d12_step COMMAND oasis_m68k_017d12_step_test)
add_executable(oasis_m68k_6281a_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_6281a_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_6281a_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_6281a_step COMMAND oasis_m68k_6281a_step_test)
add_executable(oasis_m68k_6282e_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_6282e_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_6282e_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_6282e_step COMMAND oasis_m68k_6282e_step_test)
add_executable(oasis_m68k_01ac90_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_01ac90_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_01ac90_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_01ac90_step COMMAND oasis_m68k_01ac90_step_test)
add_executable(oasis_m68k_01b102_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_01b102_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_01b102_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_01b102_step COMMAND oasis_m68k_01b102_step_test)
add_executable(oasis_m68k_00350e_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00350e_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00350e_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00350e_step COMMAND oasis_m68k_00350e_step_test)
add_executable(oasis_m68k_008e22_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_008e22_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_008e22_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_008e22_step COMMAND oasis_m68k_008e22_step_test)
add_executable(oasis_m68k_00a7e6_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00a7e6_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00a7e6_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00a7e6_step COMMAND oasis_m68k_00a7e6_step_test)
add_executable(oasis_m68k_008e90_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_008e90_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_008e90_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_008e90_step COMMAND oasis_m68k_008e90_step_test)
add_executable(oasis_m68k_00f788_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00f788_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00f788_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00f788_step COMMAND oasis_m68k_00f788_step_test)
add_executable(oasis_m68k_00a196_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00a196_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00a196_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00a196_step COMMAND oasis_m68k_00a196_step_test)
add_executable(oasis_m68k_00a342_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00a342_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00a342_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00a342_step COMMAND oasis_m68k_00a342_step_test)
add_executable(oasis_m68k_00b34c_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00b34c_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00b34c_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00b34c_step COMMAND oasis_m68k_00b34c_step_test)
add_executable(oasis_m68k_00c980_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00c980_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00c980_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00c980_step COMMAND oasis_m68k_00c980_step_test)
add_executable(oasis_m68k_00c998_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00c998_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00c998_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00c998_step COMMAND oasis_m68k_00c998_step_test)
add_executable(oasis_m68k_008dd4_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_008dd4_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_008dd4_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_008dd4_step COMMAND oasis_m68k_008dd4_step_test)
add_executable(oasis_m68k_00557a_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_00557a_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_00557a_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_00557a_step COMMAND oasis_m68k_00557a_step_test)
add_executable(oasis_m68k_005670_step_test ${PROJECT_SOURCE_DIR}/tests/m68k_005670_instruction_step_test.cpp)
target_link_libraries(oasis_m68k_005670_step_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_m68k_005670_step COMMAND oasis_m68k_005670_step_test)

add_executable(oasis_hybrid_basic_block_test ${PROJECT_SOURCE_DIR}/tests/hybrid_basic_block_test.cpp)
target_link_libraries(oasis_hybrid_basic_block_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_basic_block COMMAND oasis_hybrid_basic_block_test)
add_executable(oasis_hybrid_generated_provenance_test ${PROJECT_SOURCE_DIR}/tests/hybrid_generated_provenance_test.cpp)
target_link_libraries(oasis_hybrid_generated_provenance_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_generated_provenance COMMAND oasis_hybrid_generated_provenance_test)
add_executable(oasis_hybrid_semantic_core_test ${PROJECT_SOURCE_DIR}/tests/hybrid_semantic_core_test.cpp)
target_link_libraries(oasis_hybrid_semantic_core_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_semantic_core COMMAND oasis_hybrid_semantic_core_test)
add_executable(oasis_hybrid_m1145_semantic_test ${PROJECT_SOURCE_DIR}/tests/hybrid_m1145_semantic_test.cpp)
target_link_libraries(oasis_hybrid_m1145_semantic_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_m1145_semantic COMMAND oasis_hybrid_m1145_semantic_test)
add_executable(oasis_hybrid_m1147_semantic_test ${PROJECT_SOURCE_DIR}/tests/hybrid_m1147_semantic_test.cpp)
target_link_libraries(oasis_hybrid_m1147_semantic_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_m1147_semantic COMMAND oasis_hybrid_m1147_semantic_test)
add_executable(oasis_hybrid_mechanical_primitive_test ${PROJECT_SOURCE_DIR}/tests/hybrid_mechanical_primitive_test.cpp)
target_link_libraries(oasis_hybrid_mechanical_primitive_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_mechanical_primitive COMMAND oasis_hybrid_mechanical_primitive_test)
add_executable(oasis_hybrid_checkpoint_evidence_test ${PROJECT_SOURCE_DIR}/tests/hybrid_checkpoint_evidence_test.cpp)
target_link_libraries(oasis_hybrid_checkpoint_evidence_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_checkpoint_evidence COMMAND oasis_hybrid_checkpoint_evidence_test)
add_executable(oasis_hybrid_checkpoint_layout_test ${PROJECT_SOURCE_DIR}/tests/hybrid_checkpoint_layout_test.cpp)
target_link_libraries(oasis_hybrid_checkpoint_layout_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_checkpoint_layout COMMAND oasis_hybrid_checkpoint_layout_test)
add_executable(oasis_hybrid_address_provenance_test ${PROJECT_SOURCE_DIR}/tests/hybrid_address_provenance_test.cpp)
target_link_libraries(oasis_hybrid_address_provenance_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_address_provenance COMMAND oasis_hybrid_address_provenance_test)
add_executable(oasis_hybrid_caller_attribution_test ${PROJECT_SOURCE_DIR}/tests/hybrid_caller_attribution_test.cpp)
target_link_libraries(oasis_hybrid_caller_attribution_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_caller_attribution COMMAND oasis_hybrid_caller_attribution_test)
add_executable(oasis_hybrid_caller_continuation_test ${PROJECT_SOURCE_DIR}/tests/hybrid_caller_continuation_test.cpp)
target_link_libraries(oasis_hybrid_caller_continuation_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_caller_continuation COMMAND oasis_hybrid_caller_continuation_test)
add_executable(oasis_hybrid_a5_lifetime_test ${PROJECT_SOURCE_DIR}/tests/hybrid_a5_lifetime_test.cpp)
target_link_libraries(oasis_hybrid_a5_lifetime_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_a5_lifetime COMMAND oasis_hybrid_a5_lifetime_test)
add_executable(oasis_hybrid_callee_62ae0_test ${PROJECT_SOURCE_DIR}/tests/hybrid_callee_62ae0_test.cpp)
target_link_libraries(oasis_hybrid_callee_62ae0_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_callee_62ae0 COMMAND oasis_hybrid_callee_62ae0_test)
add_executable(oasis_hybrid_callee_61934_test ${PROJECT_SOURCE_DIR}/tests/hybrid_callee_61934_test.cpp)
target_link_libraries(oasis_hybrid_callee_61934_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_callee_61934 COMMAND oasis_hybrid_callee_61934_test)
add_executable(oasis_hybrid_callee_623ac_test ${PROJECT_SOURCE_DIR}/tests/hybrid_callee_623ac_test.cpp)
target_link_libraries(oasis_hybrid_callee_623ac_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_callee_623ac COMMAND oasis_hybrid_callee_623ac_test)
add_executable(oasis_parent_suffix_test ${PROJECT_SOURCE_DIR}/tests/parent_suffix_test.cpp)
target_link_libraries(oasis_parent_suffix_test PRIVATE oasis_core)
add_test(NAME oasis_parent_suffix COMMAND oasis_parent_suffix_test)
target_compile_options(oasis_hybrid_checkpoint_layout_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
add_executable(oasis_hybrid_recomp_generate recomp_generator_report.cpp)
target_link_libraries(oasis_hybrid_recomp_generate PRIVATE oasis_hybrid_contract)
add_executable(oasis_hybrid_interpreter_ledger interpreter_ledger.cpp)
target_link_libraries(oasis_hybrid_interpreter_ledger PRIVATE oasis_hybrid_contract)
target_compile_options(oasis_hybrid_recomp_generator_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_generated_block_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_generated_provenance_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_semantic_core_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_checkpoint_evidence_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set_tests_properties(oasis_hybrid_contract oasis_hybrid_dispatch oasis_hybrid_candidate_2d66 oasis_hybrid_candidate_604bc oasis_hybrid_replacement oasis_hybrid_recomp_generator oasis_hybrid_generated_block oasis_hybrid_basic_block oasis_hybrid_generated_provenance oasis_hybrid_semantic_core oasis_hybrid_m1145_semantic oasis_hybrid_m1147_semantic oasis_hybrid_mechanical_primitive oasis_hybrid_checkpoint_evidence oasis_hybrid_checkpoint_layout oasis_hybrid_address_provenance oasis_hybrid_caller_attribution
        oasis_hybrid_caller_continuation oasis_hybrid_a5_lifetime oasis_hybrid_callee_62ae0 oasis_hybrid_callee_61934 oasis_hybrid_callee_623ac PROPERTIES ENVIRONMENT "PATH=${compiler_bin}")
endif()

set(OASIS_HYBRID_GPGX_SOURCE "" CACHE PATH "External GPGX source for developer-only PoC")
if(OASIS_HYBRID_GPGX_SOURCE)
    add_executable(oasis_hybrid_scheduler_fixture scheduler_fixture_runner.cpp)
    target_include_directories(oasis_hybrid_scheduler_fixture PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${OASIS_HYBRID_GPGX_SOURCE}/core
        ${OASIS_HYBRID_GPGX_SOURCE}/core/debug
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_hybrid_scheduler_fixture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_hybrid_poc runner.cpp)
    target_include_directories(oasis_hybrid_poc PRIVATE
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_hybrid_poc PRIVATE oasis_hybrid_contract ${CMAKE_DL_LIBS})

    add_executable(oasis_grid_query_capture
        grid_query_capture.cpp
        grid_query_micro_oracle.cpp)
    target_include_directories(oasis_grid_query_capture PRIVATE
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_grid_query_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_grid_query_natural_capture grid_query_natural_capture.cpp)
    target_include_directories(oasis_grid_query_natural_capture PRIVATE
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_grid_query_natural_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_gameplay_session gameplay_session.cpp gameplay_session_identity.cpp
        gameplay_session_receipts.cpp gameplay_session_landmarks.cpp gameplay_session_input_sources.cpp
        gameplay_session_call_capture.cpp gameplay_session_replay_options.cpp
        gameplay_session_metrics.cpp
        gameplay_session_step_trace.cpp
        m68k_9d70_instruction_step.cpp
        m68k_94a2_instruction_step.cpp
        m68k_9c40_instruction_step.cpp
        m68k_8ef8_instruction_step.cpp
        m68k_e268_instruction_step.cpp
        m68k_613b2_instruction_step.cpp
        m68k_61e48_instruction_step.cpp
        m68k_9d_family_instruction_step.cpp
        m68k_61934_step_ops.cpp
        m68k_61934_step_moves.cpp
        m68k_61934_step_tail_ops.cpp
        m68k_61934_step_tail.cpp
        m68k_61934_instruction_step.cpp
        m68k_6147e_step_ops.cpp
        m68k_6147e_step_moves.cpp
        m68k_6147e_step_math.cpp
        m68k_6147e_instruction_step.cpp
        m68k_6147e_table1.cpp
        m68k_6147e_component_step.cpp
        m68k_6147e_table2.cpp
        m68k_623ac_step_ops.cpp
        m68k_623ac_step_moves.cpp
        m68k_623ac_step_math.cpp
        m68k_623ac_step_logic.cpp
        m68k_623ac_instruction_step.cpp
        m68k_623ac_block1.cpp
        m68k_623ac_block2.cpp
        m68k_623ac_subs.cpp
        m68k_623ac_table.cpp
        m68k_6273a_instruction_step.cpp m68k_62788_instruction_step.cpp
        m68k_60000_step_ops.cpp m68k_60000_step_mem.cpp m68k_60000_step_alu.cpp
        m68k_60000_block1.cpp m68k_60000_block2.cpp m68k_60000_instruction_step.cpp
        m68k_60286_step_ops.cpp m68k_60286_instruction_step.cpp m68k_62ae0_step_ops.cpp m68k_62ae0_instruction_step.cpp
        m68k_626bc_instruction_step.cpp m68k_0027fc_instruction_step.cpp m68k_002992_step_ops.cpp m68k_002992_instruction_step.cpp
        m68k_6138e_instruction_step.cpp m68k_00e682_instruction_step.cpp m68k_0613f8_instruction_step.cpp m68k_016ebc_instruction_step.cpp
        m68k_009ad6_step_slope.cpp m68k_009ad6_instruction_step.cpp m68k_009bf2_instruction_step.cpp m68k_0027a6_instruction_step.cpp
        m68k_00b15e_step_ops.cpp m68k_00b15e_step_setup.cpp m68k_00b15e_step_piece.cpp m68k_00b15e_step_dma.cpp m68k_00b15e_instruction_step.cpp
        m68k_00a8da_instruction_step.cpp m68k_017d12_step_ops.cpp m68k_017d12_step_init.cpp m68k_017d12_step_update.cpp m68k_017d12_instruction_step.cpp
        m68k_6281a_step_tail.cpp m68k_6281a_instruction_step.cpp m68k_6282e_instruction_step.cpp
        m68k_01ac90_instruction_step.cpp
        m68k_01b102_step_hit.cpp
        m68k_01b102_instruction_step.cpp
        m68k_00350e_instruction_step.cpp
        m68k_008e22_step_cycle.cpp
        m68k_008e22_instruction_step.cpp
        m68k_00a7e6_step_mode_a.cpp m68k_00a7e6_step_mode_b.cpp m68k_00a7e6_instruction_step.cpp
        m68k_008e90_step_ops.cpp m68k_008e90_step_scan.cpp m68k_008e90_step_anim.cpp
        m68k_008e90_step_move_x.cpp m68k_008e90_step_move_y.cpp m68k_008e90_step_move_alt.cpp m68k_008e90_instruction_step.cpp
        m68k_00f788_step_ops.cpp m68k_00f788_step_seq.cpp m68k_00f788_instruction_step.cpp
        m68k_00a196_step_ops.cpp m68k_00a196_step_seq.cpp m68k_00a196_instruction_step.cpp
        m68k_00a342_step_ops.cpp m68k_00a342_step_seq.cpp m68k_00a342_instruction_step.cpp
        m68k_00b34c_step_ops.cpp m68k_00b34c_step_seq.cpp m68k_00b34c_instruction_step.cpp
        m68k_00c980_step_ops.cpp m68k_00c980_instruction_step.cpp
        m68k_00c998_step_ops.cpp m68k_00c998_instruction_step.cpp
        m68k_008dd4_step_ops.cpp m68k_008dd4_instruction_step.cpp
        m68k_00557a_step_ops.cpp m68k_00557a_step_block1.cpp m68k_00557a_step_block2.cpp m68k_00557a_instruction_step.cpp
        m68k_005670_step_ops1.cpp m68k_005670_step_ops2.cpp m68k_005670_step_block1.cpp m68k_005670_step_block2.cpp m68k_005670_step_block3.cpp m68k_005670_instruction_step.cpp
        m68k_gameplay_step_set.cpp
        gameplay_session_cpu.cpp gameplay_session_replay_diagnostic.cpp replay_equivalence_trace.cpp
        checkpoint_evidence.cpp gpgx_checkpoint_layout.cpp
        grid_query_native_model.cpp
        m68k_call_census.cpp
        native_island_runtime.cpp subtraction_bound_model.cpp
        hybrid_registry.cpp player_dispatch_region.cpp first_divergence_oracle.cpp main_loop_tick_region.cpp
        entity_oscillation_region.cpp
        gameplay_session_runner.cpp)
    target_include_directories(oasis_gameplay_session PRIVATE
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_gameplay_session PRIVATE oasis_core ${CMAKE_DL_LIBS})
    add_executable(oasis_gameplay_session_input_sources_test gameplay_session_input_sources.cpp
        ${PROJECT_SOURCE_DIR}/tests/gameplay_session_input_sources_test.cpp)
    target_include_directories(oasis_gameplay_session_input_sources_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    add_test(NAME oasis_gameplay_session_input_sources COMMAND oasis_gameplay_session_input_sources_test)

    add_executable(oasis_grid_footprint_capture grid_footprint_capture.cpp grid_query_micro_oracle.cpp)
    target_include_directories(oasis_grid_footprint_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_grid_footprint_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_grid_leaf_capture grid_leaf_capture.cpp grid_query_micro_oracle.cpp)
    target_include_directories(oasis_grid_leaf_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_grid_leaf_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_coordinate_bound_oracle coordinate_bound_oracle.cpp coordinate_bound_model.cpp
        negative_bound_model.cpp positive_bound_model.cpp grid_query_micro_oracle.cpp)
    target_include_directories(oasis_coordinate_bound_oracle PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_coordinate_bound_oracle PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_subtraction_bound_oracle subtraction_bound_oracle.cpp subtraction_bound_model.cpp)
    target_include_directories(oasis_subtraction_bound_oracle PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_subtraction_bound_oracle PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_b730_branch_capture b730_branch_capture.cpp grid_query_micro_oracle.cpp)
    target_include_directories(oasis_b730_branch_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_b730_branch_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_raw_record_capture raw_record_capture.cpp)
    target_include_directories(oasis_raw_record_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_raw_record_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_sparse_word_store_capture sparse_word_store_capture.cpp)
    target_include_directories(oasis_sparse_word_store_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_sparse_word_store_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_transfer_provenance_capture transfer_provenance_capture.cpp)
    target_include_directories(oasis_transfer_provenance_capture PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_transfer_provenance_capture PRIVATE oasis_core ${CMAKE_DL_LIBS})

    add_executable(oasis_raw_record_live_dispatch raw_record_live_dispatch.cpp)
    target_include_directories(oasis_raw_record_live_dispatch PRIVATE ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_raw_record_live_dispatch PRIVATE oasis_core ${CMAKE_DL_LIBS})

endif()

add_executable(oasis_gameplay_session_replay_diagnostic_test gameplay_session_replay_diagnostic.cpp
    ${PROJECT_SOURCE_DIR}/tests/gameplay_session_replay_diagnostic_test.cpp)
target_include_directories(oasis_gameplay_session_replay_diagnostic_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME oasis_gameplay_session_replay_diagnostic COMMAND oasis_gameplay_session_replay_diagnostic_test)
