add_subdirectory(pass/gen/microcode)

add_arch_library(arch_diamond
    "info.cc"
)


create_resource(diamond_hwconf_default "resources/hwconf_default.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/diamond/resources/hwconf_default.inc")
add_dependencies(arch_diamond diamond_hwconf_default)

target_include_directories(arch_diamond PUBLIC "include")
target_include_directories(arch_diamond PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(arch_diamond PUBLIC utils)
target_link_libraries(arch_diamond PUBLIC pmgr)

add_library(arch_diamond_annotations INTERFACE) # This is done to break a cyclic dependency between cmake targets
target_include_directories(arch_diamond_annotations INTERFACE "include")
target_link_libraries(arch_diamond_annotations INTERFACE utils)