add_library(oasis_hybrid_contract STATIC contract.cpp dispatch.cpp replacement.cpp basic_block.cpp
    basic_block_reference.cpp
    interpreter_profile.cpp
    generated_block_runtime.cpp generated_blocks.cpp generated_blocks_m1137.cpp
    generated_blocks_m1138.cpp generated_blocks_m1139.cpp generated_block_registry.cpp
    recomp_generator.cpp
    candidate_2d66.cpp candidate_604bc.cpp candidate_61032.cpp)
target_include_directories(oasis_hybrid_contract PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(oasis_hybrid_contract PUBLIC oasis_re_tooling oasis_core)

add_executable(oasis_hybrid_contract_test ${PROJECT_SOURCE_DIR}/tests/hybrid_contract_test.cpp)
target_link_libraries(oasis_hybrid_contract_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_contract COMMAND oasis_hybrid_contract_test)
add_executable(oasis_hybrid_dispatch_test ${PROJECT_SOURCE_DIR}/tests/hybrid_dispatch_test.cpp)
target_link_libraries(oasis_hybrid_dispatch_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_dispatch COMMAND oasis_hybrid_dispatch_test)
add_executable(oasis_hybrid_candidate_2d66_test ${PROJECT_SOURCE_DIR}/tests/hybrid_candidate_2d66_test.cpp)
target_link_libraries(oasis_hybrid_candidate_2d66_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_candidate_2d66 COMMAND oasis_hybrid_candidate_2d66_test)
add_executable(oasis_hybrid_replacement_test ${PROJECT_SOURCE_DIR}/tests/hybrid_replacement_test.cpp)
target_link_libraries(oasis_hybrid_replacement_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_replacement COMMAND oasis_hybrid_replacement_test)
add_executable(oasis_hybrid_recomp_generator_test ${PROJECT_SOURCE_DIR}/tests/hybrid_recomp_generator_test.cpp)
target_link_libraries(oasis_hybrid_recomp_generator_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_recomp_generator COMMAND oasis_hybrid_recomp_generator_test)
add_executable(oasis_hybrid_generated_block_test ${PROJECT_SOURCE_DIR}/tests/hybrid_generated_block_test.cpp)
target_link_libraries(oasis_hybrid_generated_block_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_generated_block COMMAND oasis_hybrid_generated_block_test)

add_executable(oasis_hybrid_basic_block_test ${PROJECT_SOURCE_DIR}/tests/hybrid_basic_block_test.cpp)
target_link_libraries(oasis_hybrid_basic_block_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_basic_block COMMAND oasis_hybrid_basic_block_test)
add_executable(oasis_hybrid_generated_provenance_test ${PROJECT_SOURCE_DIR}/tests/hybrid_generated_provenance_test.cpp)
target_link_libraries(oasis_hybrid_generated_provenance_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_generated_provenance COMMAND oasis_hybrid_generated_provenance_test)
add_executable(oasis_hybrid_semantic_core_test ${PROJECT_SOURCE_DIR}/tests/hybrid_semantic_core_test.cpp)
target_link_libraries(oasis_hybrid_semantic_core_test PRIVATE oasis_hybrid_contract)
add_test(NAME oasis_hybrid_semantic_core COMMAND oasis_hybrid_semantic_core_test)
add_executable(oasis_hybrid_recomp_generate recomp_generator_report.cpp)
target_link_libraries(oasis_hybrid_recomp_generate PRIVATE oasis_hybrid_contract)
target_compile_options(oasis_hybrid_recomp_generator_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_generated_block_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_generated_provenance_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
target_compile_options(oasis_hybrid_semantic_core_test PRIVATE $<$<CONFIG:Release>:-UNDEBUG>)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    get_filename_component(compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set_tests_properties(oasis_hybrid_contract oasis_hybrid_dispatch oasis_hybrid_candidate_2d66 oasis_hybrid_replacement oasis_hybrid_recomp_generator oasis_hybrid_generated_block oasis_hybrid_basic_block oasis_hybrid_generated_provenance oasis_hybrid_semantic_core
        PROPERTIES ENVIRONMENT "PATH=${compiler_bin}")
endif()

set(OASIS_HYBRID_GPGX_SOURCE "" CACHE PATH "External GPGX source for developer-only PoC")
if(OASIS_HYBRID_GPGX_SOURCE)
    add_executable(oasis_hybrid_poc runner.cpp)
    target_include_directories(oasis_hybrid_poc PRIVATE
        ${OASIS_HYBRID_GPGX_SOURCE}/libretro/libretro-common/include)
    target_link_libraries(oasis_hybrid_poc PRIVATE oasis_hybrid_contract ${CMAKE_DL_LIBS})
endif()
