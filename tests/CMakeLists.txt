cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# FIXME: the commented-out examples seem to rely on stuff that was changed or
# removed. They should be fixed or removed altogether.
add_openql_test(test_cc cc/test_cc.cc cc)
add_openql_test(test_mapper test_mapper.cc .)
# add_openql_test(program_test program_test.cc .)
# add_openql_test(test_179 test_179.cc .)
