add_library(api
    "misc.cc"
    "pass.cc"
    "compiler.cc"
    "platform.cc"
    "creg.cc"
    "operation.cc"
    "unitary.cc"
    "kernel.cc"
    "program.cc"
    "cqasm_reader.cc"
)

target_include_directories(api PUBLIC include)

target_link_libraries(api PRIVATE config)
target_link_libraries(api PUBLIC ir_compat)
target_link_libraries(api PUBLIC pmgr)
target_link_libraries(api PUBLIC com_dec)
target_link_libraries(api PUBLIC arch_diamond_annotations)