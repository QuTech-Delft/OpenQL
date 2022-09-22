add_pass_library(pass_dec_structure
    "structure.cc"
)

target_include_directories(pass_dec_structure PUBLIC include)

target_link_libraries(pass_dec_structure PRIVATE com_dec)
target_link_libraries(pass_dec_structure PRIVATE com_cfg)
target_link_libraries(pass_dec_structure PUBLIC pmgr)