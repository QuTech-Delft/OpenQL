cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

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
if("${CMAKE_VERSION}" VERSION_LESS "3.12.0")
    find_package(PythonInterp 3 REQUIRED)
    if(${PYTHONINTERP_FOUND})
        find_package(PythonLibs "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" REQUIRED)
    endif()
else()
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    set(PYTHON_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}")
    set(PYTHON_LIBRARIES "${Python3_LIBRARIES}")
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
#WJV
# -modern is deprecated
# -keyword gives lots of warnings
#SET_PROPERTY(SOURCE "${SWIG_FILE}" PROPERTY SWIG_FLAGS -castmode)
# JvS: I suppose this depends on the version of SWIG? No drama on the version
# my distro comes with (3.0.12). Seems to work just fine without the flags too,
# though.

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
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    swig_link_libraries(openql ql)
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else()
    swig_link_libraries(openql "${PYTHON_LIBRARIES}" ql)
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
