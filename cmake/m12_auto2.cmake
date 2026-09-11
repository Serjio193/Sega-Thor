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
    add_test(NAME oasis_re_m12_z80_helpers
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/re_m12_z80_promote_test.py)
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
endif()
