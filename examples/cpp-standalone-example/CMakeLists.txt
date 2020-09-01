cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# This would be just add_subdirectory(OpenQL) for your program, or perhaps
# add_subdirectory(deps/OpenQL) if you prefer; wherever your OpenQL git
# submodule is.
add_subdirectory(../.. OpenQL)

# Use whatever CMake magic you need to build your program, but linking
# something against OpenQL should be as easy as the second line. This should
# take care of both the libraries and header file include directories.
add_executable(example example.cc)
target_link_libraries(example ql)
