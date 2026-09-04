# Enforce the project's hard 500-line limit for executable/source-code files.
# Prose/reference Markdown is intentionally exempt from the numeric limit.

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(MAX_LINES 500)

file(GLOB_RECURSE PROJECT_FILES
    "${ROOT}/*.cpp"
    "${ROOT}/*.hpp"
    "${ROOT}/*.h"
    "${ROOT}/*.c"
    "${ROOT}/*.cc"
    "${ROOT}/*.cxx"
    "${ROOT}/*.hh"
    "${ROOT}/*.hxx"
    "${ROOT}/*.cmake"
    "${ROOT}/CMakeLists.txt"
)

set(violations "")

foreach(path IN LISTS PROJECT_FILES)
    if(path MATCHES "/build[^/]*/" OR path MATCHES "/cmake-build-[^/]*/")
        continue()
    endif()

    file(READ "${path}" contents)
    string(REGEX REPLACE "[^\n]" "" line_breaks "${contents}")
    string(LENGTH "${line_breaks}" line_count)
    if(NOT contents STREQUAL "" AND NOT contents MATCHES "\n$")
        math(EXPR line_count "${line_count} + 1")
    endif()

    if(line_count GREATER MAX_LINES)
        file(RELATIVE_PATH rel "${ROOT}" "${path}")
        string(APPEND violations "${rel}: ${line_count} lines\n")
    endif()
endforeach()

if(violations)
    message(FATAL_ERROR "Source-code files exceed ${MAX_LINES}-line project limit:\n${violations}")
endif()

message(STATUS "Source-code size rule passed: all checked files are <= ${MAX_LINES} lines; documentation is exempt")
