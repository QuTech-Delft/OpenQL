add_pass_library(pass_dec_generalize
    "generalize.cc"
)

target_include_directories(pass_dec_generalize PUBLIC include)

target_link_libraries(pass_dec_generalize PRIVATE com_dec)
target_link_libraries(pass_dec_generalize PUBLIC pmgr)