cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Look for SWIG.
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# Look for the Python libraries.
if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    find_package(PythonInterp 3 REQUIRED)
    if(${PYTHONINTERP_FOUND})
        find_package(PythonLibs "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
    endif()
    include_directories(${PYTHON_INCLUDE_DIRS})
else()
    find_package(Python COMPONENTS Interpreter Development)
    set(PYTHON_INCLUDE_DIRS ${Python_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python_LIBRARIES})
endif()

set_source_files_properties(openql.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(openql.i PROPERTIES INCLUDE_DIRECTORIES ${PYTHON_INCLUDE_DIRS})
set_property(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode -modern -keyword -w511)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
#WJV
# -modern is deprecated
# -keyword gives lots of warnings
#SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode)

if(${CMAKE_VERSION} VERSION_LESS "3.8.0")
    swig_add_module(openql python openql.i)
else()
    swig_add_library(
        openql
        TYPE MODULE
        LANGUAGE python
        SOURCES openql.i
    )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    swig_link_libraries(openql ql)
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else()
    swig_link_libraries(openql ${PYTHON_LIBRARIES} ql)
endif()

target_include_directories(_openql PRIVATE ${PYTHON_INCLUDE_DIRS})

install(
    TARGETS _openql
    LIBRARY DESTINATION ${OPENQL_PYTHON_DIR}
)
install(
    FILES ${CMAKE_SWIG_OUTDIR}/openql.py
    DESTINATION ${OPENQL_PYTHON_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openql/
    DESTINATION ${OPENQL_PYTHON_DIR}
    FILES_MATCHING PATTERN *.py
)
