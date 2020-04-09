CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
CMAKE_POLICY(SET CMP0054 NEW)

PROJECT(EXAMPLE)

# TODO: update the path to openql and _openql.so according to your setup 
SET(OPENQL_ROOT /data/repos/quantum_repos/openql)
SET(OPENQL_LIB "${OPENQL_ROOT}/cbuild/swig/_openql.so")

SET(CMAKE_BUILD_TYPE Release CACHE STRING
    "Type of build (None Debug Release RelWithDebInfo MinSizeRel)" FORCE)
SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "RelWithDebInfo" "MinSizeRel")

SET(CMAKE_MODULE_PATH ${OPENQL_ROOT}/cmake)

FIND_PATH(LEMON_SOURCE_ROOT_DIR CMakeLists.txt
  PATHS ${OPENQL_ROOT}/lemon ${OPENQL_ROOT}/deps/lemon
  NO_DEFAULT_PATH
  DOC "Location of LEMON source as a CMAKE subproject")

IF(EXISTS ${LEMON_SOURCE_ROOT_DIR})
  ADD_SUBDIRECTORY(${LEMON_SOURCE_ROOT_DIR} deps/lemon)
  SET(LEMON_INCLUDE_DIRS
    ${LEMON_SOURCE_ROOT_DIR}
    ${PROJECT_BINARY_DIR}/deps/lemon
  )
  SET(LEMON_LIBRARIES lemon)
  UNSET(LEMON_ROOT_DIR CACHE)
  UNSET(LEMON_DIR CACHE)
  UNSET(LEMON_INCLUDE_DIR CACHE)
  UNSET(LEMON_LIBRARY CACHE)
ELSE()
  FIND_PACKAGE(LEMON QUIET NO_MODULE)
  FIND_PACKAGE(LEMON REQUIRED)
ENDIF()


SET(CLI11_INCLUDE_DIRS
  "${OPENQL_ROOT}/deps/CLI11/include"
)


## These are the include directories used by the compiler.
INCLUDE_DIRECTORIES(
  ${LEMON_INCLUDE_DIRS}
  ${CLI11_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${OPENQL_ROOT}/src
  ${OPENQL_ROOT}/swig
)

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wfatal-errors -ggdb")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -Wall -Wfatal-errors -ggdb")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++11 /MP /D_USE_MATH_DEFINES")
ENDIF(MSVC)

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wfatal-errors -ggdb")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -Wall -Wfatal-errors -Wno-unused-local-typedef -ggdb")
ENDIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")


ADD_LIBRARY(openql_lib SHARED IMPORTED)
SET_TARGET_PROPERTIES(openql_lib PROPERTIES
  IMPORTED_LOCATION ${OPENQL_LIB}
)

# TODO: change the name of .cc file which you want compile if not example.cc
ADD_EXECUTABLE(example example.cc )
TARGET_LINK_LIBRARIES(example openql_lib ${LEMON_LIBRARIES} )
