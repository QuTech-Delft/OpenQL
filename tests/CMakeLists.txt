cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

enable_testing()

function(add_openql_test name source workdir)
    add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    target_link_libraries("${name}" ql)
    add_test(
        NAME "${name}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${workdir}"
        COMMAND "${name}"
    )
endfunction()

# FIXME: the commented-out tests either legitimately fail or seem to rely on
# stuff that was changed or removed. If they indeed fail (to compile) only
# because they were never updated to the latest API, they should be fixed or
# removed.
# add_openql_test(test_cc cc/test_cc.cc cc)
add_openql_test(test_mapper test_mapper.cc .)
# add_openql_test(program_test program_test.cc .)
# add_openql_test(test_179 test_179.cc .)
