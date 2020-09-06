cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_openql_test(test_cc cc/test_cc.cc cc)
add_openql_test(test_mapper test_mapper.cc .)
add_openql_test(program_test program_test.cc .)
add_openql_test(test_179 test_179.cc .)
