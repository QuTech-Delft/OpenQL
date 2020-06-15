PROJECT(ql)

GET_FILENAME_COMPONENT(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

ADD_LIBRARY(ql
  platform.cc
  program.cc
  exception.cc
  arch/cc/eqasm_backend_cc.cc
  arch/cc/codegen_cc.cc
  arch/cc/vcd.cc
  cqasm/cqasm_reader.cc
  unitary.cc
  )

# support 'include <ql/foo.h>'
#TARGET_INCLUDE_DIRECTORIES(ql SYSTEM
#  PUBLIC ${PARENT_DIR}
#  )

SET_TARGET_PROPERTIES(ql PROPERTIES POSITION_INDEPENDENT_CODE ON)

UNSET(PARENT_DIR)
