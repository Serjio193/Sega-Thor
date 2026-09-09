if(NOT DEFINED CORE_SOURCE_DIR)
    message(FATAL_ERROR "CORE_SOURCE_DIR is required")
endif()

file(GLOB_RECURSE CORE_FILES
    "${CORE_SOURCE_DIR}/*.cpp"
    "${CORE_SOURCE_DIR}/*.hpp"
    "${CORE_SOURCE_DIR}/*.h"
)
set(forbidden
    "tools/hybrid" "oasis_hybrid" "libretro" "GPGX" "gpgx"
    "0x003A0C" "0x00389E" "0x0003F0" "0x061266"
    "0x12DA" "0x51CA" "0x51C8" "0x421D" "0x4258" "0xFFFC"
)
foreach(path IN LISTS CORE_FILES)
    file(READ "${path}" contents)
    foreach(token IN LISTS forbidden)
        string(FIND "${contents}" "${token}" position)
        if(NOT position EQUAL -1)
            message(FATAL_ERROR "oasis_core source contains forbidden dependency '${token}': ${path}")
        endif()
    endforeach()
endforeach()
message(STATUS "oasis_core dependency boundary passed")
