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
        project_file_line_limit
    )
    set_tests_properties(${oasis_runtime_test_names}
        PROPERTIES ENVIRONMENT "PATH=${oasis_compiler_bin}"
    )
endif()
