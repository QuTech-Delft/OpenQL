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


# Python development package --------------------------------------------------

# Look for the Python development package. Steps depend on CMake version.
# The location of the Python include directory can be overridden with the
# PYTHON_INCLUDE_OVERRIDE environment variable. In this case, we don't need to
# have CMake look for the Python development headers. This is necessary for
# the manylinux container, where the system Python version doesn't come with
# development headers (it's really only there for when you need Python in your
# build process) and you should be linking against Python versions installed in
# non-PATH locations, which CMake can't find.
if(DEFINED ENV{PYTHON_INCLUDE_OVERRIDE})
#     if("${CMAKE_VERSION}" VERSION_LESS "3.12.0")
#         find_package(PythonInterp 3 REQUIRED)
#     else()
#         find_package(Python3 COMPONENTS Interpreter REQUIRED)
#     endif()
    set(PYTHON_INCLUDE_DIRS "$ENV{PYTHON_INCLUDE_OVERRIDE}")
else()
    if(NOT "$PYENV_ROOT" STREQUAL "")
        set(Python_ROOT_DIR "$PYENV_ROOT")
    endif()

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
