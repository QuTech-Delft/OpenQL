add_arch_library(arch_diamond_pass_gen
    "microcode.cc"
    "detail/functions.cc"
)

target_include_directories(arch_diamond_pass_gen PUBLIC include)

target_link_libraries(arch_diamond_pass_gen PUBLIC utils)
target_link_libraries(arch_diamond_pass_gen PUBLIC pmgr)
target_link_libraries(arch_diamond_pass_gen PUBLIC ir_compat)
target_link_libraries(arch_diamond_pass_gen PRIVATE com)