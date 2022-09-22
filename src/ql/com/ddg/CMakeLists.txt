add_library(com_ddg
    "types.cc"
    "build.cc"
    "ops.cc"
    "consistency.cc"
    "dot.cc"
)

target_include_directories(com_ddg PUBLIC include)
target_link_libraries(com_ddg PUBLIC utils)
target_link_libraries(com_ddg PUBLIC ir)