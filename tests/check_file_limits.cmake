# Enforce the project's hard 500-line limit for executable/source-code files.
# Prose/reference Markdown is intentionally exempt from the numeric limit.

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(MAX_LINES 500)

# Keep inventory bounded to repository-owned implementation roots. The old
# workspace-wide GLOB_RECURSE walked ignored build/runtime trees on /mnt/c and
# could spend indefinitely in generated evidence and external tool artifacts.
# Git lists tracked and non-ignored untracked files without recursively
# stat-ing those excluded trees. The explicit pathspecs are the governed
# source/build/test roots; generated output belongs outside them.
set(GOVERNED_PATHS "cmake" "src" "tests")
execute_process(
    COMMAND git -C "${ROOT}" ls-files --cached --others --exclude-standard
            --full-name -- "CMakeLists.txt" ${GOVERNED_PATHS}
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE GIT_FILES_RAW
    ERROR_VARIABLE GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "Unable to enumerate governed files with git: ${GIT_ERROR}")
endif()

set(PROJECT_FILES)
string(REPLACE "\r\n" "\n" GIT_FILES_RAW "${GIT_FILES_RAW}")
string(REPLACE "\n" ";" GIT_FILES "${GIT_FILES_RAW}")
foreach(relative IN LISTS GIT_FILES)
    if(relative STREQUAL "CMakeLists.txt" OR
       relative MATCHES "^(cmake|src|tests)/.*\\.(cpp|hpp|h|c|cc|cxx|hh|hxx|cmake|py|lua|ps1|sh|java|js|ts)$")
        list(APPEND PROJECT_FILES "${ROOT}/${relative}")
    endif()
endforeach()
list(LENGTH GIT_FILES GOVERNED_INVENTORY_COUNT)
list(REMOVE_DUPLICATES PROJECT_FILES)
list(SORT PROJECT_FILES)
list(LENGTH PROJECT_FILES PROJECT_FILE_COUNT)

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

message(STATUS "Source-code size rule passed: ${PROJECT_FILE_COUNT} governed files <= ${MAX_LINES} lines; inventory=${GOVERNED_INVENTORY_COUNT}; documentation is exempt")
