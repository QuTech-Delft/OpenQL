add_pass_library(pass_opt_dead_code_elim
    "dead_code_elim.cc"
)

target_include_directories(pass_opt_dead_code_elim PUBLIC include)

target_link_libraries(pass_opt_dead_code_elim PUBLIC utils)
target_link_libraries(pass_opt_dead_code_elim PUBLIC pmgr)
target_link_libraries(pass_opt_dead_code_elim PRIVATE ir)