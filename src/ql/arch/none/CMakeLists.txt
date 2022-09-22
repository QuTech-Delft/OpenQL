add_arch_library(arch_none
    "info.cc"
)

target_include_directories(arch_none PUBLIC include)
create_resource(none_hwconf_default "resources/hwconf_default.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/none/resources/hwconf_default.inc")

add_dependencies(arch_none none_hwconf_default)

target_include_directories(arch_none PUBLIC include)
target_include_directories(arch_none PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(arch_none PUBLIC utils)
target_link_libraries(arch_none PRIVATE arch)