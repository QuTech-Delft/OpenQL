add_library(ir_cqasm
    "read.cc"
    "write.cc"
)

target_include_directories(ir_cqasm PUBLIC include)
target_link_libraries(ir_cqasm PUBLIC utils)
target_link_libraries(ir_cqasm PUBLIC ir)
target_link_libraries(ir_cqasm PUBLIC com_ddg)