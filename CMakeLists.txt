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

# Definitions for checked vs unchecked STL containers w.r.t. iterator safety.
# The checked versions can be considerably slower than the unchecked ones, so
# by default they are only checked in debug builds.
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(OPENQL_DEBUG OFF)
else()
    set(OPENQL_DEBUG ON)
endif()
option(
    OPENQL_CHECKED_STL
    "Whether STL containers should be guarded against undefined behavior."
    ${OPENQL_DEBUG}
)
option(
    OPENQL_CHECKED_VEC
    "Whether ql::utils::Vec should guard against undefined behavior."
    ${OPENQL_CHECKED_STL}
)
option(
    OPENQL_CHECKED_LIST
    "Whether ql::utils::Vec should guard against undefined behavior."
    ${OPENQL_CHECKED_STL}
)
option(
    OPENQL_CHECKED_MAP
    "Whether ql::utils::Vec should guard against undefined behavior."
    ${OPENQL_CHECKED_STL}
)


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
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


#=============================================================================#
# Compile code generators                                                     #
#=============================================================================#

# Simple program that generates resource include files, to include the contents
# of a file as a constant inside the OpenQL library.
add_executable(resource src/resource/main.cpp)
function(create_resource fname)
    set(infile "${CMAKE_CURRENT_SOURCE_DIR}/${fname}")
    get_filename_component(outdir "${CMAKE_CURRENT_BINARY_DIR}/${fname}" DIRECTORY)
    get_filename_component(outname "${CMAKE_CURRENT_BINARY_DIR}/${fname}" NAME_WE)
    set(outfile "${outdir}/${outname}.inc")
    add_custom_command(
        OUTPUT "${outfile}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${outdir}"
        COMMAND resource "${infile}" "${outdir}" "${outname}"
        DEPENDS "${infile}" resource
    )
    string(REGEX REPLACE "[\\./\\]" "_" target_name "${fname}")
    add_custom_target(
        ${target_name}
        DEPENDS "${outfile}"
    )
    add_dependencies(ql ${target_name})
endfunction()

# Add tree-gen. We need to do this *only* to get access to the generate_tree
# CMake function; everything else is handled implicitly because tree-gen is a
# transitive dependency of libqasm.
add_subdirectory(deps/libqasm/src/cqasm/tree-gen deps/tree-gen)


#=============================================================================#
# OpenQL library target                                                       #
#=============================================================================#

# Build the IR tree using tree-gen.
generate_tree(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/ir.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ql/ir/ir.gen.h"
    "${CMAKE_CURRENT_BINARY_DIR}/src/ql/ir/ir.gen.cc"
)

# Create the OpenQL library. This will be built either as a shared object/DLL
# or as a static library based on BUILD_SHARED_LIBS; add_library switches
# automatically.
add_library(ql
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/num.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/str.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/rangemap.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/exception.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/logger.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/filesystem.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/json.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/tree.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/vcd.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/options.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/utils/progress.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/platform.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/gate.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/classical.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/bundle.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/kernel.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/program.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/cqasm_reader.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/compat/detail/cqasm_reader.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/prim.cc"
    "${CMAKE_CURRENT_BINARY_DIR}/src/ql/ir/ir.gen.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/ops.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/operator_info.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/describe.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/consistency.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/old_to_new.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/new_to_old.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/cqasm/read.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/ir/cqasm/write.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/options.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/topology.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ana/metrics.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ana/interaction_matrix.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ddg/types.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ddg/build.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ddg/ops.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ddg/consistency.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/ddg/dot.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/sch/heuristics.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/sch/scheduler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/map/expression_mapper.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/map/qubit_mapping.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/dec/unitary.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/dec/rules.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/rmgr/resource_types/base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/rmgr/types.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/rmgr/factory.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/rmgr/state.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/rmgr/manager.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/resource/qubit.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/resource/instrument.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/resource/inter_core_channel.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/pass_types/base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/pass_types/specializations.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/condition.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/group.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/factory.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pmgr/manager.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/statistics/annotations.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/statistics/report.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/statistics/clean.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/types.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/common.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/image.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/circuit.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/interaction.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/detail/mapping.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/circuit.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/interaction.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/ana/visualize/mapping.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/io/cqasm/read.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/io/cqasm/report.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/io/sweep_points/write.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/dec/instructions/instructions.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/clifford/detail/clifford.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/clifford/optimize.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/schedule/detail/scheduler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/schedule/schedule.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/list_schedule/list_schedule.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/place_mip/detail/algorithm.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/place_mip/place_mip.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/options.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/free_cycle.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/past.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/alter.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/future.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/detail/mapper.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/map/map.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/info_base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/architecture.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/factory.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/info.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/backend.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/datapath.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/settings.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/vcd.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/vq1asm.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc_light/info.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/none/info.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/diamond/info.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/diamond/pass/gen/microcode/microcode.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/diamond/pass/gen/microcode/detail/functions.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/misc.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/pass.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/compiler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/platform.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/creg.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/operation.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/unitary.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/kernel.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/program.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/api/cqasm_reader.cc"
)

# Specify resources.
create_resource("src/ql/arch/cc/resources/hwconf_default.json")
create_resource("src/ql/arch/cc_light/resources/hwconf_default.json")
create_resource("src/ql/arch/cc_light/resources/hwconf_s5.json")
create_resource("src/ql/arch/cc_light/resources/hwconf_s7.json")
create_resource("src/ql/arch/cc_light/resources/hwconf_s17.json")
create_resource("src/ql/arch/none/resources/hwconf_default.json")
create_resource("src/ql/arch/diamond/resources/hwconf_default.json")

# Generate a header file with configuration options that cannot be compiled
# (entirely) into the shared/static library due to use of templates.
set(QL_CHECKED_VEC ${OPENQL_CHECKED_VEC})
set(QL_CHECKED_LIST ${OPENQL_CHECKED_LIST})
set(QL_CHECKED_MAP ${OPENQL_CHECKED_MAP})
set(QL_SHARED_LIB ${BUILD_SHARED_LIBS})
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/config.h.template"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ql/config.h"
)

# This definition is used to define OPENQL_DECLSPEC for __declspec. More info:
# https://docs.microsoft.com/en-us/cpp/cpp/declspec?view=vs-2019
target_compile_definitions(ql PRIVATE BUILDING_OPENQL)

# There is no distinction between public and private header files right now,
# and they'r all in the source directory. Note the / at the end of the path;
# this is necessary for the header files to be installed in the right location.
target_include_directories(ql
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src/"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include/"
)

# Configure compilation.
set_property(TARGET ql PROPERTY POSITION_INDEPENDENT_CODE ON)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors -ggdb)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors -ggdb -Wno-unused-local-typedef)
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

# Enable GPL-based initial placement code if requested.
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


# nlohmann::json --------------------------------------------------------------

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(deps/json)
target_link_libraries(ql PUBLIC nlohmann_json::nlohmann_json)


# libqasm ---------------------------------------------------------------------

# Load libqasm. libqasm's CMakeLists expose the "cqasm" target to link against.
add_subdirectory(deps/libqasm)
target_link_libraries(ql PUBLIC cqasm)


# X11/CImg ---------------------------------------------------------------------

# Only enable the visualizer if building on Windows or the X11 library is found when building on Linux or Mac.
if(WIN32)
    set(QL_VISUALIZER yes)
else()
    find_package(X11)
    if(X11_FOUND)
        set(QL_VISUALIZER yes)
        message("X11 libraries: ${X11_LIBRARIES}")
        target_link_libraries(ql PUBLIC ${X11_LIBRARIES})
        message("X11 include path: ${X11_INCLUDE_DIR}")
        target_include_directories(ql PRIVATE "${X11_INCLUDE_DIR}")
    else()
        set(QL_VISUALIZER no)
    endif()
endif()

if(QL_VISUALIZER)
    target_compile_definitions(ql PRIVATE WITH_VISUALIZER)
    target_include_directories(ql PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/cimg/include/")
endif()

# backward-cpp ----------------------------------------------------------------

# Stack trace helper library, nothing functional here.
add_subdirectory(deps/backward-cpp)
add_backward(ql)

# add_backward doesn't set INTERFACE_LINK_LIBRARIES, only LINK_LIBRARIES. That
# goes wrong when we're compiling statically, because said libraries are shared
# and need to be included in the final link.
set_property(TARGET ql APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${BACKWARD_LIBRARIES})


#=============================================================================#
# Testing                                                                     #
#=============================================================================#

# Include the tests directory if requested.
if(OPENQL_BUILD_TESTS)
    enable_testing()

    # Convenience function to add an integration test.
    function(add_openql_test name source workdir)
        add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        target_link_libraries("${name}" ql)
        add_test(
            NAME "${name}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${workdir}"
            COMMAND "${name}"
        )
    endfunction()

    # Include the directories containing integration tests.
    add_subdirectory(tests)
    add_subdirectory(examples)

    # Convenience function to add a unit test.
    function(add_openql_unit_test source)
        string(REPLACE "/" "_" name ${source})
        string(REPLACE "_tests_" "_" name ${name})
        string(REPLACE ".cc" "" name ${name})
        set(name test_${name})
        add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/${source}")
        target_link_libraries("${name}" ql)
        add_test(
            NAME "${name}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
            COMMAND "${name}"
        )
    endfunction()

    # Register unit tests.
    file(
        GLOB_RECURSE unit_tests
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/ql
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ql/*/tests/*.cc
    )
    foreach(unit_test ${unit_tests})
        add_openql_unit_test(${unit_test})
    endforeach()

    # I hate CMake.
    file(
        GLOB_RECURSE unit_tests
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/ql
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ql/*/*/tests/*.cc
    )
    foreach(unit_test ${unit_tests})
        add_openql_unit_test(${unit_test})
    endforeach()
    file(
        GLOB_RECURSE unit_tests
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/ql
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ql/*/*/*/tests/*.cc
    )
    foreach(unit_test ${unit_tests})
        add_openql_unit_test(${unit_test})
    endforeach()
    file(
        GLOB_RECURSE unit_tests
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src/ql
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ql/*/*/*/*/tests/*.cc
    )
    foreach(unit_test ${unit_tests})
        add_openql_unit_test(${unit_test})
    endforeach()

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
