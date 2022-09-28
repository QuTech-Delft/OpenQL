add_pass_library(pass_opt_clifford
    "detail/clifford.cc"
    "optimize.cc"
)

target_include_directories(pass_opt_clifford PUBLIC include)

target_link_libraries(pass_opt_clifford PUBLIC utils)
target_link_libraries(pass_opt_clifford PUBLIC pmgr)
target_link_libraries(pass_opt_clifford PRIVATE ir)