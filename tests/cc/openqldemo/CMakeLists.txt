add_openql_test(demo demo.cc .)

add_executable(run_openql run_openql.cc .)
target_link_libraries(run_openql PRIVATE ql)
target_compile_options(run_openql PUBLIC -Wall -Wfatal-errors -ggdb)