project(ql)

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

add_library(ql
  platform.cc
  exception.cc
  arch/cc/eqasm_backend_cc.cc
  arch/cc/codegen_cc.cc
  )

# support 'include <ql/foo.h>'
target_include_directories(ql SYSTEM
  PUBLIC ${PARENT_DIR}
  )

unset(PARENT_DIR)
