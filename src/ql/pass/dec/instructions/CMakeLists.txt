add_pass_library(pass_dev_instructions
    "instructions.cc"
)

target_include_directories(pass_dev_instructions PUBLIC include)
target_link_libraries(pass_dev_instructions PRIVATE com_dec)
target_link_libraries(pass_dev_instructions PUBLIC pmgr)