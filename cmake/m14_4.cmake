target_sources(oasis_re_tooling PRIVATE
    ${CMAKE_SOURCE_DIR}/src/tools/re_slice_decoder_absolute.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/re_cfg_closure.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/re_canonical_rom_bytes.cpp
    ${CMAKE_SOURCE_DIR}/src/tools/re_static_xref_scan.cpp)

add_executable(oasis_re_m14_4_cfg ${CMAKE_SOURCE_DIR}/src/tools/re_m14_4_cfg_report.cpp)
target_link_libraries(oasis_re_m14_4_cfg PRIVATE oasis_re_tooling oasis_core)
add_executable(oasis_re_cfg_closure_test ${CMAKE_SOURCE_DIR}/tests/re_cfg_closure_test.cpp)
target_link_libraries(oasis_re_cfg_closure_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_cfg_closure COMMAND oasis_re_cfg_closure_test)
add_executable(oasis_re_static_xref_scan_test ${CMAKE_SOURCE_DIR}/tests/re_static_xref_scan_test.cpp)
target_link_libraries(oasis_re_static_xref_scan_test PRIVATE oasis_re_tooling)
add_test(NAME oasis_re_static_xref_scan COMMAND oasis_re_static_xref_scan_test)
add_executable(oasis_re_m14_7_static_xrefs ${CMAKE_SOURCE_DIR}/src/tools/re_m14_7_static_xrefs.cpp)
target_link_libraries(oasis_re_m14_7_static_xrefs PRIVATE oasis_re_tooling oasis_core)
if(Python3_Interpreter_FOUND)
    add_test(NAME oasis_m14_4_generic_asm_closure
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_generic_asm_closure_test.py)
    add_test(NAME oasis_m14_5_exact_asm_map_adoption
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_map_adoption_test.py)
    add_test(NAME oasis_m14_6_stage7_source_ownership_closure
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_stage7_closure_test.py)
    add_test(NAME oasis_m14_7_static_entry_admission
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/rom_knowledge_static_entry_test.py)
endif()
