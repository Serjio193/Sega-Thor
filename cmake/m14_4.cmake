target_sources(oasis_re_tooling PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tools/re_cfg_closure.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/re_canonical_rom_bytes.cpp)

add_executable(oasis_re_m14_4_cfg ${CMAKE_SOURCE_DIR}/src/tools/re_m14_4_cfg_report.cpp)
target_link_libraries(oasis_re_m14_4_cfg PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_cfg_closure_test ${CMAKE_SOURCE_DIR}/tests/re_cfg_closure_test.cpp)
target_link_libraries(oasis_re_cfg_closure_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_cfg_closure COMMAND oasis_re_cfg_closure_test)
if(Python3_Interpreter_FOUND)
    add_test(NAME oasis_m14_4_generic_asm_closure
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_generic_asm_closure_test.py)
    add_test(NAME oasis_m14_5_exact_asm_map_adoption
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_map_adoption_test.py)
    add_test(NAME oasis_m14_6_stage7_source_ownership_closure
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_stage7_closure_test.py)
endif()
