cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
cmake_policy(VERSION 3.1)
cmake_policy(SET CMP0078 OLD)
cmake_policy(SET CMP0086 OLD)

#=============================================================================#
# Configure, build, and link dependencies                                     #
#=============================================================================#

# SWIG ------------------------------------------------------------------------

# Look for SWIG.
find_package(SWIG REQUIRED)

# Include the support module, providing swig_add_module/library and
# swig_link_libraries.
include(${SWIG_USE_FILE})


# Python development headers --------------------------------------------------

# CMake's Python-finding logic is dodgy at best at dealing with venvs, and
# venvs themselves are dodgy at best at dealing with the development headers
# and so on. "If you want something done, do it yourself," I guess.

# For the manylinux build, we provide an environment variable which can be used
# to override the development header location.
set(PYTHON_INCLUDE_DIRS "PYTHON_INCLUDE_DIRS-NOTFOUND")
if(DEFINED ENV{PYTHON_INCLUDE_OVERRIDE})
    message(STATUS "Found python3 include directory override environment variable")
    set(PYTHON_INCLUDE_DIRS "$ENV{PYTHON_INCLUDE_OVERRIDE}")
else()

    # If there is no override, look for the Python executable first. If there's
    # a virtualenv, look in it first and foremost.
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
    endif()
endif()

# Print the directory we found for reference.
message(STATUS "Found python3 include directory at ${PYTHON_INCLUDE_DIRS}")


#=============================================================================#
# Build the SWIG module                                                       #
#=============================================================================#

# Configure SWIG.
set(SWIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/openql.i")
set_source_files_properties("${SWIG_FILE}" PROPERTIES CPLUSPLUS ON)
set_source_files_properties("${SWIG_FILE}" PROPERTIES INCLUDE_DIRECTORIES
    "$<TARGET_PROPERTY:ql,INTERFACE_INCLUDE_DIRECTORIES>;${PYTHON_INCLUDE_DIRS}"
)
set_property(SOURCE "${SWIG_FILE}" PROPERTY SWIG_FLAGS -castmode)
set(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}")

# Run SWIG to generated the C++ source file and Python wrapper module.
if(${CMAKE_VERSION} VERSION_LESS "3.8.0")
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
swig_link_libraries(openql ql)
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

# Install the generated wrapper module.
install(
    FILES "${CMAKE_SWIG_OUTDIR}/openql.py"
    DESTINATION "${OPENQL_PYTHON_DIR}"
)
