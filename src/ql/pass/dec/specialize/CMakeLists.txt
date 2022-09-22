add_pass_library(pass_dec_specialize
    "specialize.cc"
)

target_include_directories(pass_dec_specialize PUBLIC include)
target_link_libraries(pass_dec_specialize PUBLIC com_dec)
target_link_libraries(pass_dec_specialize PUBLIC utils)
target_link_libraries(pass_dec_specialize PUBLIC pmgr)