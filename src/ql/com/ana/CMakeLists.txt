add_library(com_ana
    "metrics.cc"
    "interaction_matrix.cc"
)

target_include_directories(com_ana PUBLIC include)

target_link_libraries(com_ana PUBLIC utils)
target_link_libraries(com_ana PUBLIC ir)
target_link_libraries(com_ana PUBLIC ir_compat)