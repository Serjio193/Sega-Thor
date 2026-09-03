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
    src/game/world/byte_grid.cpp
    src/game/world/screen_descriptor.cpp
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

# These executables use a locally supplied verified ROM and are intentionally not CTest tests.
add_executable(oasis_graphics_reference tests/graphics_decompress_reference.cpp)
target_link_libraries(oasis_graphics_reference PRIVATE oasis_core)

add_executable(oasis_screen_descriptor_reference tests/screen_descriptor_reference.cpp)
target_link_libraries(oasis_screen_descriptor_reference PRIVATE oasis_core)

add_test(
    NAME project_file_line_limit
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_file_limits.cmake
)
