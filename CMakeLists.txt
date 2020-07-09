cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(OpenQL CXX)

# If OpenQL was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET ql)

# Library type option. Default is a static library.
option(
    BUILD_SHARED_LIBS
    "whether libraries should be built as a shared object or as a static library"
    OFF
)

# With what optimization level the library is to be built.
# JvS: I'm not sure this is doing much.
if(NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE Release CACHE STRING
        "Type of build (None Debug Release RelWithDebInfo MinSizeRel)" FORCE
    )
    set_property(
        CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel"
    )
endif()

# Whether tests should be built.
option(
    OPENQL_BUILD_TESTS
    "whether the tests should be built and added to `make test`"
    OFF
)

# Loads up the appropriate directories for installing stuff.
include(GNUInstallDirs)

# Look for thread support library (pthreads)
find_package(Threads)

# Wrap CLI11 in an interface library to link against.
add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/CLI11/include
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/CLI11/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Configure LEMON and wrap it in an interface library to link against.
add_subdirectory(deps/lemon)
target_include_directories(lemon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lemon
    ${CMAKE_CURRENT_BINARY_DIR}/deps/lemon
)

# Wrap Eigen in an interface library to link against.
add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/eigen/Eigen
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/eigen/unsupported
)

# Load libqasm. libqasm's CMakeLists expose the "cqasm" target to link against.
OPTION(LIBQASM_COMPAT "" ON)
add_subdirectory(deps/libqasm)

# Create the OpenQL library.
add_library(ql
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/program.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/decompose_toffoli.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer_insertion.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/latency_compensation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/write_sweep_points.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clifford.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/passmanager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/passes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/report.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/exception.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/eqasm_backend_cc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/codegen_cc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/vcd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cqasm/cqasm_reader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/unitary.cc
)
target_include_directories(ql
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/
)
target_link_libraries(ql
    PUBLIC cli11 lemon cqasm
    PRIVATE eigen ${CMAKE_THREAD_LIBS_INIT}
)
set_property(TARGET ql PROPERTY CXX_STANDARD 11)
set_property(TARGET ql PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ql PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ql PROPERTY POSITION_INDEPENDENT_CODE ON)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ql PRIVATE -O3 -Wall -Wfatal-errors -ggdb)
endif()
if(MSVC)
    target_compile_options(ql PRIVATE /MP /D_USE_MATH_DEFINES /EHsc /bigobj)
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(ql PRIVATE -O3 -Wall -Wfatal-errors -ggdb -Wno-unused-local-typedef )
endif()

# Install targets for the OpenQL library.
install(
    TARGETS ql
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
# file(GENERATE OUTPUT fgsfds CONTENT $<TARGET_PROPERTY:ql,INTERFACE_INCLUDE_DIRECTORIES>)
# get_target_property(INCLUDE_DIRS ql INTERFACE_INCLUDE_DIRECTORIES)
install(
    DIRECTORY $<TARGET_PROPERTY:ql,INTERFACE_INCLUDE_DIRECTORIES>
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Include the tests directory if requested.
if(OPENQL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()

endif() # NOT TARGET ql
