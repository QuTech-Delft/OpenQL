cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(OpenQL C CXX)

# If OpenQL was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET ql)

# Loads up the appropriate directories for installing stuff.
include(GNUInstallDirs)


#=============================================================================#
# Configuration options                                                       #
#=============================================================================#

# Library type option. Default is a shared object, because for CMake it doesn't
# matter, but outside of CMake dependency information is lost for static
# libraries. That requires the user to link all of ql's direct and transitive
# dependencies as well, which is terribly ugly. setup.py *has* to do this
# however, because "pip install ." builds this in a temporary directory, so the
# shared objects that get built and installed and are then depended on by the
# Python lib get deleted by pip after the install.
option(
    BUILD_SHARED_LIBS
    "Whether libraries should be built as a shared object or as a static library"
    ON
)

# With what optimization level the library is to be built.
if(NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE Release CACHE STRING
        "Type of build (Debug, Release)" FORCE
    )
    set_property(
        CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release"
    )
endif()

# Whether tests should be built.
option(
    OPENQL_BUILD_TESTS
    "Whether the tests should be built and added to `make test`"
    OFF
)

# Whether the Python module should be built. This should only be enabled for
# setup.py's builds.
option(
    OPENQL_BUILD_PYTHON
    "Whether the Python module should be built"
    OFF
)
mark_as_advanced(OPENQL_BUILD_PYTHON)

# Where the Python module should be built.
set(
    OPENQL_PYTHON_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/python/openql"
    CACHE STRING "Where to install the Python library"
)
mark_as_advanced(OPENQL_PYTHON_DIR)

# Used to override the (base)name of the Python extension.
set(
    OPENQL_PYTHON_EXT ""
    CACHE STRING "Basename for the Python extension, or \"\" to let CMake's SWIG implementation handle it"
)
mark_as_advanced(OPENQL_PYTHON_EXT)

# Whether unitary decomposition should be enabled. You can disable this while
# developing to cut back on compile-time. It can also be disabled for the
# ReadTheDocs build.
option(
    WITH_UNITARY_DECOMPOSITION
    "Whether unitary decomposition support should be enabled"
    ON
)

# Whether initial placement should be enabled. This requires one of GLPK, ILOG,
# or COIN to be installed and findable by lemon's build system. Only GLPK seems
# to work on present-day systems; on Linux it's probably in your package
# manager, on Mac it's `brew install glpk`, and on Windows you can use winglpk
# (see below). Unfortunately, GLPK is GPL-licensed, and therefore cannot be
# distributed in an Apache project, so this option is off by default until a
# non-GPL replacement is found.
option(
    WITH_INITIAL_PLACEMENT
    "Whether initial placement support should be enabled"
    OFF
)

# The following snippit helps finding GLPK from a Windows build package as
# from https://sourceforge.net/projects/winglpk/files/winglpk/. Simply set
# it to the root folder of the extracted zip file.
if(WITH_INITIAL_PLACEMENT AND DEFINED ENV{WINGLPK_ROOT_DIR})
    string(REPLACE "\\" "/" WINGLPK_ROOT_DIR "$ENV{WINGLPK_ROOT_DIR}")
    set(GLPK_INCLUDE_DIR "${WINGLPK_ROOT_DIR}/src" CACHE STRING "glpk include")
    mark_as_advanced(GLPK_INCLUDE_DIR)
    file(GLOB GLPK_LIB "${WINGLPK_ROOT_DIR}/w64/glpk_4_??.lib")
    set(GLPK_LIBRARY "${GLPK_LIB}" CACHE STRING "glpk library")
    mark_as_advanced(GLPK_LIBRARY)
    file(GLOB GLPK_DLL "${WINGLPK_ROOT_DIR}/w64/glpk_4_??.dll")
    set(GLPK_RUNTIME_LIBRARY "${GLPK_DLL}" CACHE STRING "glpk runtime library")
    mark_as_advanced(GLPK_RUNTIME_LIBRARY)
    set(WITHOUT_GLPK_INSTALL YES)
endif()


#=============================================================================#
# CMake weirdness and compatibility                                           #
#=============================================================================#

# On Windows builds, CMake complains that the CMake internal variable
# "CMAKE_MODULE_LINKER_FLAGS_MAINTAINER" is not defined *the first time you
# configure*. Weirdly, all is good with the world if you then just try again.
# It seems to have to do with the "maintainer" build type in MSVC, but there
# is no documentation whatsoever. In any case, this just mimics what CMake
# does automatically the second time you invoke it, so it works around the
# issue.
if(NOT DEFINED CMAKE_MODULE_LINKER_FLAGS_MAINTAINER)
    set(
        CMAKE_MODULE_LINKER_FLAGS_MAINTAINER ""
        CACHE STRING "Flags used by the linker during the creation of modules during MAINTAINER builds."
    )
endif()


#=============================================================================#
# Global build configuration                                                  #
#=============================================================================#

# Since we have multiple libraries to link together, we unfortunately have to
# worry about RPATH handling on Linux and OSX. See
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Everything needs C++11 support.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows weirdness: need a .lib file to link against a DLL at compile-time
# (I think), but only the .dll is generated when there are no exported symbols.
# This sort of fixes that (https://stackoverflow.com/questions/1941443)
SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


#=============================================================================#
# OpenQL library target                                                       #
#=============================================================================#

# Create the OpenQL library. This will be built either as a shared object/DLL
# or as a static library based on BUILD_SHARED_LIBS; add_library switches
# automatically.
add_library(ql
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/program.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/decompose_toffoli.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/buffer_insertion.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/latency_compensation.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/write_sweep_points.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clifford.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/passmanager.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/passes.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/report.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/exception.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/eqasm_backend_cc.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/codegen_cc.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/arch/cc/vcd.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/cqasm/cqasm_reader.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/unitary.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mapper.cc"
)

# This definition is used to define OPENQL_DECLSPEC for __declspec. More info:
# https://docs.microsoft.com/en-us/cpp/cpp/declspec?view=vs-2019
TARGET_COMPILE_DEFINITIONS(ql PRIVATE BUILDING_OPENQL)

# There is no distinction between public and private header files right now,
# and they'r all in the source directory. Note the / at the end of the path;
# this is necessary for the header files to be installed in the right location.
target_include_directories(ql
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

# Configure compilation.
set_property(TARGET ql PROPERTY POSITION_INDEPENDENT_CODE ON)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors -ggdb)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors -ggdb -Wno-unused-local-typedef )
elseif(MSVC)
    target_compile_options(ql PRIVATE /MP /D_USE_MATH_DEFINES /EHsc /bigobj)
else()
    message(SEND_ERROR "Unknown compiler!")
endif()

# Enable optimizations only for release builds.
if(NOT MSVC AND "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_options(ql PRIVATE -O3)
endif()

# Use a mock version of unitary.cc if WITH_UNITARY_DECOMPOSITION is false.
# This speeds up the build, but of course breaks unitary decomposition.
if(NOT WITH_UNITARY_DECOMPOSITION)
    target_compile_definitions(ql PRIVATE WITHOUT_UNITARY_DECOMPOSITION)
endif()

# Use a mock version of unitary.cc if WITH_UNITARY_DECOMPOSITION is false.
# This speeds up the build, but of course breaks unitary decomposition.
if(WITH_INITIAL_PLACEMENT)
    target_compile_definitions(ql PRIVATE INITIALPLACE)
endif()


#=============================================================================#
# Configure, build, and link dependencies                                     #
#=============================================================================#

# pthreads --------------------------------------------------------------------

# Look for thread support library (pthreads)
find_package(Threads REQUIRED)
target_link_libraries(ql PUBLIC ${CMAKE_THREAD_LIBS_INIT})


# CLI11 -----------------------------------------------------------------------

# Wrap CLI11 in an interface library to link against.
add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/CLI11/include/"
)
target_link_libraries(ql PUBLIC cli11)


# LEMON -----------------------------------------------------------------------

# Configure LEMON. LEMON by itself exposes the "lemon" target to link against,
# but it doesn't use target_include_directories(), so we have to do that here.
add_subdirectory(deps/lemon)
target_include_directories(lemon INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/lemon"
    "${CMAKE_CURRENT_BINARY_DIR}/deps/lemon"
)
target_link_libraries(ql PUBLIC lemon)

# Initial places uses lemon's Mip; ensure that LEMON's configuration supports
# it. Annoyingly, LEMON's build just silently removes it when it's missing the
# necessary dependencies. Also annoying: the variable LEMON_HAVE_MIP is a
# local variable, so we can't use it... and have to resort to checking this
# cache variable instead.
if(WITH_INITIAL_PLACEMENT AND NOT LEMON_DEFAULT_MIP)
    message(SEND_ERROR "Missing ilog/coin/glpk (-dev/-devel) library for lemon (one of the three is needed for initial placement support)")
endif()

# Even more annoying stuff: LEMON doesn't install itself in the right place on
# multilib systems (i.e. ones where the libdir is lib64 instead of just lib).
# So to make sure it is found in the install tree, we have to install it in the
# proper place ourselves. That would go something like this,
#
#     install(
#         TARGETS lemon
#         ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#         LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#         COMPONENT library
#     )
#
# but until CMake 3.13 the install directive MUST be in the directory where the
# target is created, so we have to insert that piece of code into LEMON's own
# CMakeLists.txt.


# Eigen -----------------------------------------------------------------------

# Wrap Eigen in an interface library to link against. Note that Eigen is only
# used internally; its headers need not be installed or exposed to programs
# linking against OpenQL.
add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/eigen/Eigen"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/eigen/unsupported"
)
target_link_libraries(ql PRIVATE eigen)


# libqasm ---------------------------------------------------------------------

# Load libqasm. libqasm's CMakeLists expose the "cqasm" target to link against.
option(LIBQASM_COMPAT "" ON)
add_subdirectory(deps/libqasm)
target_link_libraries(ql PUBLIC cqasm)


#=============================================================================#
# Testing                                                                     #
#=============================================================================#

# Include the tests directory if requested.
if(OPENQL_BUILD_TESTS)
    enable_testing()

    # Convenience function to add a test.
    function(add_openql_test name source workdir)
        add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        target_link_libraries("${name}" ql)
        add_test(
            NAME "${name}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${workdir}"
            COMMAND "${name}"
        )
    endfunction()

    # Include the directories containing tests.
    add_subdirectory(tests)
    add_subdirectory(examples)

endif()


#=============================================================================#
# Python module                                                               #
#=============================================================================#

# Include the tests directory if requested.
if(OPENQL_BUILD_PYTHON)
    add_subdirectory(python)
endif()


#=============================================================================#
# Installation                                                                #
#=============================================================================#

# Install targets for the OpenQL library.
install(
    TARGETS ql
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
    DIRECTORY "$<TARGET_PROPERTY:ql,INTERFACE_INCLUDE_DIRECTORIES>"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# On Windows + MSVC, install the GLPK runtime library if one was found.
if(GLPK_RUNTIME_LIBRARY)
    install(
        FILES "${GLPK_RUNTIME_LIBRARY}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()


endif() # NOT TARGET ql
