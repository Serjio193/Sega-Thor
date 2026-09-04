cmake_minimum_required(VERSION 3.20)
project(sega_thor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(oasis_core
    src/core/rom.cpp
    src/core/rom_identity.cpp
    src/core/runtime.cpp
    src/genesis/memory_bus.cpp
    src/genesis/vdp.cpp
    src/game/translated_routines.cpp
    src/game/graphics_decompress.cpp
    src/game/genesis_graphics.cpp
    src/game/scripts/event_router.cpp
    src/game/entities/entity_pool.cpp
    src/game/spirits/spirit_slots.cpp
    src/game/player/player.cpp
    src/game/world/byte_grid.cpp
    src/game/world/screen_descriptor.cpp
    src/game/world/terrain_collision.cpp
)

target_include_directories(oasis_core PUBLIC src)

add_executable(oasis src/main.cpp)
target_link_libraries(oasis PRIVATE oasis_core)

add_executable(oasis_inspect src/tools/asset_inspector.cpp)
target_link_libraries(oasis_inspect PRIVATE oasis_core)

# Developer-only reverse-engineering tooling; it is not linked into oasis_core.
add_library(oasis_re_tooling
    src/tools/re_slice_decoder.cpp
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

enable_testing()

add_executable(oasis_smoke tests/smoke.cpp)
target_link_libraries(oasis_smoke PRIVATE oasis_core)
add_test(NAME oasis_smoke COMMAND oasis_smoke)

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

# These executables use a locally supplied verified ROM and are intentionally not CTest tests.
add_executable(oasis_graphics_reference tests/graphics_decompress_reference.cpp)
target_link_libraries(oasis_graphics_reference PRIVATE oasis_core)

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
    oasis_smoke
    oasis_rom_identity_test
    oasis_graphics_decompress_test
    oasis_genesis_graphics_test
    oasis_vdp_test
    oasis_runtime_test
    oasis_screen_descriptor_test
    oasis_byte_grid_test
    oasis_terrain_collision_test
    oasis_player_test
    oasis_entity_pool_test
    oasis_spirit_slots_test
    oasis_event_router_test
    oasis_re_slice_decoder_test
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
        oasis_smoke
        oasis_rom_identity
        oasis_graphics_decompress
        oasis_genesis_graphics
        oasis_vdp
        oasis_runtime
        oasis_screen_descriptor
        oasis_byte_grid
        oasis_terrain_collision
        oasis_player
        oasis_entity_pool
        oasis_spirit_slots
        oasis_event_router
        oasis_re_slice_decoder
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
        project_file_line_limit
    )
    set_tests_properties(${oasis_runtime_test_names}
        PROPERTIES ENVIRONMENT "PATH=${oasis_compiler_bin}"
    )
endif()
