# Keep the native capture prototype and its tests out of the game runtime.
add_library(oasis_live_forward_worker STATIC
    ${CMAKE_CURRENT_LIST_DIR}/live_forward_trace.c
    ${CMAKE_CURRENT_LIST_DIR}/live_forward_worker.c
    ${CMAKE_CURRENT_LIST_DIR}/live_forward_flow.c
    ${CMAKE_CURRENT_LIST_DIR}/trace_ring_compat.c
)
target_include_directories(oasis_live_forward_worker PUBLIC ${CMAKE_CURRENT_LIST_DIR})
add_executable(oasis_live_forward_worker_test
    ${PROJECT_SOURCE_DIR}/tests/live_forward_worker_test.cpp
)
target_link_libraries(oasis_live_forward_worker_test PRIVATE oasis_live_forward_worker)
add_test(NAME oasis_live_forward_worker COMMAND oasis_live_forward_worker_test)
if(MSVC)
    target_compile_options(oasis_live_forward_worker_test PRIVATE /UNDEBUG)
else()
    target_compile_options(oasis_live_forward_worker_test PRIVATE -UNDEBUG)
endif()
