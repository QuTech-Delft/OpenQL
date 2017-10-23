project(qisa-as CXX)

cmake_minimum_required(VERSION 3.1)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  ## Add support for C++11
  add_definitions(-std=c++11 -O0 )
ENDIF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(PythonInterp 3 REQUIRED)

set(QISA_OPCODE_CONFIG_FILE "${PROJECT_SOURCE_DIR}/qisa_opcodes.qmap")

# Teach cmake how to generate the instruction declarations
# using the 'gen_qisa_instructions' python script.
add_custom_command(
  OUTPUT
    ${PROJECT_BINARY_DIR}/qisa_opcode_defs.inc
    ${PROJECT_BINARY_DIR}/qisa_lexer.l
    ${PROJECT_BINARY_DIR}/qisa_parser.yy
  COMMAND ${PYTHON_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/scripts/gen_qisa_instructions.py
    ARGS
      -d ${QISA_OPCODE_CONFIG_FILE}
      -co ${PROJECT_BINARY_DIR}/qisa_opcode_defs.inc
      -lt ${PROJECT_SOURCE_DIR}/qisa_lexer.tmpl
      -lo ${PROJECT_BINARY_DIR}/qisa_lexer.l
      -pt ${PROJECT_SOURCE_DIR}/qisa_parser.tmpl
      -po ${PROJECT_BINARY_DIR}/qisa_parser.yy
  DEPENDS
    ${QISA_OPCODE_CONFIG_FILE}
    ${PROJECT_SOURCE_DIR}/qisa_lexer.tmpl
    ${PROJECT_SOURCE_DIR}/qisa_parser.tmpl
    ${PROJECT_SOURCE_DIR}/scripts/gen_qisa_instructions.py
  COMMENT "[Generating instruction declarations]"
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

BISON_TARGET(qisa_parser
             ${PROJECT_BINARY_DIR}/qisa_parser.yy
             ${CMAKE_CURRENT_BINARY_DIR}/qisa_parser.tab.cc)

IF (WIN32)
  SET(FLEX_OPTIONS "--wincompat")
ENDIF (WIN32)

FLEX_TARGET(qisa_lexer
            ${PROJECT_BINARY_DIR}/qisa_lexer.l
            ${CMAKE_CURRENT_BINARY_DIR}/qisa_lexer.yy.cc
	    COMPILE_FLAGS ${FLEX_OPTIONS})

ADD_FLEX_BISON_DEPENDENCY(qisa_lexer qisa_parser)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})


# Generate the library that will be used by the main program as well as for exposing the assembler driver interface.
add_library(qisa-as-lib
  SHARED
  qisa_driver.cpp
  ${PROJECT_BINARY_DIR}/qisa_opcode_defs.inc
  qisa_parser.tmpl
  qisa_lexer.tmpl
  ${QISA_OPCODE_CONFIG_FILE}
  ${FLEX_qisa_lexer_OUTPUTS}
  ${BISON_qisa_parser_OUTPUTS}
)

add_executable(qisa-as main.cpp)
target_link_libraries(qisa-as qisa-as-lib)

set_property(TARGET qisa-as qisa-as-lib
             PROPERTY CXX_STANDARD 14)


# We use Swig to expose the assembler driver interface to Python

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs REQUIRED)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(qisa-as-swig.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(qisa-as-swig.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(pyQisaAs python qisa-as-swig.i)
SWIG_LINK_LIBRARIES(pyQisaAs ${PYTHON_LIBRARIES} qisa-as-lib)
