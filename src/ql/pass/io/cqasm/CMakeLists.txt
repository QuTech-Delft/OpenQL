add_pass_library(pass_io_cqasm
    "read.cc"
    "report.cc"
)

target_include_directories(pass_io_cqasm PUBLIC include)

target_link_libraries(pass_io_cqasm PUBLIC utils)
target_link_libraries(pass_io_cqasm PUBLIC pmgr)