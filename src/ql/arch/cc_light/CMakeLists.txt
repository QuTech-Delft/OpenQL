add_arch_library(arch_cc_light
    "info.cc"
)

create_resource(cc_light_hwconf_default "resources/hwconf_default.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/cc_light/resources/hwconf_default.inc")
create_resource(cc_light_hwconf_s5 "resources/hwconf_s5.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/cc_light/resources/hwconf_s5.inc")
create_resource(cc_light_hwconf_s7 "resources/hwconf_s7.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/cc_light/resources/hwconf_s7.inc")
create_resource(cc_light_hwconf_s17 "resources/hwconf_s17.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/cc_light/resources/hwconf_s17.inc")
target_include_directories(arch_cc_light PUBLIC include)

add_dependencies(arch_cc_light cc_light_hwconf_default)
add_dependencies(arch_cc_light cc_light_hwconf_s5)
add_dependencies(arch_cc_light cc_light_hwconf_s7)
add_dependencies(arch_cc_light cc_light_hwconf_s17)

target_include_directories(arch_cc_light PUBLIC include)
target_include_directories(arch_cc_light PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
target_link_libraries(arch_cc_light PUBLIC utils)
target_link_libraries(arch_cc_light PUBLIC pmgr)