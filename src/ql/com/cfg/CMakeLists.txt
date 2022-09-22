add_library(com_cfg
    "build.cc"
    "ops.cc"
    "consistency.cc"
    "dot.cc"
)

target_include_directories(com_cfg PUBLIC include)
target_link_libraries(com_cfg PUBLIC utils)
target_link_libraries(com_cfg PUBLIC ir)