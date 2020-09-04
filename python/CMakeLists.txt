cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
if(POLICY CMP0078)
    cmake_policy(SET CMP0078 OLD)
endif()
if(POLICY CMP0086)
    cmake_policy(SET CMP0086 OLD)
endif()

#=============================================================================#
# Configure, build, and link dependencies                                     #
#=============================================================================#

# Python development headers --------------------------------------------------

# CMake's Python-finding logic is dodgy at best at dealing with venvs, and
# venvs themselves are dodgy at best at dealing with the development headers
# and so on. "If you want something done, do it yourself," I guess.

# If there is no override, look for the Python executable first. If there's
# a virtualenv, look in it first and foremost. PYTHON_EXECUTABLE can also be
# overridden with -DPYTHON_EXECUTABLE= outright.
if(DEFINED ENV{VIRTUAL_ENV})
    find_program(PYTHON_EXECUTABLE python3 HINTS "$ENV{VIRTUAL_ENV}/bin")
else()
    find_program(PYTHON_EXECUTABLE python3)
endif()

# If we found it, we can ask it where its include directory is, using the
# sysconfig module.
if(PYTHON_EXECUTABLE)
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_paths()[\"include\"])"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # On Windows, we need to explicitly link against the Python library as
    # well. Unfortunately we can't ask Python directly where this is, but
    # we can figure it out
    if(WIN32)
        execute_process(
            COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/python_lib.py" "${CMAKE_BUILD_TYPE}"
            OUTPUT_VARIABLE PYTHON_LIBRARIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

# If we failed to find the headers ourselves, fall back to CMake's internal
# logic.
if(NOT PYTHON_INCLUDE_DIRS)
    message(WARNING "Could NOT find python3 include directory with custom logic, falling back to CMake internal")
    unset(PYTHON_INCLUDE_DIRS)
    if("${CMAKE_VERSION}" VERSION_LESS "3.12.0")
        find_package(PythonInterp 3 REQUIRED)
        if(${PYTHONINTERP_FOUND})
            find_package(PythonLibs "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" REQUIRED)
        endif()
    else()
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        set(PYTHON_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}")
        set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    endif()
endif()

# Print the directory we found for reference.
message(STATUS "Found python3 include directory at ${PYTHON_INCLUDE_DIRS}")


# SWIG ------------------------------------------------------------------------

# Look for SWIG.
find_package(SWIG REQUIRED)

# Include the support module, providing swig_add_module/library and
# swig_link_libraries.
# Unfortunately, prior to CMake 3.12, there appears to be *no way whatsoever*
# to get the result of an include dir generator expression in the form of SWIG
# include dirs, due to the omission of COMMAND_EXPAND_LISTS in UseSWIG (and the
# non-existence of that directive prior to 3.8). While it's possible to use
# $<JOIN...> to insert -I into the list to turn them into flags, it's not
# possible to make those actually separate arguments as SWIG (sanely)
# expects them to be, because spaces are escaped, because generator
# expressions cannot return multiple arguments as they are in double
# quotes. Basically, the only way to use SWIG before 3.12 is to use
# globally scoped include_directories directives everywhere, scoping and
# transitive dependencies be damned. It's complicated and above all insane.
# So, for versions before 3.12, a patched module is provided in the compat
# directory. This wraps the actual SWIG command in a Python script that does
# the list expansion CMake 3.12- is so woefully incapable of.
if("${CMAKE_VERSION}" VERSION_LESS "3.12.0")
    include("${CMAKE_CURRENT_SOURCE_DIR}/compat/UseSWIG.cmake")
else()
    include(${SWIG_USE_FILE})
endif()


#=============================================================================#
# Build the SWIG module                                                       #
#=============================================================================#

# Configure SWIG.
set(SWIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/openql.i")

set_source_files_properties("${SWIG_FILE}" PROPERTIES
    CPLUSPLUS ON
    INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:ql,INTERFACE_INCLUDE_DIRECTORIES>;${PYTHON_INCLUDE_DIRS}"
    SWIG_FLAGS "-castmode"
)

set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")

# Run SWIG to generated the C++ source file and Python wrapper module.
if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    swig_add_module(openql python "${SWIG_FILE}")
else()
    swig_add_library(
        openql
        TYPE MODULE
        LANGUAGE python
        SOURCES "${SWIG_FILE}"
    )
endif()

# Compile the C++ source file and link it against the OpenQL library.
if(WIN32)
    swig_link_libraries(openql "${PYTHON_LIBRARIES}" ql)
else()
    swig_link_libraries(openql ql)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()
target_include_directories(_openql PRIVATE "${PYTHON_INCLUDE_DIRS}")


#=============================================================================#
# Construct & install the complete Python module                              #
#=============================================================================#

# Install the shared object (*.pyd on Windows).
if("${OPENQL_PYTHON_EXT}" STREQUAL "")
    install(
        TARGETS _openql
        LIBRARY DESTINATION "${OPENQL_PYTHON_DIR}"
    )
else()
    install(
        FILES "$<TARGET_FILE:_openql>"
        DESTINATION "${OPENQL_PYTHON_DIR}"
        RENAME "${OPENQL_PYTHON_EXT}"
    )
endif()

# On Windows, ship GLPK along with the wheel. On Mac this is handled by
# delocate, and on manylinux by auditwheel, and in a local Linux install it's
# not necessary.
if(WIN32 AND GLPK_RUNTIME_LIBRARY)
    install(
        FILES "${GLPK_RUNTIME_LIBRARY}"
        DESTINATION "${OPENQL_PYTHON_DIR}"
    )
endif()

# Install the generated wrapper module.
install(
    FILES "${CMAKE_SWIG_OUTDIR}/openql.py"
    DESTINATION "${OPENQL_PYTHON_DIR}"
)
