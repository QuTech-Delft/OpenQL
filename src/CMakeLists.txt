PROJECT(ql)

GET_FILENAME_COMPONENT(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

ADD_LIBRARY(ql
  platform.cc
  program.cc
  compiler.cc
  decompose_toffoli.cc
  buffer_insertion.cc
  latency_compensation.cc
  write_sweep_points.cc
  optimizer.cc
  clifford.cc
  passmanager.cc
  passes.cc
  visualizer.cc
  report.cc
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
