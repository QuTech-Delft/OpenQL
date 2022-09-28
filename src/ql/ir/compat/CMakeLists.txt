add_library(ir_compat
    "platform.cc"
    "gate.cc"
    "classical.cc"
    "bundle.cc"
    "kernel.cc"
    "program.cc"
    "cqasm_reader.cc"
    "detail/cqasm_reader.cc"
    "detail/cqasm_reader.h"
)

target_include_directories(ir_compat PUBLIC include)

target_link_libraries(ir_compat PRIVATE utils)
target_link_libraries(ir_compat PRIVATE com)
target_link_libraries(ir_compat PRIVATE com_dec)
target_link_libraries(ir_compat PUBLIC arch)