# Generate a header file with configuration options that cannot be compiled
# (entirely) into the shared/static library due to use of templates.
set(QL_CHECKED_VEC ${OPENQL_CHECKED_VEC})
set(QL_CHECKED_LIST ${OPENQL_CHECKED_LIST})
set(QL_CHECKED_MAP ${OPENQL_CHECKED_MAP})
set(QL_SHARED_LIB ${BUILD_SHARED_LIBS})
configure_file(
    "include/ql/config.h.template"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ql/config.h"
)

add_library(config INTERFACE)
target_include_directories(config INTERFACE "include")
target_include_directories(config INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include/")

add_subdirectory(api)
add_subdirectory(arch)
add_subdirectory(com)
add_subdirectory(ir)
add_subdirectory(pass)
add_subdirectory(pmgr)
add_subdirectory(resource)
add_subdirectory(rmgr)
add_subdirectory(utils)