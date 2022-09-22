add_pass_library(pass_opt_const_prop
    "detail/propagate.cc"
    "const_prop.cc"
)

target_include_directories(pass_opt_const_prop PUBLIC include)

target_link_libraries(pass_opt_const_prop PUBLIC utils)
target_link_libraries(pass_opt_const_prop PUBLIC pmgr)
target_link_libraries(pass_opt_const_prop PRIVATE ir)