include(ExternalProject)

ExternalProject_add(Mergesat
    GIT_REPOSITORY "https://github.com/conp-solutions/mergesat.git"
    GIT_TAG "a694893e50eed973c0f01b47c092b86dface1eb5"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_MAKE_PROGRAM} config prefix=${CMAKE_CURRENT_BINARY_DIR}/external
    BUILD_IN_SOURCE 1
)

add_library(MergesatLib STATIC IMPORTED GLOBAL)
set_target_properties(MergesatLib PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libmergesat.a)
add_dependencies(MergesatLib Mergesat)

set(Mergesat_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/external/include/minisat)
set(Mergesat_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libmergesat.a)
