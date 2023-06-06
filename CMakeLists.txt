cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(OpenQL C CXX)

# If OpenQL was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET ql)

# Loads up the appropriate directories for installing stuff.
include(GNUInstallDirs)

find_package(Backward CONFIG REQUIRED)
find_package(cimg CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(highs CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Packages
include(FetchContent)
FetchContent_Declare(cqasm
    GIT_REPOSITORY https://github.com/QuTech-Delft/libqasm.git
    GIT_TAG "d3829ba0dbe7473939cd6dc821bd6d74ee9678be"
)
# TODO: use Lemon Conan package when it is ready (https://github.com/conan-io/conan-center-index/pull/17338)
# TODO: remove use of Lemon in project?
FetchContent_Declare(lemon
    URL "http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz"
    URL_HASH SHA256=71b7c725f4c0b4a8ccb92eb87b208701586cf7a96156ebd821ca3ed855bad3c8
)
FetchContent_MakeAvailable(
    cqasm
    lemon
)

add_custom_target(patch_lemon_sources
    DEPENDS patch_lemon_sources_command
)
add_custom_command(
    OUTPUT patch_lemon_sources_command
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/patches/array_map.h" "${CMAKE_CURRENT_BINARY_DIR}/_deps/lemon-src/lemon/bits/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/patches/maps.h" "${CMAKE_CURRENT_BINARY_DIR}/_deps/lemon-src/lemon/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/patches/random.h" "${CMAKE_CURRENT_BINARY_DIR}/_deps/lemon-src/lemon/"
)

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

# Make it possible to disable inclusion of debug symbols, in particular for PyPI wheels,
# since there is a size limit of 100MB.
option(
    OPENQL_DEBUG_SYMBOLS
    "Whether debug or release binaries should include debug symbols (it makes binaries a lot larger)."
    ON
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

# Everything needs C++20 support.
set(CMAKE_CXX_STANDARD 20)
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
    OBJECT
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/cfg/build.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/cfg/ops.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/cfg/consistency.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/cfg/dot.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/sch/heuristics.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/sch/scheduler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/map/expression_mapper.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/map/qubit_mapping.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/dec/unitary.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/dec/rules.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/com/dec/structure.cc"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/dec/instructions/instructions.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/dec/generalize/generalize.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/dec/specialize/specialize.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/dec/structure/structure.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/clifford/detail/clifford.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/clifford/optimize.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/const_prop/detail/propagate.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/const_prop/const_prop.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/opt/dead_code_elim/dead_code_elim.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/schedule/detail/scheduler.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/schedule/schedule.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/sch/list_schedule/list_schedule.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/pass/map/qubits/place_mip/detail/impl.cc"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/operands.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/backend.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/codesection.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/arch/cc/pass/gen/vq1asm/detail/functions.cc"
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

add_dependencies(ql patch_lemon_sources)

# There is no distinction between public and private header files right now,
# and they're all in the source directory.
# Note the / at the end of the path;
# this is necessary for the header files to be installed in the right location.
target_include_directories(ql
    SYSTEM PUBLIC "${Backward_INCLUDE_DIRS}"
    SYSTEM PUBLIC "${Eigen3_INCLUDE_DIRS}/unsupported"
    SYSTEM PUBLIC "${highs_INCLUDE_DIRS}"
    SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/_deps/lemon-src/"
    SYSTEM PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/_deps/lemon-build/"
    SYSTEM PUBLIC "${libqasm_INCLUDE_DIRS}"
    SYSTEM PUBLIC "${nlohmann_json_INCLUDE_DIRS}"
    SYSTEM PUBLIC ${TREE_LIB_PRIVATE_INCLUDE}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src/"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include/"
)

target_link_libraries(ql PUBLIC
    ${CONAN_LIBS}
    cqasm
    lemon
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

# Configure compilation.
set_property(TARGET ql PROPERTY POSITION_INDEPENDENT_CODE ON)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(ql PRIVATE -Wall -Wfatal-errors -Wno-unused-local-typedef)
elseif(MSVC)
    target_compile_options(ql PRIVATE /MP /D_USE_MATH_DEFINES /EHsc /bigobj)
else()
    message(SEND_ERROR "Unknown compiler!")
endif()

# Enable optimizations only for release builds.
if(NOT MSVC AND "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_options(ql PRIVATE -O3)
endif()

# Add debug information only for debug builds.
if(NOT MSVC AND OPENQL_DEBUG_SYMBOLS)
    target_compile_options(ql PRIVATE -ggdb)
endif()

# Use a mock version of unitary.cc if WITH_UNITARY_DECOMPOSITION is false.
# This speeds up the build, but of course breaks unitary decomposition.
if(NOT WITH_UNITARY_DECOMPOSITION)
    target_compile_definitions(ql PRIVATE WITHOUT_UNITARY_DECOMPOSITION)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_definitions(ql PRIVATE NDEBUG)
endif()


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
    target_include_directories(ql SYSTEM PRIVATE "${cimg_INCLUDE_DIRS}")
endif()

#=============================================================================#
# Testing                                                                     #
#=============================================================================#

# Include the tests directory if requested.
if(False)
#if(OPENQL_BUILD_TESTS)
    enable_testing()
    find_package(doctest CONFIG REQUIRED)

    # Convenience function to add an integration test.
    function(add_openql_test name source workdir)
        add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        target_link_libraries("${name}" PUBLIC ql)
        add_test(
            NAME "${name}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${workdir}"
            COMMAND "${name}"
        )
    endfunction()

    # Include the directories containing integration tests.
    add_subdirectory(test)
    add_subdirectory(tests)
    add_subdirectory(examples)

    # Convenience function to add a unit test.
    function(add_openql_unit_test source)
        string(REPLACE "/" "_" name ${source})
        string(REPLACE "_tests_" "_" name ${name})
        string(REPLACE ".cc" "" name ${name})
        set(name test_${name})
        add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/${source}")
        target_link_libraries("${name}" PUBLIC ql doctest::doctest)
        target_include_directories(ql SYSTEM PUBLIC "${doctest_INCLUDE_DIRS}")
        get_filename_component(working_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/ql/${source}" DIRECTORY)
        add_test(
            NAME "${name}"
            WORKING_DIRECTORY "${working_dir}"
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

endif() # NOT TARGET ql
