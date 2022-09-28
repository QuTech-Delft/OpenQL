add_arch_library(arch_cc
    "info.cc"
    "pass/gen/vq1asm/detail/backend.cc"
    "pass/gen/vq1asm/detail/codegen.cc"
    "pass/gen/vq1asm/detail/codesection.cc"
    "pass/gen/vq1asm/detail/datapath.cc"
    "pass/gen/vq1asm/detail/functions.cc"
    "pass/gen/vq1asm/detail/operands.cc"
    "pass/gen/vq1asm/detail/settings.cc"
    "pass/gen/vq1asm/detail/vcd.cc"
    "pass/gen/vq1asm/vq1asm.cc"
)

create_resource(cc_hwconf_default "resources/hwconf_default.json" "${CMAKE_CURRENT_BINARY_DIR}/include/ql/arch/cc/resources/hwconf_default.inc")
add_dependencies(arch_cc cc_hwconf_default)

target_include_directories(arch_cc PUBLIC include)
target_include_directories(arch_cc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(arch_cc PUBLIC utils)
target_link_libraries(arch_cc PUBLIC pmgr)
target_link_libraries(arch_cc PRIVATE ir)
target_link_libraries(arch_cc PUBLIC config)