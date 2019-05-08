FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LEMON_INCLUDE_DIRS}
)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -Wall -Wfatal-errors")

# SET(CMAKE_SWIG_FLAGS -castmode -modern -keyword)

SET_SOURCE_FILES_PROPERTIES(openql.i PROPERTIES CPLUSPLUS ON)
# SET_SOURCE_FILES_PROPERTIES(openql.i PROPERTIES SWIG_FLAGS -castmode -modern -keyword)

#WJV    SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode -modern -keyword)
# -modern is deprecated
# -keyword gives lots of warnings
SET_PROPERTY(SOURCE openql.i PROPERTY SWIG_FLAGS -castmode)

# FIXME: warning
#   CMake Deprecation Warning at /usr/local/Cellar/cmake/3.14.3/share/cmake/Modules/UseSWIG.cmake:559 (message):
#     SWIG_ADD_MODULE is deprecated.  Use SWIG_ADD_LIBRARY instead.

# SWIG_ADD_MODULE(openql python openql.i openql.cxx)
#WJV    SWIG_ADD_MODULE(openql python openql.i)
# SWIG_ADD_LIBRARY(openql LANGUAGE python SOURCES openql.i TYPE SHARED)
SWIG_ADD_LIBRARY(openql LANGUAGE python SOURCES openql.i TYPE SHARED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SWIG_LINK_LIBRARIES(openql ${LEMON_LIBRARIES})
    set_target_properties(_openql PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
else ()
    SWIG_LINK_LIBRARIES(openql ${PYTHON_LIBRARIES} ${LEMON_LIBRARIES})
endif()
