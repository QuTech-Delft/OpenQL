cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory(../.. openql)

add_executable(example example.cc)
target_link_libraries(example ql)
